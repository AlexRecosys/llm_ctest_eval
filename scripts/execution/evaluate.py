"""Kompiliert generierte Tests und misst Coverage via gcov."""

import subprocess
import json
import re
import shutil
from pathlib import Path
import yaml
import traceback
import subprocess
import gzip


def load_config():

    script_dir = Path(__file__).parent
    config_path = script_dir.parent.parent / "config.yaml"
    
    with open(config_path) as f:
        cfg =  yaml.safe_load(f)
    
    project_root = config_path.parent

    for key, value in cfg["paths"].items():
        p = Path(value)
        if not p.is_absolute():
            cfg["paths"][key] = str(project_root / p)
    return cfg


def find_source_files(src_dir, codebase_name):
    """Findet alle .c-Dateien einer Codebase für die Kompilierung."""
    codebase_path = Path(src_dir) / codebase_name
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
    flags    = gcc_flags
    includes = [f"-I{unity_include}", f"-I{src_root}"] + [f"-I{d}" for d in unique_codebase_dirs]
    sources  = [str(test_file.resolve()), str(unity_c.resolve())]
    output   = ["-o", str(output_binary), "-lm"]

    cmd = compiler + flags + includes + sources + output
    print(f"Compiling with command: {cmd}")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30, cwd=test_build_dir)
    log = result.stdout + result.stderr
    print(f"Compilation log:\n{log}")
    return result.returncode == 0, log, output_binary, test_build_dir

def run_test(exe, build_dir):
    """Führt das kompilierte Test-Binary aus."""
    result = subprocess.run(
        [str(exe)], capture_output=True, text=True, timeout=30, cwd=str(build_dir)
    )
    return result.returncode, result.stdout + result.stderr


def classify_test_result(returncode, test_output):
    if returncode == 0:
        return "passed"
    # Unkontrollierter Crash (Signal), kein Unity-Output
    if returncode < 0 or not test_output.strip():
        return "runtime_error"
    # Unity hat Segfault abgefangen – steht im Output
    if "Segmentation fault" in test_output or "SIGSEGV" in test_output:
        return "runtime_error"
    # Normale fehlgeschlagene Assertions
    if "FAIL" in test_output or "Expected" in test_output:
        return "assertion_error"
    return "runtime_error"  # Fallback


def classify_compile_error(compile_log):
    """Unterscheidet Linker-Fehler von einfachen Compile-Fehlern (FALL A).

    String-Check auf 'undefined reference' im Compile-Log. Erster Fund stoppt
    die Suche (Python's `in` bricht beim ersten Treffer ab).
    """
    if compile_log and "undefined reference" in compile_log:
        return "linker_error"
    return "basic_compile_error"


def count_test_metrics(test_output):
    """Zählt Segfaults, Assertion-Fehler, bestandene und ausgeführte Tests (FALL B).

    Wird auf den Unity-Output angewandt, wann immer einer vorhanden ist.
    Liegt kein Output vor (z. B. Compile-Fehler oder harter Crash ohne Ausgabe),
    sind die Werte nicht ermittelbar und bleiben auf dem Default 0.
    """
    metrics = {
        "segfaults_counter": 0,
        "assertion_errors_counter": 0,
        "tests_passed_counter": 0,
        "amount_of_tests": 0,
    }
    if not test_output:
        return metrics

    for line in test_output.splitlines():
        # Segfault-Markierung hat Vorrang: solche Zeilen enthalten zwar ':FAIL',
        # zählen aber als Segfault und nicht als normale Assertion.
        if "SIGSEGV" in line or "Segmentation fault" in line:
            metrics["segfaults_counter"] += 1
        elif ":PASS" in line:
            metrics["tests_passed_counter"] += 1
        elif ":FAIL" in line:
            metrics["assertion_errors_counter"] += 1

    # Gesamtzahl bevorzugt aus der Unity-Summary "N Tests M Failures K Ignored"
    summary = re.search(r"(\d+)\s+Tests\s+(\d+)\s+Failures\s+(\d+)\s+Ignored", test_output)
    if summary:
        metrics["amount_of_tests"] = int(summary.group(1))
    else:
        metrics["amount_of_tests"] = (
            metrics["tests_passed_counter"]
            + metrics["assertion_errors_counter"]
            + metrics["segfaults_counter"]
        )

    return metrics


def parse_function_coverage(gcov_text, func_name):
    """Sucht im gcov-Output den 'Function <name>'-Block und liest dessen Coverage."""
    lines = gcov_text.splitlines()
    func_header = f"Function '{func_name}'"
 
    start = next((i for i, line in enumerate(lines) if func_header in line), None)
    if start is None:
        return None, None
 
    end = start + 1
    while end < len(lines) and not (
        lines[end].startswith("Function '") or lines[end].startswith("File '")
    ):
        end += 1
 
    block = "\n".join(lines[start:end])
    line_cov   = _extract_metric(block, r"Lines executed:(\d+\.\d+)% of (\d+)")
    branch_cov = _extract_metric(block, r"Branches executed:(\d+\.\d+)% of (\d+)")
    return line_cov, branch_cov
 
 
def _extract_metric(text, pattern):
    """Extrahiert percent/total/covered aus einer gcov-Zeile."""
    m = re.search(pattern, text)
    if not m:
        return None
    percent = float(m.group(1))
    total   = int(m.group(2))
    covered = round(percent / 100 * total)
    return {"percent": percent, "total": total, "covered": covered}


def measure_coverage(build_dir, func_name):
    """Line- und Branch-Coverage der Funktion via gcov --json-format."""
    gcda_files = list(build_dir.glob("*.gcda"))
    if not gcda_files:
        return None, None

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

    return None, None

def _coverage_from_json(data, func_name):
    for file_entry in data.get("files", []):
        func_names = {fn["name"] for fn in file_entry.get("functions", [])}
        if func_name not in func_names:
            continue

        lines = [l for l in file_entry.get("lines", [])
                 if l.get("function_name") == func_name]
        if not lines:
            continue

        total_lines   = len(lines)
        covered_lines = sum(1 for l in lines if l["count"] > 0)
        branches      = [b for l in lines for b in l.get("branches", [])]
        total_br      = len(branches)
        covered_br    = sum(1 for b in branches if b["count"] > 0)

        def metric(covered, total):
            if total == 0:
                return None
            return {"percent": round(100 * covered / total, 2),
                    "total": total, "covered": covered}

        return metric(covered_lines, total_lines), metric(covered_br, total_br)

    return None

def clean_build(build_dir):
    if build_dir.exists():
        shutil.rmtree(build_dir)


RUN_ORDER = ["run_01_test", "run_02_test", "run_03_test"]


def _pct(num, den):
    """Prozent oder None, wenn der Nenner 0 ist (vermeidet Division durch 0)."""
    return round(100 * num / den, 2) if den else None


def _norm_run(run):
    """Normalisiert einen einzelnen Lauf auf zählbare Test-Case-Metriken (Fix #1, #2).

    Invariante für kompilierte Läufe: passed + assertion_errors + runtime_errors == tests.
    Harter Crash ohne verwertbaren Unity-Output (compiled==True, kein Output,
    amount_of_tests==0): zählt als genau 1 Test, der als Runtime-Fehler gewertet
    wird – damit bleibt der Lauf im Nenner statt zu verschwinden.
    """
    compiled = bool(run.get("compiled"))
    status = run.get("test_status")
    cet = run.get("compile_error_type")

    out = {
        "compiled": compiled,
        "compile_error": not compiled,
        "basic_compile_error": (not compiled) and cet == "basic_compile_error",
        "linker_error": (not compiled) and cet == "linker_error",
        "tests": 0,
        "passed": 0,
        "assertion_errors": 0,
        "runtime_errors": 0,
    }
    if not compiled:
        return out

    amount    = run.get("amount_of_tests", 0) or 0
    passed    = run.get("tests_passed_counter", 0) or 0
    assertion = run.get("assertion_errors_counter", 0) or 0
    runtime   = run.get("segfaults_counter", 0) or 0  # Unity-gefangene Segfaults

    if amount == 0 and status != "passed":
        # Harter Crash / Timeout ohne Unity-Output: als 1 fehlgeschlagenen
        # (Runtime-)Test werten, damit er im Nenner bleibt (Fix #2).
        passed, assertion, runtime, amount = 0, 0, 1, 1

    out.update(tests=amount, passed=passed,
               assertion_errors=assertion, runtime_errors=runtime)
    return out


def _rate_bundle(norm_runs):
    """Aggregiert normalisierte Läufe zu Fehler-/Pass-Raten.

    Nenner-Konventionen (laut Vorgabe):
      - Compile Error Rate : über ALLE FUT-Läufe der Gruppe.
      - Linker Error Rate  : über alle bis zum Objektcode kompilierten FUT-Läufe
                             (= erfolgreich gelinkte + Linker-Fehler; ein
                             Basic-Compile-Fehler erreicht das Linken nie).
      - Runtime/Assert/Pass: über die Tests der erfolgreich kompilierten FUT-Läufe.
    """
    n = len(norm_runs)
    basic   = sum(1 for u in norm_runs if u["basic_compile_error"])
    linker  = sum(1 for u in norm_runs if u["linker_error"])
    compile_err     = basic + linker
    compiled_ok     = sum(1 for u in norm_runs if u["compiled"])
    compiled_to_obj = compiled_ok + linker  # Nenner für Linker-Rate

    tests     = sum(u["tests"]            for u in norm_runs if u["compiled"])
    passed    = sum(u["passed"]           for u in norm_runs if u["compiled"])
    assertion = sum(u["assertion_errors"] for u in norm_runs if u["compiled"])
    runtime   = sum(u["runtime_errors"]   for u in norm_runs if u["compiled"])

    return {
        "fut_runs": n,
        "compiled_ok": compiled_ok,
        "compiled_to_object": compiled_to_obj,
        "compile_errors": compile_err,
        "basic_compile_errors": basic,
        "linker_errors": linker,
        "tests_in_compiled": tests,
        "passed": passed,
        "assertion_errors": assertion,
        "runtime_errors": runtime,
        "compile_error_rate": _pct(compile_err, n),
        "linker_error_rate": _pct(linker, compiled_to_obj),
        "runtime_error_rate": _pct(runtime, tests),
        "assert_error_rate": _pct(assertion, tests),
        "pass_rate": _pct(passed, tests),
    }


def _mean_cov(cov_list):
    """Mittelwert über die vorhandenen Coverage-Werte der Läufe einer FUT (Fix #3)."""
    vals = [c for c in cov_list if c]
    if not vals:
        return None
    n = len(vals)
    covered = sum(c["covered"] for c in vals) / n
    total   = sum(c["total"]   for c in vals) / n
    return {
        "percent": round(100 * covered / total, 2) if total else 0.0,
        "covered": round(covered, 2),
        "total":   round(total, 2),
        "runs_with_data": n,
    }


def _sum_cov(cov_list):
    """Summiert covered/total über FUTs (oder Läufe) zu einer Quote.

    Für Codebase-/Modell-Coverage werden die FUT-MITTELWERTE summiert, NICHT
    drei Läufe pro FUT (kein Triple-Counting der Quellzeilen mehr).
    """
    vals = [c for c in cov_list if c]
    if not vals:
        return None
    covered = sum(c["covered"] for c in vals)
    total   = sum(c["total"]   for c in vals)
    return {
        "percent": round(100 * covered / total, 2) if total else 0.0,
        "covered": round(covered, 2),
        "total":   round(total, 2),
        "n": len(vals),
    }


def _mean_of_bundles(bundles):
    """Mittelwert der Raten über die Läufe (Vergleichsbasis, parallel zur Coverage)."""
    rate_keys = ["compile_error_rate", "linker_error_rate", "runtime_error_rate",
                 "assert_error_rate", "pass_rate"]
    out = {}
    for k in rate_keys:
        vals = [b[k] for b in bundles if b.get(k) is not None]
        out[k] = round(sum(vals) / len(vals), 2) if vals else None
    for ck in ["line_coverage", "branch_coverage"]:
        pcts = [b[ck]["percent"] for b in bundles if b.get(ck)]
        out[ck + "_percent"] = round(sum(pcts) / len(pcts), 2) if pcts else None
    return out





def _aggregate(fut_names, functions, all_metrics):
    """Aggregiert eine Menge von FUTs zu Codebase- bzw. Modell-Kennzahlen.

    Liefert: Coverage (Summe der FUT-Mittelwerte), das Raten-Aggregat über alle
    FUT-Läufe, eine Pro-Lauf-Aufschlüsselung und den Mittelwert über die Läufe.
    """
    all_norm = []
    per_run_norm   = {ri: [] for ri in RUN_ORDER}
    per_run_line   = {ri: [] for ri in RUN_ORDER}
    per_run_branch = {ri: [] for ri in RUN_ORDER}
    line_means, branch_means, no_cov = [], [], []

    for fut in fut_names:
        fmean_line   = functions[fut]["mean"]["line_coverage"]
        fmean_branch = functions[fut]["mean"]["branch_coverage"]
        if fmean_line:
            line_means.append(fmean_line)
        else:
            no_cov.append(fut)
        if fmean_branch:
            branch_means.append(fmean_branch)

        for r in all_metrics[fut]:
            u = _norm_run(r)
            all_norm.append(u)
            ri = r.get("run")
            if ri in per_run_norm:
                per_run_norm[ri].append(u)
                if r.get("line_coverage"):
                    per_run_line[ri].append(r["line_coverage"])
                if r.get("branch_coverage"):
                    per_run_branch[ri].append(r["branch_coverage"])

    per_run = {}
    for ri in RUN_ORDER:
        if not per_run_norm[ri]:
            continue
        b = _rate_bundle(per_run_norm[ri])
        b["line_coverage"]   = _sum_cov(per_run_line[ri])
        b["branch_coverage"] = _sum_cov(per_run_branch[ri])
        per_run[ri] = b

    return {
        "line_coverage":   _sum_cov(line_means),     # Summe der FUT-Mittelwerte
        "branch_coverage": _sum_cov(branch_means),
        "futs_without_coverage": no_cov,
        "aggregate_over_all_fut_runs": _rate_bundle(all_norm),
        "per_run": per_run,
        "mean_over_runs": _mean_of_bundles([per_run[ri] for ri in RUN_ORDER if ri in per_run]),
    }


def build_model_report(model_name, all_metrics):
    """Erzeugt den vollständigen Metrik-Report eines Modells.

    Ebenen: pro FUT (mit jedem Lauf einzeln + Mittelwert als Vergleichsbasis),
    pro Codebase und pro Modell.
    """
    functions = {}
    for fut, runs in all_metrics.items():
        codebase = runs[0].get("codebase") if runs else "unknown"
        norm = [_norm_run(r) for r in runs]

        run_views = []
        for r, u in zip(runs, norm):
            tests = u["tests"]
            run_views.append({
                "run": r.get("run"),
                "compiled": u["compiled"],
                "compile_error_type": r.get("compile_error_type"),
                "test_status": r.get("test_status"),
                "line_coverage": r.get("line_coverage"),      # dieser Lauf einzeln
                "branch_coverage": r.get("branch_coverage"),  # dieser Lauf einzeln
                "tests": tests,
                "passed": u["passed"],
                "assertion_errors": u["assertion_errors"],
                "runtime_errors": u["runtime_errors"],
                # "pass_rate":          _pct(u["passed"], tests) if u["compiled"] else None,
                # "assert_error_rate":  _pct(u["assertion_errors"], tests) if u["compiled"] else None,
                # "runtime_error_rate": _pct(u["runtime_errors"], tests) if u["compiled"] else None,
            })

        mean = {
            "line_coverage":   _mean_cov([r.get("line_coverage")   for r in runs]),
            "branch_coverage": _mean_cov([r.get("branch_coverage") for r in runs]),
        }
        mean.update(_rate_bundle(norm))  # Pass-/Fehler-Raten der FUT über ihre Läufe

        functions[fut] = {
            "codebase": codebase,
            "runs": run_views,
            "mean": mean,
        }

    by_cb = {}
    for fut, data in functions.items():
        by_cb.setdefault(data["codebase"], []).append(fut)

    codebase_summary = {
        cb: _aggregate(futs, functions, all_metrics) for cb, futs in by_cb.items()
    }
    model_summary = _aggregate(list(functions.keys()), functions, all_metrics)

    return {
        "model": model_name,
        "functions": functions,
        "codebase_summary": codebase_summary,
        "model_summary": model_summary,
    }


def process_model(model_name, cfg):
    """Verarbeitet alle Tests eines Modells."""
    tests_base = Path(cfg["paths"]["output_tests"]) / f"{model_name}_modell"
    metrics_dir = Path(cfg["paths"]["output_metrics"]) / f"{model_name}_modell"
    metrics_dir.mkdir(parents=True, exist_ok=True)

    build_dir = Path(cfg["paths"]["build_dir"]) / f"{model_name}_modell"
    build_dir.mkdir(parents=True, exist_ok=True)
    
    unity_c = Path(cfg["paths"]["unity_dir"]) / "unity.c"
    gcc_flags = cfg["experiment"]["gcc_flags"]

    all_metrics = {}

    if not tests_base.exists():
        print(f"  Keine Tests gefunden für {model_name}")
        return

    for func_dir in sorted(tests_base.iterdir()):
        if not func_dir.is_dir():
            continue
        func_name = func_dir.name
        codebase = codebase_for_function(func_name, cfg["paths"]["functions_dir"])
        print(f'CODEBASE: {codebase}')
        print(f'FUNC_DIR: {func_dir}')
        print(f'FUNC_NAME: {func_name}')
        if not codebase:
            print(f"  [WARN] Keine Codebase für {func_name}")
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
                "returncode": None,
                "all_passed": False,
                "test_status": None,
                "compile_error_type": None,
                "assertion_errors_counter": 0,
                "segfaults_counter": 0,
                "tests_passed_counter": 0,
                "amount_of_tests": 0,
                "test_output": None,
                "line_coverage": None,
                "branch_coverage": None,
            }

            if compiled:
                try:
                    returncode, test_output = run_test(exe, test_build_dir)
                    line_cov, branch_cov = measure_coverage(test_build_dir, func_name)

                    run_result["returncode"]      = returncode
                    run_result["test_output"]     = test_output
                    run_result["test_status"]     = classify_test_result(returncode, test_output)
                    run_result["line_coverage"]   = line_cov
                    run_result["branch_coverage"] = branch_cov
                    run_result["all_passed"]      = run_result["test_status"] == "passed"

                    # FALL B: Fehler-/Test-Metriken aus dem Unity-Output zählen.
                    # Wird auf jeden vorhandenen Output angewandt; erfolgreiche Läufe
                    # liefern so korrekte Werte, statt nur Defaults.
                    run_result.update(count_test_metrics(test_output))

                    print(f"   Run {run_name} | status: {run_result['test_status']} (exit {returncode})")
                    print(f"   Test output:\n{test_output}")
                except subprocess.TimeoutExpired:
                    run_result["test_status"] = "timeout"
                    run_result["error"] = "timeout"
                except Exception:
                    error_stack = traceback.format_exc()
                    run_result["test_status"] = "error"
                    run_result["error"] = error_stack
            else:
                # FALL A: Compile-Fehler – Linker- vs. einfacher Compile-Fehler.
                run_result["test_status"] = "compile_error"
                run_result["compile_error_type"] = classify_compile_error(compile_log)
                run_result["error"] = f"Compilation failed: {compile_log.strip()}"
                print(compile_log)

            # Aufräumen: .gcda und .gcno entfernen
            if test_build_dir.exists():
                for f in test_build_dir.glob("*.gcda"):
                    f.unlink()
                for f in test_build_dir.glob("*.gcno"):
                    f.unlink()

            error_msg = ""
            if run_result.get("error"):
                lines = [line for line in run_result["error"].strip().splitlines() if line.strip()]
                error_msg = f" | Msg: {lines[-1]}" if lines else " | Msg: Unknown error"

            func_metrics.append(run_result)
            status = "OK" if compiled else "FAIL"
            cov = run_result["line_coverage"]
            cov_str = f"{cov['percent']}%" if cov else "n/a"
            print(f"  [{status}] {model_name}/{func_name}/{run_name} -> line_cov: {cov_str} | test_status: {run_result['test_status']}{error_msg}")

        all_metrics[func_name] = func_metrics

    metrics_file = metrics_dir / "model_metric_function.json"
    metrics_file.write_text(json.dumps(all_metrics, indent=2))
    print(f"  -> Metriken: {metrics_file}")

    # Vollständiger Report: pro FUT (Läufe einzeln + Mittelwert), pro Codebase, pro Modell.
    report = build_model_report(model_name, all_metrics)
    summary_file = metrics_dir / "model_metric_summary.json"
    summary_file.write_text(json.dumps(report, indent=2))
    print(f"  -> Zusammenfassung: {summary_file}")

    return report


def main():
    import sys
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "deepseek", "qwen"]

    per_model = {}

    for model in models:
        print(f"\n=== Kompiliere & messe {model} ===")
        report = process_model(model, cfg)
        if not report:
            continue
        # Für den Modellvergleich nur die aggregierten Ebenen sammeln –
        # Raten/Coverage werden NICHT modellübergreifend zusammengezählt
        # (das wäre inhaltlich sinnlos), sondern nebeneinandergestellt.
        per_model[model] = {
            "model_summary": report["model_summary"],
            "codebase_summary": report["codebase_summary"],
        }

    global_summary = {
        "models": list(per_model.keys()),
        "per_model": per_model,
    }
    out_dir = Path(cfg["paths"]["output_metrics"])
    out_dir.mkdir(parents=True, exist_ok=True)
    global_file = out_dir / "global_metric_summary.json"
    global_file.write_text(json.dumps(global_summary, indent=2))
    print(f"\n-> Gesamt-Zusammenfassung über alle Modelle: {global_file}")


if __name__ == "__main__":
    main()
