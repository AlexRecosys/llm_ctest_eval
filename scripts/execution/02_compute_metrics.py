#!/usr/bin/env python3
"""02_compute_metrics.py

Liest raw_metrics.json (aus 01_measure_coverage.py) und erzeugt pro Modell
die drei Ziel-JSONs gemaess Schema:

1. function_level.json  - pro FUT eine Liste der Lauf-Datensaetze
2. codebase_level.json  - absolute Aggregation pro Codebase-Cluster:
     * Strukturelle Code-Eigenschaften: "Keep Bounded" (statische Totals,
       genau einmal pro FUT, NICHT mit der Lauf-Anzahl multipliziert)
     * Unique Code Coverage: "Take the Union" (eindeutige Zeilen/Branches,
       die ueber alle Laeufe hinweg getroffen wurden)
     * Ausfuehrungsereignisse: "Sum over all runs" (kumulative Summen)
3. model_level.json     - oberste Evaluationsebene ohne rohe Zeilen/Branches:
     * Coverage gemaess LC-Modell_Run_r = Sum(LC_f,r) / Sum(LT_f) * 100,
       anschliessend Mittelwert ueber die R Laeufe
     * Raten als hierarchische Makro-Mittel: Run -> FUT -> Codebase -> Modell
"""

import json
from pathlib import Path

import yaml

RATE_KEYS = ("cer", "ler", "rer", "aer", "pr")


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


# ---------------------------------------------------------------------------
# Helfer
# ---------------------------------------------------------------------------

def _pct(num, den):
    """Prozent (ungerundet) oder None, wenn der Nenner 0 ist."""
    return 100.0 * num / den if den else None


def _mean(vals):
    """Ungerundeter Mittelwert; None-Werte (undefinierte Raten) werden
    uebersprungen, damit sie den Nenner nicht verfaelschen."""
    vals = [v for v in vals if v is not None]
    return sum(vals) / len(vals) if vals else None


def _round(v):
    return round(v, 2) if v is not None else None


def _norm_run(r):
    """Normalisiert einen Roh-Lauf fuer die Raten-/Summenbildung.

    Harter Crash/Timeout ohne verwertbaren Unity-Output (kompiliert, aber
    0 Tests und nicht 'passed') zaehlt als genau 1 Test mit 1 Runtime-Fehler,
    damit der Lauf im Nenner bleibt statt zu verschwinden. Damit bleibt die
    Identitaet RE + AE + P = T erhalten.
    """
    compiled = bool(r.get("compiled"))
    tests = int(r.get("tests_total") or 0)
    passed = int(r.get("tests_passed") or 0)
    assertion = int(r.get("assertion_errors") or 0)
    runtime = int(r.get("runtime_errors") or 0)

    if compiled and tests == 0 and not r.get("all_tests_passed"):
        runtime = max(runtime, 1)  # Runtime-Fehler fuer absolute Summen erhalten
    if not compiled:
        tests = passed = assertion = runtime = 0

    return {
        "compiled": compiled,
        "ce": int(r.get("compile_error_status") or 0),
        "le": int(r.get("linker_error_status") or 0),
        "tests": tests,
        "passed": passed,
        "assertion": assertion,
        "runtime": runtime,
    }

def _all_passed(r):
    if "all_tests_passed" in r:
        return bool(r["all_tests_passed"])
    n = _norm_run(r)
    return (n["compiled"] and n["tests"] > 0 and n["passed"] == n["tests"]
            and n["runtime"] == 0 and n["assertion"] == 0)



def _fut_bounded_totals(fut, fut_totals, runs):
    # Erst aus fut_totals (statische Referenz) holen
    info = (fut_totals or {}).get(fut) or {}
    lt = info.get("lines_total")
    bt = info.get("branches_total")

    if lt is None:
        vals = [r.get("lines_total") for r in runs
                if r.get("lines_total") is not None]
        lt = max(vals) if vals else None

    if bt is None:
        vals = [r.get("branches_total") for r in runs
                if r.get("branches_total") is not None]
        bt = max(vals) if vals else None


    return lt, bt


def _union_covered(runs, list_key, count_key):
    """Unique-Union der getroffenen Zeilen/Branches UEBER die Laeufe
    ("Take the Union" - keine Addition der Lauf-Zaehler).

    Fallback (alte Rohdaten ohne ID-Listen): Maximum der Lauf-Zaehler als
    konservative untere Schranke der Union."""
    if all(r.get(list_key) is None for r in runs):
        return max((int(r.get(count_key) or 0) for r in runs), default=0)
    union = set()
    for r in runs:
        union |= set(r.get(list_key) or [])
    return len(union)


def _fut_rate_means(runs):
    """Raten einer FUT: pro Lauf berechnen, dann ueber die Laeufe mitteln.

    CER/LER: binaere Flags, Mittel ueber ALLE R Laeufe (LaTeX: /R).
    RER/AER/PR: count / T_{f,r} * 100; Laeufe ohne Tests (nicht kompiliert)
    liefern None und fallen aus dem Mittel."""
    norms = [_norm_run(r) for r in runs]
    return {
        "cer": _mean([100.0 * n["ce"] for n in norms]),
        "ler": _mean([100.0 * n["le"] for n in norms]),
        "rer": _mean([_pct(n["runtime"], n["tests"]) for n in norms]),
        "aer": _mean([_pct(n["assertion"], n["tests"]) for n in norms]),
        "pr": _mean([_pct(n["passed"], n["tests"]) for n in norms]),
    }


# ---------------------------------------------------------------------------
# File 1: function_level.json
# ---------------------------------------------------------------------------

def _function_record(r):
    """Ein Lauf-Datensatz strikt im Ziel-Schema (keine Legacy-Felder)."""
    n = _norm_run(r)
    return {
        "run": r.get("run"),
        "codebase": r.get("codebase"),
        "compile_error_status": n["ce"],
        "linker_error_status": n["le"],
        "return_code": r.get("return_code"),
        "tests_total": n["tests"],
        "tests_passed": n["passed"],
        "runtime_errors": n["runtime"],
        "assertion_errors": n["assertion"],
        "lines_total": r.get("lines_total"),
        "lines_covered": r.get("lines_covered"),
        "branches_total": r.get("branches_total"),
        "branches_covered": r.get("branches_covered"),
        "test_output": r.get("test_output") or "",
        "error_message": r.get("error_message") or "",
    }


def build_function_level(all_metrics):
    return {fut: [_function_record(r) for r in runs]
            for fut, runs in all_metrics.items()}


# ---------------------------------------------------------------------------
# File 2: codebase_level.json
# ---------------------------------------------------------------------------

def build_codebase_summary(futs, all_metrics, fut_totals):
    """Absolute Aggregation eines Codebase-Clusters inkl. der Gewichte fuer
    die modellweiten Prozentwerte."""
    lines_total = branches_total = 0
    lines_covered = branches_covered = 0
    futs_without_coverage = []
    futs_without_values = []
    futs_all_passed = 0
    execution = {"tests_total": 0, "tests_passed": 0, "compile_errors": 0,
                 "linker_errors": 0, "runtime_errors": 0, "assertion_errors": 0}

    for fut in futs:
        runs = all_metrics[fut]
        lt, bt = _fut_bounded_totals(fut, fut_totals, runs)

        if max((int(r.get("tests_total") or 0) for r in runs), default=0) == 0:
            futs_without_values.append(fut)

        # Keep Bounded: statische Totals genau einmal pro FUT.
        # Take the Union: eindeutige getroffene Zeilen/Branches ueber Laeufe.
        if lt is None:
            futs_without_coverage.append(fut)
        else:
            lines_total += lt
            lines_covered += min(_union_covered(runs, "covered_lines",
                                                "lines_covered"), lt)
        if bt is not None:
            branches_total += bt
            branches_covered += min(_union_covered(runs, "covered_branches",
                                                   "branches_covered"), bt)

        if runs and all(_all_passed(r) for r in runs):
            futs_all_passed += 1

        # Sum over all runs: jedes Ereignis kumulativ aufsummieren.
        for r in runs:
            n = _norm_run(r)
            execution["tests_total"] += n["tests"]
            execution["tests_passed"] += n["passed"]
            execution["compile_errors"] += n["ce"]
            execution["linker_errors"] += n["le"]
            execution["runtime_errors"] += n["runtime"]
            execution["assertion_errors"] += n["assertion"]

    return {
        "codebase_summary": {
            "functions_under_test": {
                "total_futs": len(futs),
                "futs_all_tests_passed": futs_all_passed,
                "futs_without_values": sorted(futs_without_values),
            },
            "coverage_variables": {
                "lines_total": lines_total,
                "lines_covered": lines_covered,
                "branches_total": branches_total,
                "branches_covered": branches_covered,
            },
            "execution_totals": execution,
            
        }
    }


def build_codebase_level(all_metrics, fut_totals):
    by_cb = {}
    for fut, runs in all_metrics.items():
        cb = runs[0].get("codebase") if runs else "unknown"
        by_cb.setdefault(cb, []).append(fut)
    return {cb: build_codebase_summary(futs, all_metrics, fut_totals)
            for cb, futs in sorted(by_cb.items())}


# ---------------------------------------------------------------------------
# File 3: model_level.json
# ---------------------------------------------------------------------------

def build_model_level(all_metrics, fut_totals):
    futs = list(all_metrics)
    run_ids = sorted({r.get("run") for runs in all_metrics.values()
                      for r in runs if r.get("run")})

    totals = {fut: _fut_bounded_totals(fut, fut_totals, all_metrics[fut])
              for fut in futs}
   
    
    futs_without_values = sorted(
        f for f in futs
        if max((int(r.get("tests_total") or 0) for r in all_metrics[f]), default=0) == 0
    )

    # Coverage gemaess LaTeX-Modell:
    #   LC-Modell_Run_r = Sum_f LC_{f,r} / Sum_f LT_f * 100  (0%-Strafe:
    #   nicht kompilierende Laeufe tragen covered = 0 bei vollem Nenner),
    #   LC_Modell = Mittel ueber die R Laeufe.
    line_den = sum(lt for lt, _ in totals.values() if lt is not None)
    branch_den = sum(bt for _, bt in totals.values() if bt is not None)
    lc_runs, bc_runs = [], []
    for rid in run_ids:
        line_num = branch_num = 0
        for fut in futs:
            r = next((x for x in all_metrics[fut] if x.get("run") == rid), None)
            if r is None:
                continue  # fehlender Lauf -> covered 0 (0%-Strafe)
            lt, bt = totals[fut]
            if lt is not None:
                line_num += min(int(r.get("lines_covered") or 0), lt)
            if bt is not None:
                branch_num += min(int(r.get("branches_covered") or 0), bt)
        if line_den:
            lc_runs.append(100.0 * line_num / line_den)
        if branch_den:
            bc_runs.append(100.0 * branch_num / branch_den)

    # Raten hierarchisch: Run -> FUT -> Codebase -> Modell (Makro-Mittel).
    by_cb = {}
    for fut, runs in all_metrics.items():
        cb = runs[0].get("codebase") if runs else "unknown"
        by_cb.setdefault(cb, []).append(fut)

    cb_rates = {}
    for cb, cb_futs in by_cb.items():
        per_fut = [_fut_rate_means(all_metrics[f]) for f in cb_futs]
        cb_rates[cb] = {k: _mean([m[k] for m in per_fut]) for k in RATE_KEYS}
    model_rates = {k: _mean([cb_rates[cb][k] for cb in cb_rates])
                   for k in RATE_KEYS}

    total_runs = sum(len(runs) for runs in all_metrics.values())
    runs_all_passed = sum(1 for runs in all_metrics.values()
                          for r in runs if _all_passed(r))
    futs_all_passed = sum(1 for runs in all_metrics.values()
                          if runs and all(_all_passed(r) for r in runs))

    return {
        "model_summary": {
            "variables": {
                "total_futs": len(futs),
                "futs_without_values": sorted(futs_without_values),
                
            },
            "run_statistics": {
                "total_runs": total_runs,
                "runs_all_tests_passed": runs_all_passed,
                "futs_all_tests_passed": futs_all_passed,
            },
            "coverage": {
                "line_coverage": _round(_mean(lc_runs)),
                "branch_coverage": _round(_mean(bc_runs)),
            },
            "rates": {
                "compile_error_rate": _round(model_rates["cer"]),
                "linker_error_rate": _round(model_rates["ler"]),
                "runtime_error_rate": _round(model_rates["rer"]),
                "assert_error_rate": _round(model_rates["aer"]),
                "pass_rate": _round(model_rates["pr"]),
            },
        }
    }


# ---------------------------------------------------------------------------
# Ablauf pro Modell
# ---------------------------------------------------------------------------

def report_model(model_name, cfg):
    """Liest die Rohmetriken eines Modells und schreibt die drei Ziel-JSONs."""
    metrics_dir = Path(cfg["paths"]["output_metrics"]) / f"{model_name}_modell"
    raw_file = metrics_dir / "raw_metrics.json"

    if not raw_file.exists():
        print(f"  Keine Rohmetriken gefunden fuer {model_name} ({raw_file})")
        print("  -> Bitte zuerst 01_measure_coverage.py ausfuehren.")
        return None

    raw = json.loads(raw_file.read_text())
    all_metrics = raw["all_metrics"]
    fut_totals = raw.get("fut_totals", {})

    function_level = build_function_level(all_metrics)
    f1 = metrics_dir / "function_level.json"
    f1.write_text(json.dumps(function_level, indent=2))
    print(f"  -> File 1 (Funktionen):  {f1}")

    codebase_level = build_codebase_level(all_metrics, fut_totals)
    f2 = metrics_dir / "codebase_level.json"
    f2.write_text(json.dumps(codebase_level, indent=2))
    print(f"  -> File 2 (Codebases):   {f2}")

    model_level = build_model_level(all_metrics, fut_totals)
    f3 = metrics_dir / "model_level.json"
    f3.write_text(json.dumps(model_level, indent=2))
    print(f"  -> File 3 (Modell):      {f3}")

    return model_level


def main():
    import sys
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "qwen"]

    per_model = {}
    for model in models:
        print(f"\n=== Berechne Metriken: {model} ===")
        report = report_model(model, cfg)
        if report:
            # Modellvergleich: nur nebeneinanderstellen, NICHT
            # modelluebergreifend zusammenzaehlen.
            per_model[model] = report["model_summary"]

    if per_model:
        out_dir = Path(cfg["paths"]["output_metrics"])
        out_dir.mkdir(parents=True, exist_ok=True)
        global_file = out_dir / "model_comparison.json"
        global_file.write_text(json.dumps(
            {"models": list(per_model), "per_model": per_model}, indent=2))
        print(f"\n-> Modellvergleich (On-Premise vs. Cloud): {global_file}")


if __name__ == "__main__":
    main()
