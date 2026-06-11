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


COUNT_KEYS = ["compile_errors", "linker_errors", "runtime_errors",
              "assertion_errors", "passed", "tests"]

RATE_KEYS = ["compile_error_rate", "linker_error_rate", "runtime_error_rate",
             "assert_error_rate", "pass_rate"]


def _pct(num, den):
    """Prozent oder None, wenn der Nenner 0 ist."""
    return round(100 * num / den, 2) if den else None


def _run_ids(all_metrics):
    """Alle vorkommenden Lauf-Namen (run_01_test, ...) sortiert und dynamisch.

    Frueher fest auf 3 Laeufe verdrahtet; jetzt aus den Daten abgeleitet,
    damit beliebig viele Laeufe pro Funktion verarbeitet werden.
    """
    ids = set()
    for runs in all_metrics.values():
        for r in runs:
            if r.get("run"):
                ids.add(r["run"])
    return sorted(ids)


# ---------------------------------------------------------------------------
# Coverage-Helfer (Micro-Average ueber flache Zaehler)
# ---------------------------------------------------------------------------

def _cov_pair(covered, total):
    """Coverage-Paar {covered, total} aus den flachen Rohzaehlern.

    None, wenn keine Totals vorliegen (coverage_source == 'unavailable').
    Nicht-kompilierende Tests besitzen dank der 0%-Strafe in Script 01
    bereits covered=0 / total=<voll> und werden hier mitgezaehlt.
    """
    if total is None or covered is None:
        return None
    return {"covered": covered, "total": total}


def _sum_coverage(covs):
    """Micro-Summe der Coverage ueber mehrere Eintraege (z.B. Funktionen eines Laufs)."""
    valid = [c for c in covs if c]
    if not valid:
        return None
    covered = sum(c["covered"] for c in valid)
    total = sum(c["total"] for c in valid)
    return {"percent": round(100 * covered / total, 2) if total else None,
            "covered": covered, "total": total}


def _micro_coverage(covs):
    """Echter Micro-Average: Summe(covered) / Summe(total) ueber ALLE Eintraege.

    Entspricht der Doppelsumme ueber Funktionen und Laeufe:
        LC = ( Sum_j Sum_i covered_ij / Sum_j Sum_i total_ij ) * 100
    Da die Bundle-Coverage bereits covered/total-Summen enthaelt, ergibt das
    erneute Aufsummieren die korrekte Gesamtsumme.
    """
    valid = [c for c in covs if c]
    if not valid:
        return None
    covered = sum(c["covered"] for c in valid)
    total = sum(c["total"] for c in valid)
    return {"percent": round(100 * covered / total, 2) if total else None,
            "covered": covered, "total": total}


# ---------------------------------------------------------------------------
# Parsing / Normalisierung eines einzelnen Laufs
# ---------------------------------------------------------------------------

def _parse_run(run_results):
    """Uebersetzt einen Roh-Run aus raw_metrics.json in saubere Kennzahlen."""
    compiled = bool(run_results.get("compiled"))
    translation_error_type = run_results.get("translation_error_type")

    tests = run_results.get("amount_of_tests", 0) or 0
    passed = run_results.get("amount_of_tests_passed", 0) or 0
    assertion = run_results.get("amount_of_assertion_errors", 0) or 0
    runtime = run_results.get("amount_of_runtime_errors", 0) or 0

    # Kompiliert, aber kein Unity-Output (z.B. Crash vor Ausgabe) -> als 1 Test
    # mit 1 Laufzeitfehler werten, damit die Raten einen Nenner haben.
    if compiled and tests == 0 and run_results.get("test_status") != "passed":
        tests, passed, assertion, runtime = 1, 0, 0, max(runtime, 1)

    # Coverage UNABHAENGIG vom Compile-Status uebernehmen: nicht-kompilierende
    # Tests tragen dank der 0%-Strafe covered=0 / total=<voll> und sollen den
    # Micro-Average druecken (nicht ausgeschlossen werden).
    line_cov = _cov_pair(run_results.get("lines_covered"), run_results.get("lines_total"))
    branch_cov = _cov_pair(run_results.get("branches_covered"), run_results.get("branches_total"))

    return {
        "compiled": compiled,
        "compile_error": (not compiled) and translation_error_type == "compile_error",
        "linker_error": (not compiled) and translation_error_type == "linker_error",
        "tests": tests if compiled else 0,
        "passed": passed if compiled else 0,
        "assertion_errors": assertion if compiled else 0,
        "runtime_errors": runtime if compiled else 0,
        "line_coverage": line_cov,
        "branch_coverage": branch_cov,
    }


def _run_summary(normalized_run_results):
    """Aggregiert eine Menge normalisierter Laeufe zu Counts, Raten und Coverage."""
    n = len(normalized_run_results)

    compile_err = sum(1 for x in normalized_run_results if x["compile_error"])
    linker = sum(1 for x in normalized_run_results if x["linker_error"])
    compiled_ok = sum(1 for x in normalized_run_results if x["compiled"])
    compiled_to_obj = compiled_ok + linker  # bis zum Linker gekommen (Compile-Stufe bestanden)

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
        "compile_error_rate": _pct(compile_err, n),
        "linker_error_rate": _pct(linker, compiled_to_obj),
        "runtime_error_rate": _pct(runtime, tests),
        "assert_error_rate": _pct(assertion, tests),
        "pass_rate": _pct(passed, tests),
    }


def _mean_bundles(bundles):
    """Mittelwert ueber Lauf-Bundles fuer Counts und Raten;
    Coverage dagegen als echter Micro-Average (gepoolt, nicht gemittelt)."""
    bundles = [b for b in bundles if b]

    def mean(key):
        vals = [b[key] for b in bundles if b.get(key) is not None]
        return round(sum(vals) / len(vals), 2) if vals else None

    out = {k: mean(k) for k in COUNT_KEYS}
    out.update({k: mean(k) for k in RATE_KEYS})
    out["line_coverage"] = _micro_coverage([b.get("line_coverage") for b in bundles])
    out["branch_coverage"] = _micro_coverage([b.get("branch_coverage") for b in bundles])
    return out


def _cov_view(covered, total, percent):
    """Lesbare Coverage-Ansicht {covered, total, percent} fuer JSON1."""
    if total is None:
        return None
    return {"covered": covered, "total": total, "percent": percent}


def _slim_run(r):
    """Schlanke Lauf-Ansicht fuer JSON1 mit den neuen Keys."""
    compiled = bool(r.get("compiled"))
    norm = _parse_run(r)

    out = {
        "run": r.get("run"),
        "compiled": compiled,
        "translation_error_type": r.get("translation_error_type"),
        "return_code": r.get("return_code"),
        "test_status": r.get("test_status"),
        "all_tests_passed": bool(r.get("all_tests_passed")),
        "tests": norm["tests"],
        "passed": norm["passed"],
        "assertion_errors": norm["assertion_errors"],
        "runtime_errors": norm["runtime_errors"],
        "coverage_source": r.get("coverage_source"),
        "line_coverage": _cov_view(r.get("lines_covered"), r.get("lines_total"),
                                   r.get("line_coverage_pct")),
        "branch_coverage": _cov_view(r.get("branches_covered"), r.get("branches_total"),
                                     r.get("branch_coverage_pct")),
    }
    if compiled:
        out["test_output"] = r.get("test_output")
    if r.get("error_message"):
        out["error_message"] = r.get("error_message")
    return out


def _per_run_bundles(futs, all_metrics, run_ids):
    """Pro Lauf-Index ein Bundle ueber die uebergebenen FUTs."""
    bundles = []
    for rid in run_ids:
        norms = []
        for fut in futs:
            r = next((x for x in all_metrics[fut] if x.get("run") == rid), None)
            if r is not None:
                norms.append(_parse_run(r))
        if norms:
            bundles.append(_run_summary(norms))
    return bundles


def _futs_without_coverage(futs, all_metrics):
    """FUTs, fuer die in KEINEM Lauf Totals ermittelt werden konnten
    (coverage_source == 'unavailable'). Mit der 0%-Strafe sind das nur noch
    Funktionen, deren ausfuehrbare Zeilen gar nicht bestimmbar waren."""
    return [fut for fut in futs
            if not any(_parse_run(r)["line_coverage"] for r in all_metrics[fut])]


def _function_all_passed(runs):
    return bool(runs) and all(bool(r.get("all_tests_passed")) for r in runs)


def build_functions_report(model_name, all_metrics):
    """JSON1: pro Funktion jeder Lauf einzeln + Mittelwert/Micro ueber die Laeufe."""
    functions = {}
    for fut, runs in all_metrics.items():
        codebase = runs[0].get("codebase") if runs else "unknown"
        # Mittelwert: jeder Lauf einzeln zu einem 1er-Bundle, dann mitteln.
        per_run = [_run_summary([_parse_run(r)]) for r in runs]
        functions[fut] = {
            "codebase": codebase,
            "all_tests_passed": _function_all_passed(runs),
            "runs": [_slim_run(r) for r in runs],
            "mean": _mean_bundles(per_run),
        }
    return {"model": model_name, "functions": functions}


def build_summary_report(model_name, all_metrics):
    """JSON2: pro Codebase + Modell-Gesamtuebersicht (Counts/Raten gemittelt,
    Coverage als Micro-Average)."""
    run_ids = _run_ids(all_metrics)

    fut_codebase = {fut: (runs[0].get("codebase") if runs else "unknown")
                    for fut, runs in all_metrics.items()}
    by_cb = {}
    for fut, cb in fut_codebase.items():
        by_cb.setdefault(cb, []).append(fut)

    def summarize(futs):
        summary = _mean_bundles(_per_run_bundles(futs, all_metrics, run_ids))
        summary["futs_without_coverage"] = _futs_without_coverage(futs, all_metrics)
        return summary

    codebase_summary = {cb: summarize(futs) for cb, futs in by_cb.items()}

    model_mean = summarize(list(all_metrics.keys()))
    line_cov = model_mean.get("line_coverage") or {}
    branch_cov = model_mean.get("branch_coverage") or {}
    model_summary = {
        "amount_of_functions": len(all_metrics),
        "amount_of_all_tests_passed": sum(
            1 for runs in all_metrics.values() if _function_all_passed(runs)
        ),
        "mean_over_runs": {
            "compile_error_rate": model_mean["compile_error_rate"],
            "linker_error_rate": model_mean["linker_error_rate"],
            "runtime_error_rate": model_mean["runtime_error_rate"],
            "assert_error_rate": model_mean["assert_error_rate"],
            "pass_rate": model_mean["pass_rate"],
        },
        "coverage_micro": {
            "line_coverage_percent": line_cov.get("percent"),
            "line_covered": line_cov.get("covered"),
            "line_total": line_cov.get("total"),
            "branch_coverage_percent": branch_cov.get("percent"),
            "branch_covered": branch_cov.get("covered"),
            "branch_total": branch_cov.get("total"),
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
        print("  -> Bitte zuerst das Test-Runner-Skript (01_run_tests.py) ausfuehren.")
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
