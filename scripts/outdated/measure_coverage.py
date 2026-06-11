import subprocess
import json
import re
import shutil
from pathlib import Path
import yaml
import traceback
import gzip


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
    print(f"Compiling with command: {cmd}")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30, cwd=test_build_dir)
    log = result.stdout + result.stderr
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
    """Linker-Fehler vs. einfacher Compile-Fehler (FALL A)."""
    if compile_log and "undefined reference" in compile_log:
        return "linker_error"
    return "basic_compile_error"


def count_test_metrics(test_output):
    """Zaehlt Segfaults, Assertion-Fehler, bestandene und ausgefuehrte Tests (FALL B)."""
    metrics = {
        "segfaults_counter": 0,
        "assertion_errors_counter": 0,
        "tests_passed_counter": 0,
        "amount_of_tests": 0,
    }
    if not test_output:
        return metrics

    for line in test_output.splitlines():
        if "SIGSEGV" in line or "Segmentation fault" in line:
            metrics["segfaults_counter"] += 1
        elif ":PASS" in line:
            metrics["tests_passed_counter"] += 1
        elif ":FAIL" in line:
            metrics["assertion_errors_counter"] += 1

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

        total_lines = len(lines)
        covered_lines = sum(1 for l in lines if l["count"] > 0)
        branches = [b for l in lines for b in l.get("branches", [])]
        total_br = len(branches)
        covered_br = sum(1 for b in branches if b["count"] > 0)

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

COUNT_KEYS = ["compile_errors", "linker_errors", "runtime_errors",
              "assertion_errors", "passed", "tests"]
RATE_KEYS = ["compile_error_rate", "linker_error_rate", "runtime_error_rate",
             "assert_error_rate", "pass_rate"]


def _pct(num, den):
    """Prozent oder None, wenn der Nenner 0 ist."""
    return round(100 * num / den, 2) if den else None


def _norm(r):
    """Ein Rohlauf -> normalisierte, zaehlbare Kennzahlen fuer die Aggregation.

    Harter Crash/Timeout ohne verwertbaren Unity-Output (kompiliert, aber 0 Tests
    und nicht 'passed') zaehlt als genau 1 Runtime-Fehler-Test, damit der Lauf im
    Nenner bleibt statt zu verschwinden.
    """
    compiled = bool(r.get("compiled"))
    cet = r.get("compile_error_type")
    seg = r.get("segfaults_counter", 0) or 0
    tests = r.get("amount_of_tests", 0) or 0
    passed = r.get("tests_passed_counter", 0) or 0
    assertion = r.get("assertion_errors_counter", 0) or 0
    runtime = seg  # von Unity gefangene Segfaults = Runtime-Fehler

    if compiled and tests == 0 and r.get("test_status") != "passed":
        tests, passed, assertion, runtime = 1, 0, 0, 1

    return {
        "compiled": compiled,
        "basic_compile_error": (not compiled) and cet == "basic_compile_error",
        "linker_error": (not compiled) and cet == "linker_error",
        "tests": tests if compiled else 0,
        "passed": passed if compiled else 0,
        "assertion_errors": assertion if compiled else 0,
        "runtime_errors": runtime if compiled else 0,
        "line_coverage": r.get("line_coverage") if compiled else None,
        "branch_coverage": r.get("branch_coverage") if compiled else None,
    }


def _sum_cov(covs):
    """Coverage INNERHALB eines Laufs ueber mehrere Funktionen aufsummieren."""
    vals = [c for c in covs if c]
    if not vals:
        return None
    covered = sum(c["covered"] for c in vals)
    total = sum(c["total"] for c in vals)
    return {"percent": round(100 * covered / total, 2) if total else 0.0,
            "covered": covered, "total": total}


def _mean_cov(covs):
    """Coverage-Dicts UEBER die Laeufe mitteln (keine Lauf-Summen).

    percent = Mittelwert der Lauf-Prozentwerte (run-gemittelt);
    covered/total = Mittelwert der Lauf-Summen als Kontext.
    """
    vals = [c for c in covs if c]
    if not vals:
        return None
    n = len(vals)
    return {
        "percent": round(sum(c["percent"] for c in vals) / n, 2),
        "covered": round(sum(c["covered"] for c in vals) / n, 2),
        "total": round(sum(c["total"] for c in vals) / n, 2),
    }


def _bundle(norms):
    """Kennzahlen EINES Laufs (ueber die enthaltenen FUT-Laeufe)."""
    n = len(norms)
    basic = sum(1 for x in norms if x["basic_compile_error"])
    linker = sum(1 for x in norms if x["linker_error"])
    compile_err = basic + linker
    compiled_ok = sum(1 for x in norms if x["compiled"])
    compiled_to_obj = compiled_ok + linker  # bis zum Linker gekommen

    tests = sum(x["tests"] for x in norms)
    passed = sum(x["passed"] for x in norms)
    assertion = sum(x["assertion_errors"] for x in norms)
    runtime = sum(x["runtime_errors"] for x in norms)

    return {
        "compile_errors": compile_err,
        "linker_errors": linker,
        "runtime_errors": runtime,
        "assertion_errors": assertion,
        "passed": passed,
        "tests": tests,
        "line_coverage": _sum_cov(x["line_coverage"] for x in norms),
        "branch_coverage": _sum_cov(x["branch_coverage"] for x in norms),
        "compile_error_rate": _pct(compile_err, n),
        "linker_error_rate": _pct(linker, compiled_to_obj),
        "runtime_error_rate": _pct(runtime, tests),
        "assert_error_rate": _pct(assertion, tests),
        "pass_rate": _pct(passed, tests),
    }


def _mean_bundles(bundles):
    """Mittelwert ueber Lauf-Bundles: Counts, Raten und Coverage."""
    bundles = [b for b in bundles if b]

    def mean(key):
        vals = [b[key] for b in bundles if b.get(key) is not None]
        return round(sum(vals) / len(vals), 2) if vals else None

    out = {k: mean(k) for k in COUNT_KEYS}
    out.update({k: mean(k) for k in RATE_KEYS})
    out["line_coverage"] = _mean_cov([b.get("line_coverage") for b in bundles])
    out["branch_coverage"] = _mean_cov([b.get("branch_coverage") for b in bundles])
    return out


def _slim_run(r):
    """Schlanke Lauf-Ansicht fuer JSON1 inkl. returncode, segfaults, output/error."""
    compiled = bool(r.get("compiled"))
    norm = _norm(r)

    out = {
        "run": r.get("run"),
        "compiled": compiled,
        "compile_error_type": r.get("compile_error_type"),
        "returncode": r.get("returncode"),
        "test_status": r.get("test_status"),
        "segfault_errors": r.get("segfaults_counter", 0) or 0,
        "assertion_errors": norm["assertion_errors"],
        "runtime_errors": norm["runtime_errors"],
        "tests": norm["tests"],
        "passed": norm["passed"],
    }
    if compiled:
        out["test_output"] = r.get("test_output")
    if r.get("error"):
        out["error"] = r.get("error")
    out["line_coverage"] = r.get("line_coverage")
    out["branch_coverage"] = r.get("branch_coverage")
    return out


def _per_run_bundles(futs, all_metrics):
    """Pro Lauf-Index ein Bundle ueber die uebergebenen FUTs."""
    bundles = []
    for rid in RUN_ORDER:
        norms = []
        for fut in futs:
            r = next((x for x in all_metrics[fut] if x.get("run") == rid), None)
            if r is not None:
                norms.append(_norm(r))
        if norms:
            bundles.append(_bundle(norms))
    return bundles


def _futs_without_coverage(futs, all_metrics):
    """FUTs, die in KEINEM Lauf Coverage liefern, explizit sichtbar machen."""
    return [fut for fut in futs
            if not any(_norm(r)["line_coverage"] for r in all_metrics[fut])]


def build_functions_report(model_name, all_metrics):
    """JSON1: pro Funktion jeder Lauf einzeln + Mittelwert ueber die Laeufe."""
    functions = {}
    for fut, runs in all_metrics.items():
        codebase = runs[0].get("codebase") if runs else "unknown"
        # Mittelwert: jeder Lauf einzeln zu einem 1er-Bundle, dann mitteln.
        per_run = [_bundle([_norm(r)]) for r in runs]
        functions[fut] = {
            "codebase": codebase,
            "runs": [_slim_run(r) for r in runs],
            "mean": _mean_bundles(per_run),
        }
    return {"model": model_name, "functions": functions}


def build_summary_report(model_name, all_metrics):
    """JSON2: pro Codebase + Modell-Gesamtuebersicht (Mittelwert ueber die Laeufe)."""
    fut_codebase = {fut: (runs[0].get("codebase") if runs else "unknown")
                    for fut, runs in all_metrics.items()}
    by_cb = {}
    for fut, cb in fut_codebase.items():
        by_cb.setdefault(cb, []).append(fut)

    def summarize(futs):
        summary = _mean_bundles(_per_run_bundles(futs, all_metrics))
        summary["futs_without_coverage"] = _futs_without_coverage(futs, all_metrics)
        return summary

    codebase_summary = {cb: summarize(futs) for cb, futs in by_cb.items()}

    model_mean = summarize(list(all_metrics.keys()))
    line_cov = model_mean.get("line_coverage") or {}
    branch_cov = model_mean.get("branch_coverage") or {}
    model_summary = {
        "mean_over_runs": {
            "compile_error_rate": model_mean["compile_error_rate"],
            "linker_error_rate": model_mean["linker_error_rate"],
            "runtime_error_rate": model_mean["runtime_error_rate"],
            "assert_error_rate": model_mean["assert_error_rate"],
            "pass_rate": model_mean["pass_rate"],
            "line_coverage_percent": line_cov.get("percent"),
            "branch_coverage_percent": branch_cov.get("percent"),
        },
        "futs_without_coverage": model_mean["futs_without_coverage"],
    }

    return {
        "model": model_name,
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
        print(f"  Keine Tests gefunden fuer {model_name}")
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

                    run_result["returncode"] = returncode
                    run_result["test_output"] = test_output
                    run_result["test_status"] = classify_test_result(returncode, test_output)
                    run_result["line_coverage"] = line_cov
                    run_result["branch_coverage"] = branch_cov
                    run_result["all_passed"] = run_result["test_status"] == "passed"
                    run_result.update(count_test_metrics(test_output))

                    print(f"   Run {run_name} | status: {run_result['test_status']} (exit {returncode})")
                    print(f"   Test output:\n{test_output}")
                except subprocess.TimeoutExpired:
                    run_result["test_status"] = "timeout"
                    run_result["error"] = "timeout"
                except Exception:
                    run_result["test_status"] = "error"
                    run_result["error"] = traceback.format_exc()
            else:
                # FALL A: Compile-Fehler – Linker- vs. einfacher Compile-Fehler.
                run_result["test_status"] = "compile_error"
                run_result["compile_error_type"] = classify_compile_error(compile_log)
                run_result["error"] = f"Compilation failed: {compile_log.strip()}"
                print(compile_log)

            # Aufraeumen: .gcda und .gcno entfernen
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

    # JSON1: pro Funktion (Laeufe einzeln + Mittelwert)
    functions_report = build_functions_report(model_name, all_metrics)
    functions_file = metrics_dir / "metric_functions.json"
    functions_file.write_text(json.dumps(functions_report, indent=2))
    print(f"  -> JSON1 (Funktionen): {functions_file}")

    # JSON2: pro Codebase + Modell-Gesamtuebersicht
    summary_report = build_summary_report(model_name, all_metrics)
    summary_file = metrics_dir / "metric_summary.json"
    summary_file.write_text(json.dumps(summary_report, indent=2))
    print(f"  -> JSON2 (Zusammenfassung): {summary_file}")

    return summary_report


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
        # Modellvergleich: nur nebeneinanderstellen, NICHT modelluebergreifend
        # zusammenzaehlen.
        per_model[model] = {
            "model_summary": report["model_summary"],
            "codebase_summary": report["codebase_summary"],
        }

    global_summary = {"models": list(per_model.keys()), "per_model": per_model}
    out_dir = Path(cfg["paths"]["output_metrics"])
    out_dir.mkdir(parents=True, exist_ok=True)
    global_file = out_dir / "global_metric_summary.json"
    global_file.write_text(json.dumps(global_summary, indent=2))
    print(f"\n-> Gesamt-Zusammenfassung ueber alle Modelle: {global_file}")


if __name__ == "__main__":
    main()
