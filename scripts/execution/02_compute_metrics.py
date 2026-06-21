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



def _pct(num, den):
    return 100.0 * num / den if den else None


def _mean(vals):
    vals = [v for v in vals if v is not None]
    return sum(vals) / len(vals) if vals else None


def _round(v):
    return round(v, 2) if v is not None else None


def _100_minus(v):
    return None if v is None else 100.0 - v


def _norm_run(r):
    """Normalisiert einen Roh-Lauf fuer die absoluten Summen.
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
    if "pass_status" in r and r["pass_status"] is not None:
        return bool(r["pass_status"])
    n = _norm_run(r)
    return (n["compiled"] and n["tests"] > 0 and n["passed"] == n["tests"]
            and n["runtime"] == 0 and n["assertion"] == 0)


def _fut_run_flags(r):
    """Binaere Flags eines FUT-Laufs.
    """
    ce = int(r.get("compile_error_status") or 0)
    le = int(r.get("linker_error_status") or 0)

    p = r.get("pass_status")
    re = r.get("runtime_error_status")
    ae = r.get("assertion_error_status")

    if p is None or re is None or ae is None:
        if _all_passed(r):
            re, ae, p = 0, 0, 1
        elif ce or le:
            re, ae, p = 1, 0, 0
        elif int(r.get("runtime_errors") or 0) > 0:
            re, ae, p = 1, 0, 0
        elif int(r.get("assertion_errors") or 0) > 0:
            re, ae, p = 0, 1, 0
        else:
            re, ae, p = 1, 0, 0

    return {"ce": ce, "le": le, "re": int(re), "ae": int(ae), "p": int(p)}



def _fut_bounded_totals(fut, fut_totals, runs):
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
    if all(r.get(list_key) is None for r in runs):
        return max((int(r.get(count_key) or 0) for r in runs), default=0)
    union = set()
    for r in runs:
        union |= set(r.get(list_key) or [])
    return len(union)


def _fut_rate_means(runs):
    flags = [_fut_run_flags(r) for r in runs]
    R = len(flags)
    if R == 0:
        return {k: None for k in RATE_KEYS}
    return {
        "cer": 100.0 * sum(f["ce"] for f in flags) / R,
        "ler": 100.0 * sum(f["le"] for f in flags) / R,
        "rer": 100.0 * sum(f["re"] for f in flags) / R,
        "aer": 100.0 * sum(f["ae"] for f in flags) / R,
        "pr":  100.0 * sum(f["p"]  for f in flags) / R,
    }



def _function_record(r):
    n = _norm_run(r)
    flags = _fut_run_flags(r)
    return {
        "run": r.get("run"),
        "codebase": r.get("codebase"),
        "compile_error_status": flags["ce"],
        "linker_error_status": flags["le"],
        "runtime_error_status": flags["re"],
        "assertion_error_status": flags["ae"],
        "pass_status": flags["p"],
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



def _codebase_coverage_pct(futs, all_metrics, fut_totals):

    run_ids = sorted({r.get("run") for f in futs for r in all_metrics[f]
                      if r.get("run")})
    totals = {f: _fut_bounded_totals(f, fut_totals, all_metrics[f]) for f in futs}
    line_den = sum(lt for lt, _ in totals.values() if lt is not None)
    branch_den = sum(bt for _, bt in totals.values() if bt is not None)

    lc_runs, bc_runs = [], []
    for rid in run_ids:
        line_num = branch_num = 0
        for f in futs:
            r = next((x for x in all_metrics[f] if x.get("run") == rid), None)
            if r is None:
                continue
            lt, bt = totals[f]
            if lt is not None:
                line_num += min(int(r.get("lines_covered") or 0), lt)
            if bt is not None:
                branch_num += min(int(r.get("branches_covered") or 0), bt)
        if line_den:
            lc_runs.append(100.0 * line_num / line_den)
        if branch_den:
            bc_runs.append(100.0 * branch_num / branch_den)

    return _mean(lc_runs), _mean(bc_runs)


def _codebase_rates(futs, all_metrics):
    """Makro-Mittel der FUT-Raten"""
    per_fut = [_fut_rate_means(all_metrics[f]) for f in futs]
    return {k: _mean([m[k] for m in per_fut]) for k in RATE_KEYS}


def build_codebase_summary(futs, all_metrics, fut_totals):
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

        for r in runs:
            n = _norm_run(r)
            execution["tests_total"] += n["tests"]
            execution["tests_passed"] += n["passed"]
            execution["compile_errors"] += n["ce"]
            execution["linker_errors"] += n["le"]
            execution["runtime_errors"] += n["runtime"]
            execution["assertion_errors"] += n["assertion"]


    rates = _codebase_rates(futs, all_metrics)
    lc_cb, bc_cb = _codebase_coverage_pct(futs, all_metrics, fut_totals)

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

            "coverage": {
                "line_coverage": _round(lc_cb),
                "branch_coverage": _round(bc_cb),
            },
       
            "rates": {
                "compile_error_rate": _round(rates["cer"]),
                "linker_error_rate": _round(rates["ler"]),
                "runtime_error_rate": _round(rates["rer"]),
                "assert_error_rate": _round(rates["aer"]),
                "pass_rate": _round(rates["pr"]),
                "compile_success_rate": _round(_100_minus(rates["cer"])),
                "linker_success_rate": _round(_100_minus(rates["ler"])),
            },
        }
    }


def build_codebase_level(all_metrics, fut_totals):
    by_cb = {}
    for fut, runs in all_metrics.items():
        cb = runs[0].get("codebase") if runs else "unknown"
        by_cb.setdefault(cb, []).append(fut)
    return {cb: build_codebase_summary(futs, all_metrics, fut_totals)
            for cb, futs in sorted(by_cb.items())}


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


    line_den = sum(lt for lt, _ in totals.values() if lt is not None)
    branch_den = sum(bt for _, bt in totals.values() if bt is not None)
    lc_runs, bc_runs = [], []

    lcf_runs, bcf_runs = [], []
    for rid in run_ids:
        line_num = branch_num = 0
        line_num_f = line_den_f = 0
        branch_num_f = branch_den_f = 0
        for fut in futs:
            r = next((x for x in all_metrics[fut] if x.get("run") == rid), None)
            if r is None:
                continue 
            lt, bt = totals[fut]
            lc = min(int(r.get("lines_covered") or 0), lt) if lt is not None else 0
            bc = min(int(r.get("branches_covered") or 0), bt) if bt is not None else 0
            if lt is not None:
                line_num += lc
            if bt is not None:
                branch_num += bc

            flags = _fut_run_flags(r)
            success = (1 - flags["ce"]) * (1 - flags["le"]) * (1 - flags["re"])
            if success:
                if lt is not None:
                    line_num_f += lc
                    line_den_f += lt
                if bt is not None:
                    branch_num_f += bc
                    branch_den_f += bt
        if line_den:
            lc_runs.append(100.0 * line_num / line_den)
        if branch_den:
            bc_runs.append(100.0 * branch_num / branch_den)
        if line_den_f:
            lcf_runs.append(100.0 * line_num_f / line_den_f)
        if branch_den_f:
            bcf_runs.append(100.0 * branch_num_f / branch_den_f)


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

            "coverage_filtered": {
                "line_coverage": _round(_mean(lcf_runs)),
                "branch_coverage": _round(_mean(bcf_runs)),
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
