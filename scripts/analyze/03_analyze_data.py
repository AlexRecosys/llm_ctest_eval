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
    """Findet das output_metrics-Verzeichnis: erst explizit (CLI), sonst
    ueber config.yaml wie in 02_compute_metrics.py."""
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
    """Laedt die drei JSONs eines Modells. Gibt None zurueck, wenn das
    Modellverzeichnis oder eine Pflichtdatei fehlt."""
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


def _pct(num, den):
    return 100.0 * num / den if den else None


def _mean(vals):
    vals = [v for v in vals if v is not None]
    return sum(vals) / len(vals) if vals else None


def _bounded_totals(runs):
    """Keep Bounded: statische Totals einer FUT (max ueber die Laeufe, da die
    function_level-Datensaetze lines_total/branches_total je Lauf tragen)."""
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
    """Berechnet je Lauf (Run 1/2/3) die modellweiten Kennzahlen.

    Coverage: LC_run = Sum_f min(LC_{f,run}, LT_f) / Sum_f LT_f * 100.
    Raten:    pro FUT die Lauf-Rate, dann Makro-Mittel FUT -> Codebase -> Modell.
    Success-Raten = 100 - Fehlerrate.
    """
    run_ids = sorted({r.get("run") for runs in function_level.values()
                      for r in runs if r.get("run")})
    futs = list(function_level)
    totals = {f: _bounded_totals(function_level[f]) for f in futs}
    by_cb = _group_by_codebase(function_level)

    line_den = sum(lt for lt, _ in totals.values() if lt is not None)
    branch_den = sum(bt for _, bt in totals.values() if bt is not None)

    out = {"runs": [], "line_coverage": [], "branch_coverage": [],
           "comp_success": [], "linker_success": [],
           "pass_rate": [], "assert_error_rate": []}
    
    #"runtime_error_rate": [],

    for rid in run_ids:
        # --- Coverage (LC-Modell, 0%-Strafe fuer fehlende Laeufe) ---
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

        # --- Raten: Makro-Mittel je Lauf ---
        cb_rates = {}
        for cb, cb_futs in by_cb.items():
            per_fut = []
            for f in cb_futs:
                r = next((x for x in function_level[f] if x.get("run") == rid), None)
                if r is None:
                    per_fut.append({k: None for k in RATE_KEYS})
                    continue
                tests = int(r.get("tests_total") or 0)
                per_fut.append({
                    "cer": 100.0 * int(r.get("compile_error_status") or 0),
                    "ler": 100.0 * int(r.get("linker_error_status") or 0),
                    "rer": _pct(int(r.get("runtime_errors") or 0), tests),
                    "aer": _pct(int(r.get("assertion_errors") or 0), tests),
                    "pr": _pct(int(r.get("tests_passed") or 0), tests),
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
        #out["runtime_error_rate"].append(model_rate["rer"])
        out["assert_error_rate"].append(model_rate["aer"])
    return out



def plot_model_comparison(per_model, out_path):
    metrics = [
        ("Line Coverage", lambda s: s["coverage"].get("line_coverage")),
        ("Branch Coverage", lambda s: s["coverage"].get("branch_coverage")),
        ("Compilation Success Rate",
         lambda s: _100_minus(s["rates"].get("compile_error_rate"))),
        ("Linker Success Rate",
         lambda s: _100_minus(s["rates"].get("linker_error_rate"))),
    ]
    models = list(per_model)
    labels = [m for m, _ in metrics]
    x = range(len(labels))
    n = len(models)
    width = 0.8 / max(n, 1)

    fig, ax = plt.subplots(figsize=(10, 5.5))
    for i, model in enumerate(models):
        summary = per_model[model]["model_summary"]
        vals = [getter(summary) or 0.0 for _, getter in metrics]
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



def plot_run_stability(model, breakdown, out_path):
    metrics = [
        ("Line Coverage", "line_coverage"),
        ("Branch Coverage", "branch_coverage"),
        ("Compilation Success Rate", "comp_success"),
        ("Linker Success Rate", "linker_success"),
        ("Pass Rate", "pass_rate"),
        # ("Runtime Error Rate", "runtime_error_rate"),
        ("Assertion Error Rate", "assert_error_rate"),
    ]
    run_labels = [f"Run {i + 1}" for i in range(len(breakdown["runs"]))]
    x = range(len(metrics))
    n = len(run_labels)
    width = 0.8 / max(n, 1)

    fig, ax = plt.subplots(figsize=(12, 5.5))
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
    """Erzeugt die Tabellenzeilen (Liste von Dicts) fuer ein Modell."""
    fl = data["function_level"]
    cl = data["codebase_level"]
    ml = data["model_level"]["model_summary"]
    by_cb = _group_by_codebase(fl)

    # FUTs ohne Werte (modellweit) zum Markieren
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
        # Subtotal: bevorzugt autoritativ aus codebase_level.json,
        # sonst Summe der FUT-Zeilen dieses Clusters.
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

    loaded = {}
    for model in models:
        print(f"\n=== Lade {model} ===")
        data = load_model_files(metrics_dir, model)
        if data:
            loaded[model] = data

    if not loaded:
        raise SystemExit("Keine Modelldaten gefunden - nichts zu visualisieren.")

    # Diagramm 1: Modellvergleich
    per_model = {m: d["model_level"] for m, d in loaded.items()}
    plot_model_comparison(per_model, out_dir / "diagramm1_modellvergleich.png")

    # Diagramm 2
    for model, data in loaded.items():
        breakdown = per_run_breakdown(data["function_level"])
        plot_run_stability(model, breakdown,
                           out_dir / f"diagramm2_run_stabilitaet_{model}.png")
        rows = build_table(data)
        write_table_csv(rows, out_dir / f"tabelle_{model}.csv")

    print(f"\nFertig. Alle Ausgaben in: {out_dir}")


if __name__ == "__main__":
    main()
