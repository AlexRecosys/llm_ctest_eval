#!/usr/bin/env python3

import argparse
import csv
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

try:
    import yaml
except ImportError:
    yaml = None

RATE_KEYS = ("cer", "ler", "rer", "aer", "pr")


CODEBASE_LABELS = {
    "cjson_codebase": "Codebase cJSON",
    "csv_codebase": "Codebase CSV",
    "hashmap_codebase": "Codebase Hashmap",
    "unknown": "Codebase (unbekannt)",
}



def resolve_metrics_dir(explicit=None):
    """Findet das output_metrics-Verzeichnis"""
    if explicit:
        return Path(explicit)
    if yaml is None:
        raise SystemExit("Kein --metrics-dir angegeben und PyYAML fehlt.")
    script_dir = Path(__file__).parent
    config_path = script_dir.parent.parent / "config.yaml"
    if not config_path.exists():
        raise SystemExit(
            f"config.yaml nicht gefunden ({config_path}). "
            f"Bitte --metrics-dir <Pfad> angeben."
        )
    with open(config_path) as f:
        cfg = yaml.safe_load(f)
    val = Path(cfg["paths"]["output_metrics"])
    if not val.is_absolute():
        val = config_path.parent / val
    return val


def load_model_files(metrics_dir, model):
    """Laedt die drei JSONs eines Modells."""
    base = Path(metrics_dir) / f"{model}_modell"
    fl = base / "function_level.json"
    cl = base / "codebase_level.json"
    ml = base / "model_level.json"
    if not fl.exists() or not ml.exists():
        print(f"  [!] Dateien fuer '{model}' fehlen in {base} - uebersprungen.")
        return None
    data = {
        "function_level": json.loads(fl.read_text()),
        "model_level": json.loads(ml.read_text()),
        "codebase_level": json.loads(cl.read_text()) if cl.exists() else {},
    }
    return data


def load_human_eval(metrics_dir):
    """Laedt human_eval.json"""
    path = Path(metrics_dir) / "human_eval.json"
    if not path.exists():
        print(f"  [!] human_eval.json nicht gefunden ({path}) - "
              f"Human-Eval-Werte werden uebersprungen.")
        return {}
    try:
        return json.loads(path.read_text())
    except (json.JSONDecodeError, OSError) as e:
        print(f"  [!] human_eval.json nicht lesbar ({e}) - uebersprungen.")
        return {}


def _pct(num, den):
    return 100.0 * num / den if den else None


def _mean(vals):
    vals = [v for v in vals if v is not None]
    return sum(vals) / len(vals) if vals else None


def _he_fut_pct(score):
    if score is None:
        return None
    try:
        return float(score) / 10.0 * 100.0
    except (TypeError, ValueError):
        return None


def he_codebase_pct(human_eval, model, function_level):

    model_scores = human_eval.get(model) or {}
    by_cb = _group_by_codebase(function_level)

    nested = bool(model_scores) and all(
        isinstance(v, dict) for v in model_scores.values())

    if nested:
        flat = {}
        for inner in model_scores.values():
            if isinstance(inner, dict):
                flat.update(inner)
    else:
        flat = model_scores

    out = {}
    for cb in sorted(by_cb, key=lambda c: (c == "unknown", c)):
        pcts = [_he_fut_pct(flat.get(f)) for f in by_cb[cb]]
        out[cb] = _mean(pcts)
    return out


def he_model_pct(human_eval, model, function_level):

    cb_pct = he_codebase_pct(human_eval, model, function_level)
    return _mean(list(cb_pct.values()))


def _run_flags(r):
    ce = int(r.get("compile_error_status") or 0)
    le = int(r.get("linker_error_status") or 0)
    p = r.get("pass_status")
    re = r.get("runtime_error_status")
    ae = r.get("assertion_error_status")

    if p is None or re is None or ae is None:
        tests = int(r.get("tests_total") or 0)
        passed = int(r.get("tests_passed") or 0)
        runtime = int(r.get("runtime_errors") or 0)
        assertion = int(r.get("assertion_errors") or 0)
        if ce == 0 and le == 0 and tests > 0 and passed == tests:
            re, ae, p = 0, 0, 1
        elif ce or le:
            re, ae, p = 1, 0, 0
        elif runtime > 0:
            re, ae, p = 1, 0, 0
        elif assertion > 0:
            re, ae, p = 0, 1, 0
        else:
            re, ae, p = 1, 0, 0

    return {"ce": ce, "le": le, "re": int(re), "ae": int(ae), "p": int(p)}


def _bounded_totals(runs):
    """Totals einer FUT """
    lt = [r.get("lines_total") for r in runs if r.get("lines_total") is not None]
    bt = [r.get("branches_total") for r in runs if r.get("branches_total") is not None]
    return (max(lt) if lt else None, max(bt) if bt else None)


def _group_by_codebase(function_level):
    by_cb = {}
    for fut, runs in function_level.items():
        cb = runs[0].get("codebase") if runs else "unknown"
        by_cb.setdefault(cb, []).append(fut)
    return by_cb


def per_run_breakdown(function_level):

    run_ids = sorted({r.get("run") for runs in function_level.values()
                      for r in runs if r.get("run")})
    futs = list(function_level)
    totals = {f: _bounded_totals(function_level[f]) for f in futs}
    by_cb = _group_by_codebase(function_level)

    line_den = sum(lt for lt, _ in totals.values() if lt is not None)
    branch_den = sum(bt for _, bt in totals.values() if bt is not None)

    out = {"runs": [], "line_coverage": [], "branch_coverage": [],
           "comp_success": [], "linker_success": [],
           "pass_rate": [], "assert_error_rate": [], "runtime_error_rate": []}

    for rid in run_ids:

        line_num = branch_num = 0
        for f in futs:
            r = next((x for x in function_level[f] if x.get("run") == rid), None)
            if r is None:
                continue
            lt, bt = totals[f]
            if lt is not None:
                line_num += min(int(r.get("lines_covered") or 0), lt)
            if bt is not None:
                branch_num += min(int(r.get("branches_covered") or 0), bt)

        cb_rates = {}
        for cb, cb_futs in by_cb.items():
            per_fut = []
            for f in cb_futs:
                r = next((x for x in function_level[f] if x.get("run") == rid), None)
                if r is None:
                    per_fut.append({k: None for k in RATE_KEYS})
                    continue
                fl = _run_flags(r)
                per_fut.append({
                    "cer": 100.0 * fl["ce"],
                    "ler": 100.0 * fl["le"],
                    "rer": 100.0 * fl["re"],
                    "aer": 100.0 * fl["ae"],
                    "pr": 100.0 * fl["p"],
                })
            cb_rates[cb] = {k: _mean([m[k] for m in per_fut]) for k in RATE_KEYS}
        model_rate = {k: _mean([cb_rates[cb][k] for cb in cb_rates])
                      for k in RATE_KEYS}

        def _succ(rate):
            return None if rate is None else 100.0 - rate

        out["runs"].append(rid)
        out["line_coverage"].append(_pct(line_num, line_den))
        out["branch_coverage"].append(_pct(branch_num, branch_den))
        out["comp_success"].append(_succ(model_rate["cer"]))
        out["linker_success"].append(_succ(model_rate["ler"]))
        out["pass_rate"].append(model_rate["pr"])
        out["assert_error_rate"].append(model_rate["aer"])
        out["runtime_error_rate"].append(model_rate["rer"])
    return out


def collect_codebase_metrics(data):
    """Liest die Codebase-Ebenen-Kennzahlen aus codebase_level.json."""
    cl = data.get("codebase_level") or {}
    fl = data["function_level"]
    by_cb = _group_by_codebase(fl)

    out = {}
    for cb in sorted(by_cb, key=lambda c: (c == "unknown", c)):
        summ = (cl.get(cb) or {}).get("codebase_summary", {})
        cov = summ.get("coverage")
        rates = summ.get("rates")

        if cov is None or rates is None:
            # Fallback: aus function_level berechnen (Run -> FUT -> Codebase).
            cov, rates = _codebase_from_function_level(fl, by_cb[cb])

        out[cb] = {
            "line_coverage": cov.get("line_coverage"),
            "branch_coverage": cov.get("branch_coverage"),
            "compile_success": rates.get("compile_success_rate"),
            "linker_success": rates.get("linker_success_rate"),
            "pass_rate": rates.get("pass_rate"),
            "assert_error_rate": rates.get("assert_error_rate"),
            "runtime_error_rate": rates.get("runtime_error_rate"),
        }
    return out


def _codebase_from_function_level(function_level, futs):

    totals = {f: _bounded_totals(function_level[f]) for f in futs}
    run_ids = sorted({r.get("run") for f in futs for r in function_level[f]
                      if r.get("run")})
    line_den = sum(lt for lt, _ in totals.values() if lt is not None)
    branch_den = sum(bt for _, bt in totals.values() if bt is not None)

    lc_runs, bc_runs = [], []
    for rid in run_ids:
        ln = bn = 0
        for f in futs:
            r = next((x for x in function_level[f] if x.get("run") == rid), None)
            if r is None:
                continue
            lt, bt = totals[f]
            if lt is not None:
                ln += min(int(r.get("lines_covered") or 0), lt)
            if bt is not None:
                bn += min(int(r.get("branches_covered") or 0), bt)
        if line_den:
            lc_runs.append(100.0 * ln / line_den)
        if branch_den:
            bc_runs.append(100.0 * bn / branch_den)

    per_fut = []
    for f in futs:
        flags = [_run_flags(r) for r in function_level[f]]
        R = len(flags) or 1
        per_fut.append({
            "cer": 100.0 * sum(x["ce"] for x in flags) / R,
            "ler": 100.0 * sum(x["le"] for x in flags) / R,
            "rer": 100.0 * sum(x["re"] for x in flags) / R,
            "aer": 100.0 * sum(x["ae"] for x in flags) / R,
            "pr": 100.0 * sum(x["p"] for x in flags) / R,
        })
    rate = {k: _mean([m[k] for m in per_fut]) for k in RATE_KEYS}

    cov = {"line_coverage": _mean(lc_runs), "branch_coverage": _mean(bc_runs)}
    rates = {
        "compile_error_rate": rate["cer"],
        "linker_error_rate": rate["ler"],
        "runtime_error_rate": rate["rer"],
        "assert_error_rate": rate["aer"],
        "pass_rate": rate["pr"],
        "compile_success_rate": None if rate["cer"] is None else 100.0 - rate["cer"],
        "linker_success_rate": None if rate["ler"] is None else 100.0 - rate["ler"],
    }
    return cov, rates



def plot_model_comparison(per_model, he_per_model, out_path):
    """Diagramm 1: Modellvergleich je Metrik."""
    metrics = [
        ("Line Coverage", lambda s, m: s["coverage"].get("line_coverage")),
        ("Branch Coverage", lambda s, m: s["coverage"].get("branch_coverage")),
        ("Compilation Success Rate",
         lambda s, m: _100_minus(s["rates"].get("compile_error_rate"))),
        ("Linker Success Rate",
         lambda s, m: _100_minus(s["rates"].get("linker_error_rate"))),
        ("Runtime Error Rate",
         lambda s, m: s["rates"].get("runtime_error_rate")),
        ("Human Eval Rate",
         lambda s, m: he_per_model.get(m)),
    ]
    models = list(per_model)
    labels = [m for m, _ in metrics]
    x = range(len(labels))
    n = len(models)
    width = 0.8 / max(n, 1)

    fig, ax = plt.subplots(figsize=(13, 5.5))
    for i, model in enumerate(models):
        summary = per_model[model]["model_summary"]
        vals = [getter(summary, model) or 0.0 for _, getter in metrics]
        offs = [xi + (i - (n - 1) / 2) * width for xi in x]
        bars = ax.bar(offs, vals, width, label=model.capitalize())
        _annotate(ax, bars)

    ax.set_xticks(list(x))
    ax.set_xticklabels(labels, rotation=12, ha="right")
    ax.set_ylabel("Prozent (%)")
    ax.set_ylim(0, 105)
    ax.set_title("LLM-Performance je Metrik - gemittelt ueber alle Funktionen/Runs")
    ax.legend()
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"  -> Diagramm 1: {out_path}")


def plot_coverage_human_eval(per_model, he_per_model, out_path):
    """Diagramm: nur Line Coverage, Branch Coverage und Human Eval Rate auf
    Modellebene"""
    metrics = [
        ("Line Coverage", lambda s, m: s["coverage"].get("line_coverage")),
        ("Branch Coverage", lambda s, m: s["coverage"].get("branch_coverage")),
        ("Human Eval Rate", lambda s, m: he_per_model.get(m)),
    ]
    models = list(per_model)
    labels = [m for m, _ in metrics]
    x = range(len(labels))
    n = len(models)
    width = 0.8 / max(n, 1)

    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, model in enumerate(models):
        summary = per_model[model]["model_summary"]
        vals = [getter(summary, model) or 0.0 for _, getter in metrics]
        offs = [xi + (i - (n - 1) / 2) * width for xi in x]
        bars = ax.bar(offs, vals, width, label=model.capitalize())
        _annotate(ax, bars)

    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel("Prozent (%)")
    ax.set_ylim(0, 105)
    ax.set_title("Coverage & Human Eval - Modellvergleich")
    ax.legend()
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"  -> Diagramm Coverage & Human Eval: {out_path}")


def plot_filtered_coverage(per_model_filtered, out_path):
    """Diagramm: Line-/Branch-Coverage ueber die Modelle, ABER nur fuer
    erfolgreiche FUT-Laeufe (kompiliert, gelinkt, kein Runtime-Fehler)."""
    metrics = [("Line Coverage", "line_coverage"),
               ("Branch Coverage", "branch_coverage")]
    models = list(per_model_filtered)
    labels = [m for m, _ in metrics]
    x = range(len(labels))
    n = len(models)
    width = 0.8 / max(n, 1)

    fig, ax = plt.subplots(figsize=(8, 5.5))
    for i, model in enumerate(models):
        cov = per_model_filtered[model] or {}
        vals = [cov.get(key) or 0.0 for _, key in metrics]
        offs = [xi + (i - (n - 1) / 2) * width for xi in x]
        bars = ax.bar(offs, vals, width, label=model.capitalize())
        _annotate(ax, bars)

    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel("Prozent (%)")
    ax.set_ylim(0, 105)
    ax.set_title("Coverage (gefiltert) - nur kompiliert, gelinkt, ohne Runtime-Fehler")
    ax.legend()
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"  -> Diagramm gefilterte Coverage: {out_path}")



def plot_run_stability(model, breakdown, out_path):
    metrics = [
        ("Line Coverage", "line_coverage"),
        ("Branch Coverage", "branch_coverage"),
        ("Compilation Success Rate", "comp_success"),
        ("Linker Success Rate", "linker_success"),
        ("Pass Rate", "pass_rate"),
        ("Assertion Error Rate", "assert_error_rate"),
        ("Runtime Error Rate", "runtime_error_rate"),
    ]
    run_labels = [f"Run {i + 1}" for i in range(len(breakdown["runs"]))]
    x = range(len(metrics))
    n = len(run_labels)
    width = 0.8 / max(n, 1)

    fig, ax = plt.subplots(figsize=(13, 5.5))
    for i in range(n):
        vals = [(breakdown[key][i] if breakdown[key][i] is not None else 0.0)
                for _, key in metrics]
        offs = [xi + (i - (n - 1) / 2) * width for xi in x]
        bars = ax.bar(offs, vals, width, label=run_labels[i])
        _annotate(ax, bars)

    ax.set_xticks(list(x))
    ax.set_xticklabels([m for m, _ in metrics], rotation=15, ha="right")
    ax.set_ylabel("Prozent (%)")
    ax.set_ylim(0, 105)
    ax.set_title(f"Stabilitaet ueber die Runs - {model.capitalize()}")
    ax.legend(title="Lauf")
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"  -> Diagramm Run-Stabilitaet ({model}): {out_path}")


def plot_codebase_metrics(model, cb_metrics, out_path):
    """Diagramm: alle Metriken auf Codebase-Ebene"""
    metrics = [
        ("Line Coverage", "line_coverage"),
        ("Branch Coverage", "branch_coverage"),
        ("Compile Success Rate", "compile_success"),
        ("Linker Success Rate", "linker_success"),
        ("Pass Rate", "pass_rate"),
        ("Assertion Error Rate", "assert_error_rate"),
        ("Runtime Error Rate", "runtime_error_rate"),
        ("Human Eval Rate", "human_eval"),
    ]
    codebases = list(cb_metrics)
    x = range(len(metrics))
    n = len(codebases)
    width = 0.8 / max(n, 1)

    fig, ax = plt.subplots(figsize=(15, 5.5))
    for i, cb in enumerate(codebases):
        vals = [(cb_metrics[cb].get(key) if cb_metrics[cb].get(key) is not None
                 else 0.0) for _, key in metrics]
        offs = [xi + (i - (n - 1) / 2) * width for xi in x]
        bars = ax.bar(offs, vals, width, label=CODEBASE_LABELS.get(cb, cb))
        _annotate(ax, bars)

    ax.set_xticks(list(x))
    ax.set_xticklabels([m for m, _ in metrics], rotation=15, ha="right")
    ax.set_ylabel("Prozent (%)")
    ax.set_ylim(0, 105)
    ax.set_title(f"Metriken je Codebase - {model.capitalize()}")
    ax.legend(title="Codebase")
    ax.grid(axis="y", linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"  -> Diagramm Codebase-Metriken ({model}): {out_path}")



TABLE_COLUMNS = [
    "Lines Executed", "Lines", "Branches Executed", "Branches",
    "Compile Errors", "Linker Errors", "Runtime Errors",
    "Assertion Errors", "Passes",
]


def _fut_row(runs):
    """Aggregiert eine FUT ueber ihre Laeufe in die Tabellenspalten."""
    lt, bt = _bounded_totals(runs)
    lc = max((int(r.get("lines_covered") or 0) for r in runs), default=0)
    bc = max((int(r.get("branches_covered") or 0) for r in runs), default=0)
    if lt is not None:
        lc = min(lc, lt)
    if bt is not None:
        bc = min(bc, bt)
    return {
        "Lines Executed": lc,
        "Lines": lt or 0,
        "Branches Executed": bc,
        "Branches": bt or 0,
        "Compile Errors": sum(int(r.get("compile_error_status") or 0) for r in runs),
        "Linker Errors": sum(int(r.get("linker_error_status") or 0) for r in runs),
        "Runtime Errors": sum(int(r.get("runtime_errors") or 0) for r in runs),
        "Assertion Errors": sum(int(r.get("assertion_errors") or 0) for r in runs),
        "Passes": sum(int(r.get("tests_passed") or 0) for r in runs),
    }


def _codebase_subtotal(cb_summary):
    """Autoritative Subtotale aus codebase_level.json."""
    cov = cb_summary["codebase_summary"]["coverage_variables"]
    ex = cb_summary["codebase_summary"]["execution_totals"]
    return {
        "Lines Executed": cov.get("lines_covered", 0),
        "Lines": cov.get("lines_total", 0),
        "Branches Executed": cov.get("branches_covered", 0),
        "Branches": cov.get("branches_total", 0),
        "Compile Errors": ex.get("compile_errors", 0),
        "Linker Errors": ex.get("linker_errors", 0),
        "Runtime Errors": ex.get("runtime_errors", 0),
        "Assertion Errors": ex.get("assertion_errors", 0),
        "Passes": ex.get("tests_passed", 0),
    }


def build_table(data):
    """Tabellenzeilen (Liste von Dicts) fuer ein Modell."""
    fl = data["function_level"]
    cl = data["codebase_level"]
    ml = data["model_level"]["model_summary"]
    by_cb = _group_by_codebase(fl)


    without_values = set(ml.get("variables", {}).get("futs_without_values", []))

    ordered_cbs = sorted(by_cb, key=lambda c: (c == "unknown", c))
    rows = []
    grand = {c: 0 for c in TABLE_COLUMNS}

    for cb in ordered_cbs:
        for fut in sorted(by_cb[cb]):
            r = _fut_row(fl[fut])
            rows.append({
                "Region": fut,
                "Typ": "FUT",
                "FUT_ohne_Werte": "ja" if fut in without_values else "nein",
                **r,
            })
        if cb in cl:
            sub = _codebase_subtotal(cl[cb])
        else:
            sub = {c: sum(row[c] for row in rows if row["Typ"] == "FUT")
                   for c in TABLE_COLUMNS}
        rows.append({
            "Region": CODEBASE_LABELS.get(cb, cb),
            "Typ": "Codebase-Summe",
            "FUT_ohne_Werte": "",
            **sub,
        })
        for c in TABLE_COLUMNS:
            grand[c] += sub[c]

    total_futs = ml.get("variables", {}).get("total_futs", len(fl))
    rows.append({
        "Region": f"Alle Funktionen (n={total_futs})",
        "Typ": "Gesamt",
        "FUT_ohne_Werte": f"{len(without_values)} ohne Werte",
        **grand,
    })
    return rows


def write_table_csv(rows, out_path):
    fieldnames = ["Region", "Typ"] + TABLE_COLUMNS + ["FUT_ohne_Werte"]
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for row in rows:
            w.writerow({k: row.get(k, "") for k in fieldnames})
    print(f"  -> Tabelle (CSV): {out_path}")




def _100_minus(v):
    return None if v is None else 100.0 - v


def _annotate(ax, bars):
    for b in bars:
        h = b.get_height()
        ax.annotate(f"{h:.1f}", (b.get_x() + b.get_width() / 2, h),
                    xytext=(0, 2), textcoords="offset points",
                    ha="center", va="bottom", fontsize=7)



def main():
    ap = argparse.ArgumentParser(description="Visualisierung der LLM-Eval-Metriken")
    ap.add_argument("models", nargs="*", default=None,
                    help="Modellnamen (Default: claude qwen)")
    ap.add_argument("--metrics-dir", default=None,
                    help="output_metrics-Verzeichnis (sonst aus config.yaml)")
    ap.add_argument("--out", default=None,
                    help="Ausgabeordner fuer Plots/CSV (Default: <metrics>/plots)")
    args = ap.parse_args()

    models = args.models or ["claude", "qwen"]
    metrics_dir = resolve_metrics_dir(args.metrics_dir)
    out_dir = Path(args.out) if args.out else metrics_dir / "plots"
    out_dir.mkdir(parents=True, exist_ok=True)


    human_eval = load_human_eval(metrics_dir)

    loaded = {}
    for model in models:
        print(f"\n=== Lade {model} ===")
        data = load_model_files(metrics_dir, model)
        if data:
            loaded[model] = data

    if not loaded:
        raise SystemExit("Keine Modelldaten gefunden - nichts zu visualisieren.")

    he_per_model = {m: he_model_pct(human_eval, m, d["function_level"])
                    for m, d in loaded.items()}


    per_model = {m: d["model_level"] for m, d in loaded.items()}
    plot_model_comparison(per_model, he_per_model,
                          out_dir / "diagramm1_modellvergleich.png")


    plot_coverage_human_eval(per_model, he_per_model,
                             out_dir / "diagramm_coverage_human_eval.png")


    per_model_filtered = {
        m: d["model_level"].get("model_summary", {}).get("coverage_filtered", {})
        for m, d in loaded.items()
    }
    plot_filtered_coverage(per_model_filtered,
                           out_dir / "diagramm_coverage_gefiltert.png")


    for model, data in loaded.items():
        breakdown = per_run_breakdown(data["function_level"])
        plot_run_stability(model, breakdown,
                           out_dir / f"diagramm2_run_stabilitaet_{model}.png")

        cb_metrics = collect_codebase_metrics(data)

        he_cb = he_codebase_pct(human_eval, model, data["function_level"])
        for cb in cb_metrics:
            cb_metrics[cb]["human_eval"] = he_cb.get(cb)
        plot_codebase_metrics(model, cb_metrics,
                              out_dir / f"diagramm3_codebase_metriken_{model}.png")

        rows = build_table(data)
        write_table_csv(rows, out_dir / f"tabelle_{model}.csv")

    print(f"\nFertig. Alle Ausgaben in: {out_dir}")


if __name__ == "__main__":
    main()
