import json
from pathlib import Path
import yaml


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


RUN_ORDER = ["run_01_test", "run_02_test", "run_03_test"]

COUNT_KEYS = ["compile_errors", "linker_errors", "runtime_errors",
              "assertion_errors", "passed", "tests"]

RATE_KEYS = ["compile_error_rate", "linker_error_rate", "runtime_error_rate",
             "assert_error_rate", "pass_rate"]


def _pct(num, den):
    """Prozent oder None, wenn der Nenner 0 ist."""
    return round(100 * num / den, 2) if den else None


def _parse_run(run_results):
    """
    übersetzt die Daten aus der gcov Ausgabe in Kennzahlen.
    Ein Roh Run wird geparst und in ein sauberes Format gebracht.

    """
    compiled = bool(run_results.get("compiled"))
    compile_error_type = run_results.get("compile_error_type")
    segfaults = run_results.get("segfaults_counter", 0) or 0
    tests = run_results.get("amount_of_tests", 0) or 0
    passed = run_results.get("tests_passed_counter", 0) or 0
    assertion = run_results.get("assertion_errors_counter", 0) or 0
    runtime = segfaults  # von Unity gefangene Segfaults = Runtime-Fehler

    if compiled and tests == 0 and run_results.get("test_status") != "passed":
        tests, passed, assertion, runtime = 1, 0, 0, 1

    return {
        "compiled": compiled,
        "compile_error": (not compiled),
        "linker_error": (not compiled) and compile_error_type == "linker_error",
        "tests": tests if compiled else 0,
        "passed": passed if compiled else 0,
        "assertion_errors": assertion if compiled else 0,
        "runtime_errors": runtime if compiled else 0,
        "line_coverage": run_results.get("line_coverage") if compiled else None,
        "branch_coverage": run_results.get("branch_coverage") if compiled else None,
    }


def _sum_coverage(covs):
    """Coverage INNERHALB eines Laufs ueber mehrere Funktionen aufsummieren."""
    valid_coverages = [c for c in covs if c]
    if not valid_coverages:
        return None
    covered = sum(c["covered"] for c in valid_coverages)
    total = sum(c["total"] for c in valid_coverages)
    return {"percent": round(100 * covered / total, 2) if total else 0.0,
            "covered": covered, "total": total}


def _mean_coverage(covs):
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


def _run_summary(normalized_run_results):
    
    len_normalized_run_results = len(normalized_run_results)

    compile_err = sum(1 for x in normalized_run_results if x["compile_error"])
    linker = sum(1 for x in normalized_run_results if x["linker_error"])
    compiled_ok = sum(1 for x in normalized_run_results if x["compiled"])
    compiled_to_obj = compiled_ok + linker  # bis zum Linker gekommen

    tests = sum(x["tests"] for x in normalized_run_results)
    passed = sum(x["passed"] for x in normalized_run_results)
    assertion = sum(x["assertion_errors"] for x in normalized_run_results)
    runtime = sum(x["runtime_errors"] for x in normalized_run_results)

    return {
        "compile_errors": compile_err,
        "linker_errors": linker,
        "runtime_errors": runtime,
        "assertion_errors": assertion,
        "passed": passed,
        "tests": tests,
        "line_coverage": _sum_coverage(x["line_coverage"] for x in normalized_run_results),
        "branch_coverage": _sum_coverage(x["branch_coverage"] for x in normalized_run_results),
        "compile_error_rate": _pct(compile_err, len_normalized_run_results),
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
    out["line_coverage"] = _mean_coverage([b.get("line_coverage") for b in bundles])
    out["branch_coverage"] = _mean_coverage([b.get("branch_coverage") for b in bundles])
    return out


def _slim_run(r):
    """Schlanke Lauf-Ansicht fuer JSON1 inkl. returncode, segfaults, output/error."""
    compiled = bool(r.get("compiled"))
    norm = _parse_run(r)

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
                norms.append(_parse_run(r))
        if norms:
            bundles.append(_run_summary(norms))
    return bundles


def _futs_without_coverage(futs, all_metrics):
    """FUTs, die in KEINEM Lauf Coverage liefern, explizit sichtbar machen."""
    return [fut for fut in futs
            if not any(_parse_run(r)["line_coverage"] for r in all_metrics[fut])]


def build_functions_report(model_name, all_metrics):
    """JSON1: pro Funktion jeder Lauf einzeln + Mittelwert ueber die Laeufe."""
    functions = {}
    for fut, runs in all_metrics.items():
        codebase = runs[0].get("codebase") if runs else "unknown"
        # Mittelwert: jeder Lauf einzeln zu einem 1er-Bundle, dann mitteln.
        per_run = [_run_summary([_parse_run(r)]) for r in runs]
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


def report_model(model_name, cfg):
    """Liest die Rohmetriken eines Modells ein und schreibt JSON1 + JSON2."""
    metrics_dir = Path(cfg["paths"]["output_metrics"]) / f"{model_name}_modell"
    raw_file = metrics_dir / "raw_metrics.json"

    if not raw_file.exists():
        print(f"  Keine Rohmetriken gefunden fuer {model_name} ({raw_file})")
        print("  -> Bitte zuerst 01_compile_run.py ausfuehren.")
        return None

    raw = json.loads(raw_file.read_text())
    all_metrics = raw["all_metrics"]

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
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "qwen"]

    per_model = {}
    for model in models:
        print(f"\n=== Berechne Metriken: {model} ===")
        report = report_model(model, cfg)
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
