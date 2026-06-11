import subprocess
import json
import re
import traceback
import gzip
import shutil
from pathlib import Path
import yaml

# Wenn True, werden Compile-Kommandos und vollstaendiger Test-Output geloggt.
# Standardmaessig aus, damit der Log sauber bleibt.
VERBOSE = False

# Nicht-kompilierende Tests gehen als 0 Covered / volles Total in den
# Micro-Average ein (statt rausgerechnet zu werden) -> sie ziehen die
# Coverage nach unten. Das Total der Funktion wird notfalls ueber eine
# reine Source-Kompilierung (.gcno ohne Ausfuehrung) ermittelt.
PENALIZE_NONCOMPILING = True
# Gleiches Prinzip fuer Tests, die zwar kompilieren, aber keine Coverage
# liefern (z.B. Crash vor dem Flush der .gcda).
PENALIZE_COMPILED_NO_COVERAGE = True


def load_config():
    script_dir = Path(__file__).parent
    config_path = script_dir.parent.parent / "config.yaml"

    with open(config_path) as f:
        cfg = yaml.safe_load(f)

    project_root = config_path.parent
    for key, value in cfg["paths"].items():
        p = Path(value)
        if not p.is_absolute():
            cfg["paths"][key] = str(project_root / p)
    return cfg


def find_source_files(src_dir, codebase_name):
    """Findet alle .c-Dateien einer Codebase fuer die Kompilierung."""
    codebase_path = Path(src_dir) / codebase_name
    if VERBOSE:
        print(f"Looking for source files in: {codebase_path}")
    return list(codebase_path.glob("*.c"))


def codebase_for_function(func_name, functions_dir):
    """Ermittelt die Codebase anhand des Ordners, in dem die Funktion liegt."""
    for codebase_dir in Path(functions_dir).iterdir():
        if not codebase_dir.is_dir():
            continue
        for f in codebase_dir.rglob("*.txt"):
            if f.stem == func_name:
                return codebase_dir.name
    return None


def compile_test(test_file, src_files, unity_c, build_dir, gcc_flags, cfg, func_name):
    # Eigenes Unterverzeichnis pro Test
    test_build_dir = (build_dir / f"test_{func_name}").resolve()
    test_build_dir.mkdir(parents=True, exist_ok=True)

    output_binary = test_build_dir / f"test_{func_name}"
    unity_include = Path(cfg["paths"]["unity_dir"]).resolve()
    src_root = Path(cfg["paths"]["src_dir"]).resolve()
    unique_codebase_dirs = {f.parent.resolve() for f in src_files}

    compiler = ["gcc"]
    flags = gcc_flags
    includes = [f"-I{unity_include}", f"-I{src_root}"] + [f"-I{d}" for d in unique_codebase_dirs]
    sources = [str(test_file.resolve()), str(unity_c.resolve())]
    output = ["-o", str(output_binary), "-lm"]

    cmd = compiler + flags + includes + sources + output
    if VERBOSE:
        print(f"Compiling with command: {cmd}")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30, cwd=test_build_dir)
    log = result.stdout + result.stderr
    if VERBOSE and log.strip():
        print(f"Compilation log:\n{log}")
    return result.returncode == 0, log, output_binary, test_build_dir


def run_test(exe, build_dir):
    """Fuehrt das kompilierte Test-Binary aus."""
    result = subprocess.run(
        [str(exe)], capture_output=True, text=True, timeout=30, cwd=str(build_dir)
    )
    return result.returncode, result.stdout + result.stderr


def classify_test_result(returncode, test_output):
    if returncode == 0:
        return "passed"
    if returncode < 0 or not test_output.strip():
        return "runtime_error"
    if "Segmentation fault" in test_output or "SIGSEGV" in test_output:
        return "runtime_error"
    if "FAIL" in test_output or "Expected" in test_output:
        return "assertion_error"
    return "runtime_error"


def classify_compile_error(compile_log):
    """Uebersetzungsfehler-Typ: Linkerfehler vs. Kompilierfehler (FALL A)."""
    if compile_log and "undefined reference" in compile_log:
        return "linker_error"
    return "compile_error"


def count_test_metrics(test_output):
    """Zaehlt Runtime- (Segfaults), Assertion-Fehler, bestandene und ausgefuehrte Tests (FALL B)."""
    metrics = {
        "amount_of_tests": 0,
        "amount_of_tests_passed": 0,
        "amount_of_assertion_errors": 0,
        "amount_of_runtime_errors": 0,
    }
    if not test_output:
        return metrics

    for line in test_output.splitlines():
        if "SIGSEGV" in line or "Segmentation fault" in line:
            metrics["amount_of_runtime_errors"] += 1
        elif ":PASS" in line:
            metrics["amount_of_tests_passed"] += 1
        elif ":FAIL" in line:
            metrics["amount_of_assertion_errors"] += 1

    summary = re.search(r"(\d+)\s+Tests\s+(\d+)\s+Failures\s+(\d+)\s+Ignored", test_output)
    if summary:
        metrics["amount_of_tests"] = int(summary.group(1))
    else:
        metrics["amount_of_tests"] = (
            metrics["amount_of_tests_passed"]
            + metrics["amount_of_assertion_errors"]
            + metrics["amount_of_runtime_errors"]
        )
    return metrics


def measure_coverage(build_dir, func_name):
    """Line- und Branch-Coverage der Funktion via gcov --json-format.

    Rueckgabe: dict mit lines_total/lines_covered/branches_total/branches_covered
    oder None, falls keine Coverage-Daten vorliegen.
    """
    gcda_files = list(build_dir.glob("*.gcda"))
    if not gcda_files:
        return None

    for gcda in gcda_files:
        result = subprocess.run(
            ["gcov", "-b", "--json-format", str(gcda.name)],
            capture_output=True, text=True, cwd=str(build_dir)
        )
        if result.returncode != 0:
            continue

        json_gz = build_dir / f"{gcda.stem}.gcov.json.gz"
        if not json_gz.exists():
            continue

        try:
            with gzip.open(json_gz, "rt", encoding="utf-8") as f:
                data = json.load(f)
        except (OSError, json.JSONDecodeError):
            continue

        cov = _coverage_from_json(data, func_name)
        if cov is not None:
            return cov

    return None


def _coverage_from_json(data, func_name):
    """Extrahiert die rohen Coverage-Zaehler (covered/total) der Zielfunktion."""
    for file_entry in data.get("files", []):
        func_names = {fn["name"] for fn in file_entry.get("functions", [])}
        if func_name not in func_names:
            continue

        lines = [l for l in file_entry.get("lines", [])
                 if l.get("function_name") == func_name]
        if not lines:
            continue

        lines_total = len(lines)
        lines_covered = sum(1 for l in lines if l["count"] > 0)
        branches = [b for l in lines for b in l.get("branches", [])]
        branches_total = len(branches)
        branches_covered = sum(1 for b in branches if b["count"] > 0)

        return {
            "lines_total": lines_total,
            "lines_covered": lines_covered,
            "branches_total": branches_total,
            "branches_covered": branches_covered,
        }

    return None


def measure_function_totals(src_files, func_name, base_dir, gcc_flags, cfg):
    """Statische Totals (lines/branches) einer Funktion ohne Ausfuehrung.

    Kompiliert die Source-Dateien nur zu Objektdateien (-c) mit Coverage-
    Instrumentierung, sodass .gcno-Dateien entstehen. gcov liest daraus die
    ausfuehrbaren Zeilen/Zweige (alle counts = 0). Wird gebraucht, um nicht-
    kompilierende Tests mit 0 Covered / vollem Total zu bestrafen, auch wenn
    KEIN Run der Funktion erfolgreich kompiliert hat.

    Rueckgabe: dict (covered=0, totals gesetzt) oder None.
    """
    totals_dir = (base_dir / f"totals_{func_name}").resolve()
    totals_dir.mkdir(parents=True, exist_ok=True)

    unity_include = Path(cfg["paths"]["unity_dir"]).resolve()
    src_root = Path(cfg["paths"]["src_dir"]).resolve()
    codebase_dirs = {f.parent.resolve() for f in src_files}
    includes = [f"-I{unity_include}", f"-I{src_root}"] + [f"-I{d}" for d in codebase_dirs]

    try:
        for src in src_files:
            obj = totals_dir / f"{src.stem}.o"
            cmd = ["gcc"] + gcc_flags + includes + ["-c", str(src.resolve()), "-o", str(obj)]
            subprocess.run(cmd, capture_output=True, text=True, timeout=30, cwd=totals_dir)

        cov = None
        for gcno in totals_dir.glob("*.gcno"):
            subprocess.run(
                ["gcov", "-b", "--json-format", str(gcno.name)],
                capture_output=True, text=True, cwd=str(totals_dir)
            )
            json_gz = totals_dir / f"{gcno.stem}.gcov.json.gz"
            if not json_gz.exists():
                continue
            try:
                with gzip.open(json_gz, "rt", encoding="utf-8") as f:
                    data = json.load(f)
            except (OSError, json.JSONDecodeError):
                continue
            cov = _coverage_from_json(data, func_name)
            if cov is not None:
                break
        return cov
    except subprocess.TimeoutExpired:
        return None
    finally:
        shutil.rmtree(totals_dir, ignore_errors=True)


def apply_coverage_penalty(func_metrics, src_files, func_name, build_dir, gcc_flags, cfg):
    """Rechnet Runs ohne Coverage-Daten als 0%-Strafe ein.

    Ein nicht-kompilierender (oder kompilierender, aber coverage-loser) Test
    bekommt lines_covered=0 / branches_covered=0 bei vollem Total der Funktion,
    statt aus dem Micro-Average ausgeschlossen zu werden. Dadurch zaehlt er als
    0 % und senkt die Gesamt-Coverage.
    """
    # Baseline-Total ermitteln: zuerst aus einem messbaren Run, sonst per .gcno.
    base = None
    for r in func_metrics:
        if r["lines_total"] is not None:
            base = {"lines_total": r["lines_total"],
                    "branches_total": r["branches_total"]}
            break
    if base is None and any(r["lines_total"] is None for r in func_metrics):
        bt = measure_function_totals(src_files, func_name, build_dir, gcc_flags, cfg)
        if bt is not None:
            base = {"lines_total": bt["lines_total"],
                    "branches_total": bt["branches_total"]}

    for r in func_metrics:
        if r["lines_total"] is not None:
            r["coverage_source"] = "measured"
            continue

        penalize = (PENALIZE_NONCOMPILING if not r["compiled"]
                    else PENALIZE_COMPILED_NO_COVERAGE)
        if penalize and base is not None:
            r["lines_total"] = base["lines_total"]
            r["lines_covered"] = 0
            r["branches_total"] = base["branches_total"]
            r["branches_covered"] = 0
            r["line_coverage_pct"] = micro_pct(0, base["lines_total"])
            r["branch_coverage_pct"] = micro_pct(0, base["branches_total"])
            r["coverage_source"] = ("penalty_no_compile" if not r["compiled"]
                                    else "penalty_no_coverage")
        else:
            # Kann nicht bestraft werden (keine Totals ermittelbar) -> ausgeschlossen.
            r["coverage_source"] = "unavailable"


def clean_build(build_dir):
    if build_dir.exists():
        shutil.rmtree(build_dir)


# ---------------------------------------------------------------------------
# Aggregation / Micro-Average
# ---------------------------------------------------------------------------

def micro_pct(covered, total):
    """Micro-Average-Prozentwert: 100 * (Summe covered) / (Summe total)."""
    if not total:
        return None
    return round(100 * covered / total, 2)


def summarize_runs(run_results):
    """Summiert die Rohzaehler ueber eine Menge von Runs und bildet daraus
    den Micro-Average fuer Line- und Branch-Coverage.

    Micro-Average heisst: erst alle covered/total ueber alle Runs aufsummieren
    (Doppelsumme ueber Funktionen j und Runs i), dann das Verhaeltnis bilden.
    """
    s = {
        # Tests
        "amount_of_tests": 0,
        "amount_of_tests_passed": 0,
        # Fehler
        "amount_of_compile_errors": 0,
        "amount_of_linker_errors": 0,
        "amount_of_runtime_errors": 0,
        "amount_of_assertion_errors": 0,
        # Coverage (Rohzaehler fuer Micro-Average)
        "lines_total": 0,
        "lines_covered": 0,
        "branches_total": 0,
        "branches_covered": 0,
        # Run-Buchhaltung
        "runs_total": len(run_results),
        "runs_all_passed": 0,
    }

    for r in run_results:
        s["amount_of_tests"] += r["amount_of_tests"]
        s["amount_of_tests_passed"] += r["amount_of_tests_passed"]
        s["amount_of_runtime_errors"] += r["amount_of_runtime_errors"]
        s["amount_of_assertion_errors"] += r["amount_of_assertion_errors"]

        if r["translation_error_type"] == "compile_error":
            s["amount_of_compile_errors"] += 1
        elif r["translation_error_type"] == "linker_error":
            s["amount_of_linker_errors"] += 1

        if r["all_tests_passed"]:
            s["runs_all_passed"] += 1

        if r.get("lines_total") is not None:
            s["lines_total"] += r["lines_total"]
            s["lines_covered"] += r["lines_covered"]
            s["branches_total"] += r["branches_total"]
            s["branches_covered"] += r["branches_covered"]

    s["line_coverage_micro"] = micro_pct(s["lines_covered"], s["lines_total"])
    s["branch_coverage_micro"] = micro_pct(s["branches_covered"], s["branches_total"])
    return s


def summarize_function(func_name, run_results):
    """Funktions-Zusammenfassung inkl. 'All Test passed' (alle Runs der Funktion bestanden)."""
    summary = summarize_runs(run_results)
    summary["function"] = func_name
    summary["all_tests_passed"] = (
        summary["runs_total"] > 0
        and summary["runs_all_passed"] == summary["runs_total"]
    )
    return summary


def summarize_model(model_name, all_metrics, function_summaries):
    """Modell-Zusammenfassung: Micro-Average ueber ALLE Runs aller Funktionen."""
    all_runs = [r for runs in all_metrics.values() for r in runs]
    summary = summarize_runs(all_runs)
    summary["model"] = model_name
    summary["amount_of_functions"] = len(function_summaries)
    # 'Amount of all Tests passed' = Anzahl der Funktionen, in denen alle Tests durchliefen
    summary["amount_of_all_tests_passed"] = sum(
        1 for fs in function_summaries if fs["all_tests_passed"]
    )
    return summary


# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------

def _fmt_cov(covered, total, pct):
    pct_str = f"{pct:.2f} %" if pct is not None else "n/a"
    return f"covered {covered} / total {total}  ->  {pct_str}"


def print_run_line(model_name, func_name, r):
    cov = r.get("line_coverage_pct")
    cov_str = f"{cov:.2f}%" if cov is not None else "n/a"
    if r["compiled"]:
        print(
            f"  [OK]   {model_name}/{func_name}/{r['run']} | "
            f"status: {r['test_status']} | exit: {r['return_code']} | "
            f"tests: {r['amount_of_tests_passed']}/{r['amount_of_tests']} passed | "
            f"line_cov: {cov_str}"
        )
    else:
        cov = r.get("line_coverage_pct")
        cov_str = f"{cov:.2f}%" if cov is not None else "n/a"
        print(
            f"  [FAIL] {model_name}/{func_name}/{r['run']} | "
            f"translation_error: {r['translation_error_type']} | "
            f"line_cov: {cov_str} ({r.get('coverage_source')})"
        )


def print_model_log(summary):
    bar = "=" * 70
    print(f"\n{bar}")
    print(f" MODELL: {summary['model']}")
    print(bar)
    print(f" Amount of Functions:          {summary['amount_of_functions']}")
    print(f" Amount of all Tests passed:   {summary['amount_of_all_tests_passed']}"
          f"  (Funktionen mit komplett bestandenen Tests)")

    print("\n -- Tests --")
    print(f" Amount of Tests:              {summary['amount_of_tests']}")
    print(f" Amount of Tests passed:       {summary['amount_of_tests_passed']}")

    print("\n -- Fehler --")
    print(f" Amount of Compile Errors:     {summary['amount_of_compile_errors']}")
    print(f" Amount of Linker Errors:      {summary['amount_of_linker_errors']}")
    print(f" Amount of Runtime Errors:     {summary['amount_of_runtime_errors']}")
    print(f" Amount of Assertion Errors:   {summary['amount_of_assertion_errors']}")

    print("\n -- Coverage (Micro-Average) --")
    print(f" Lines:    {_fmt_cov(summary['lines_covered'], summary['lines_total'], summary['line_coverage_micro'])}")
    print(f" Branches: {_fmt_cov(summary['branches_covered'], summary['branches_total'], summary['branch_coverage_micro'])}")
    print(f"{bar}\n")


# ---------------------------------------------------------------------------
# Hauptlauf
# ---------------------------------------------------------------------------

def run_model(model_name, cfg):
    """Kompiliert + fuehrt alle Tests eines Modells aus und schreibt die Rohmetriken.

    Erzeugt pro Modell eine raw_metrics.json mit:
      - all_metrics:          {func_name: [run_result, ...]}   (Eingabe fuer 02_compute_metrics.py)
      - function_summaries:   [funktions-zusammenfassung, ...]
      - summary:              Modell-Zusammenfassung inkl. Micro-Average-Coverage
    """
    tests_base = Path(cfg["paths"]["output_tests"]) / f"{model_name}_modell"
    metrics_dir = Path(cfg["paths"]["output_metrics"]) / f"{model_name}_modell"
    metrics_dir.mkdir(parents=True, exist_ok=True)

    build_dir = Path(cfg["paths"]["build_dir"]) / f"{model_name}_modell"
    build_dir.mkdir(parents=True, exist_ok=True)

    unity_c = Path(cfg["paths"]["unity_dir"]) / "unity.c"
    gcc_flags = cfg["experiment"]["gcc_flags"]

    all_metrics = {}
    function_summaries = []

    if not tests_base.exists():
        print(f"  Keine Tests gefunden fuer {model_name}")
        return None

    for func_dir in sorted(tests_base.iterdir()):
        if not func_dir.is_dir():
            continue
        func_name = func_dir.name
        codebase = codebase_for_function(func_name, cfg["paths"]["functions_dir"])
        if not codebase:
            print(f"  [WARN] Keine Codebase fuer {func_name}")
            continue

        src_files = find_source_files(cfg["paths"]["src_dir"], codebase)
        func_metrics = []

        for test_file in sorted(func_dir.glob("run_*_test.c")):
            run_name = test_file.stem

            compiled, compile_log, exe, test_build_dir = compile_test(
                test_file, src_files, unity_c, build_dir, gcc_flags, cfg, f"{func_name}-{run_name}"
            )

            run_result = {
                "run": run_name,
                "codebase": codebase,
                "compiled": compiled,
                "test_status": None,
                # --- Tests ---
                "amount_of_tests": 0,           # Amount of Tests
                "amount_of_tests_passed": 0,    # Amount of Tests passed
                "all_tests_passed": False,      # All Test passed (alle Tests dieses Runs bestanden)
                # --- Fehler ---
                "translation_error_type": None,  # Translation Error Type: compile_error | linker_error | None
                "amount_of_runtime_errors": 0,   # Amount of Runtime Errors
                "amount_of_assertion_errors": 0,  # Amount of Assertion Errors
                # --- Ausgabe ---
                "return_code": None,            # Return Code
                "test_output": None,            # Test Output
                "error_message": "",            # Error Message (leer, falls kein Uebersetzungsfehler)
                # --- Coverage (Rohzaehler) ---
                "lines_total": None,            # Lines Total
                "lines_covered": None,          # Lines Covered
                "branches_total": None,         # Branches Total
                "branches_covered": None,       # Branches Covered
                "line_coverage_pct": None,      # bequemer Einzelwert (nur Info)
                "branch_coverage_pct": None,
                "coverage_source": None,        # measured | penalty_no_compile | penalty_no_coverage | unavailable
            }

            if compiled:
                try:
                    returncode, test_output = run_test(exe, test_build_dir)
                    cov = measure_coverage(test_build_dir, func_name)

                    run_result["return_code"] = returncode
                    run_result["test_output"] = test_output
                    run_result["test_status"] = classify_test_result(returncode, test_output)
                    run_result["all_tests_passed"] = run_result["test_status"] == "passed"
                    run_result.update(count_test_metrics(test_output))

                    # Falls als Runtime-Error klassifiziert, aber keine Segfault-Zeile
                    # gezaehlt wurde (z.B. Abbruch ohne Output): mindestens 1 zaehlen.
                    if (run_result["test_status"] == "runtime_error"
                            and run_result["amount_of_runtime_errors"] == 0):
                        run_result["amount_of_runtime_errors"] = 1

                    if cov is not None:
                        run_result["lines_total"] = cov["lines_total"]
                        run_result["lines_covered"] = cov["lines_covered"]
                        run_result["branches_total"] = cov["branches_total"]
                        run_result["branches_covered"] = cov["branches_covered"]
                        run_result["line_coverage_pct"] = micro_pct(
                            cov["lines_covered"], cov["lines_total"])
                        run_result["branch_coverage_pct"] = micro_pct(
                            cov["branches_covered"], cov["branches_total"])

                    if VERBOSE:
                        print(f"   Test output:\n{test_output}")
                except subprocess.TimeoutExpired:
                    run_result["test_status"] = "timeout"
                    run_result["error_message"] = "timeout"
                except Exception:
                    run_result["test_status"] = "error"
                    run_result["error_message"] = traceback.format_exc()
            else:
                # FALL A: Uebersetzungsfehler – Linker- vs. Kompilierfehler.
                run_result["test_status"] = "compile_error"
                run_result["translation_error_type"] = classify_compile_error(compile_log)
                run_result["error_message"] = compile_log.strip()
                if VERBOSE:
                    print(compile_log)

            # Aufraeumen: .gcda und .gcno entfernen
            if test_build_dir.exists():
                for f in test_build_dir.glob("*.gcda"):
                    f.unlink()
                for f in test_build_dir.glob("*.gcno"):
                    f.unlink()

            func_metrics.append(run_result)

        # Nicht-kompilierende / coverage-lose Runs als 0%-Strafe einrechnen,
        # bevor geloggt und aggregiert wird.
        apply_coverage_penalty(func_metrics, src_files, func_name, build_dir, gcc_flags, cfg)
        for run_result in func_metrics:
            print_run_line(model_name, func_name, run_result)

        all_metrics[func_name] = func_metrics
        func_summary = summarize_function(func_name, func_metrics)
        function_summaries.append(func_summary)

    model_summary = summarize_model(model_name, all_metrics, function_summaries)

    raw_report = {
        "model": model_name,
        "summary": model_summary,
        "function_summaries": function_summaries,
        "all_metrics": all_metrics,
    }
    raw_file = metrics_dir / "raw_metrics.json"
    raw_file.write_text(json.dumps(raw_report, indent=2))

    print_model_log(model_summary)
    print(f"  -> Rohmetriken: {raw_file}")

    return raw_report


def main():
    import sys
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "deepseek", "qwen"]

    for model in models:
        print(f"\n=== Kompiliere & fuehre aus: {model} ===")
        run_model(model, cfg)


if __name__ == "__main__":
    main()
