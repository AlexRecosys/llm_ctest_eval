
import json
from pathlib import Path

import yaml
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd


# --------------------------------------------------------------------------
# Konfiguration / Konstanten
# --------------------------------------------------------------------------
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


# Reihenfolge + Anzeigenamen der abhaengigen Variablen (alle in Prozent).
METRICS = [
    "Line Coverage",
    "Branch Coverage",
    "Compile Error Rate",
    "Linker Error Rate",
    "Runtime Error Rate",
    "Assertion Error Rate",
    "Pass Rate",
]

# Optionale schoenere Beschriftung; faellt sonst auf den Rohnamen zurueck.
MODEL_DISPLAY = {"claude": "Claude (Cloud)", "qwen": "Qwen (On-Premise)"}
CODEBASE_DISPLAY = {"cjson_codebase": "cJSON", "csv_codebase": "CSV"}

# Farbpaletten (farbenblind-freundlich).
MODEL_COLORS = ["#0072B2", "#D55E00", "#009E73", "#CC79A7"]
CODEBASE_COLORS = ["#0072B2", "#E69F00", "#009E73", "#CC79A7", "#56B4E9"]
METRIC_COLORS = ["#0072B2", "#56B4E9", "#D55E00", "#E69F00",
                 "#CC79A7", "#F0A3A3", "#009E73"]

# Spalten der FUT-Tabelle (Rohzahlen, Mittel ueber die Laeufe).
TABLE_COLUMNS = ["Lines Executed", "Lines", "Branches Executed", "Branches",
                 "Compile Errors", "Linker Errors", "Runtime Errors",
                 "Assertion Errors", "Passes"]


def disp_model(m):
    return MODEL_DISPLAY.get(m, m)


def disp_codebase(c):
    return CODEBASE_DISPLAY.get(c, c)


# --------------------------------------------------------------------------
# Daten extrahieren
# --------------------------------------------------------------------------
def _metric_values(d):
    """7 Metrik-Prozentwerte aus einem Summary-Dict ziehen.

    Funktioniert sowohl fuer model_summary['mean_over_runs'] (flache
    *_percent-Keys) als auch fuer codebase_summary[cb] (verschachtelte
    coverage-Dicts).
    """
    def cov(flat, nested):
        if d.get(flat) is not None:
            return d[flat]
        c = d.get(nested)
        if isinstance(c, dict):
            return c.get("percent")
        return None

    return {
        "Line Coverage": cov("line_coverage_percent", "line_coverage"),
        "Branch Coverage": cov("branch_coverage_percent", "branch_coverage"),
        "Compile Error Rate": d.get("compile_error_rate"),
        "Linker Error Rate": d.get("linker_error_rate"),
        "Runtime Error Rate": d.get("runtime_error_rate"),
        "Assertion Error Rate": d.get("assert_error_rate"),
        "Pass Rate": d.get("pass_rate"),
    }


def model_metrics(global_summary):
    """{modell: {metrik: wert}} – Modell-Mittel ueber alle Funktionen."""
    out = {}
    for model, blk in global_summary["per_model"].items():
        out[model] = _metric_values(blk["model_summary"]["mean_over_runs"])
    return out


def codebase_metrics_over_models(global_summary):
    """{codebase: {metrik: wert}} – pro Codebase Mittel ueber die Modelle."""
    acc = {}  # codebase -> metrik -> [werte]
    for blk in global_summary["per_model"].values():
        for cb, summ in blk["codebase_summary"].items():
            vals = _metric_values(summ)
            for k, v in vals.items():
                if v is not None:
                    acc.setdefault(cb, {}).setdefault(k, []).append(v)
    out = {}
    for cb, metr in acc.items():
        out[cb] = {k: (round(sum(vs) / len(vs), 2) if vs else None)
                   for k, vs in metr.items()}
    return out


# --------------------------------------------------------------------------
# Plot-Helfer
# --------------------------------------------------------------------------
def _grouped_bar(ax, group_labels, series, colors):
    """series: dict name -> liste von werten (auf group_labels ausgerichtet)."""
    n = len(series)
    x = np.arange(len(group_labels))
    width = 0.8 / max(n, 1)
    for i, (name, vals) in enumerate(series.items()):
        offset = (i - (n - 1) / 2) * width
        safe = [v if v is not None else 0 for v in vals]
        bars = ax.bar(x + offset, safe, width, label=name,
                      color=colors[i % len(colors)], edgecolor="white", linewidth=0.5)
        ax.bar_label(bars, fmt="%.1f", fontsize=7, padding=2)
    ax.set_xticks(x)
    ax.set_xticklabels(group_labels, rotation=25, ha="right")
    ax.set_ylim(0, 108)
    ax.set_ylabel("Prozent (%)")
    ax.grid(axis="y", alpha=0.3)
    ax.legend(frameon=False)


def _save(fig, out_dir, name):
    for ext in ("png", "pdf"):
        fig.savefig(out_dir / f"{name}.{ext}", dpi=160, bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------
# Diagramm 1: Gesamtuebersicht je Modell
# --------------------------------------------------------------------------
def fig_model_overview(mm, out_dir):
    models = list(mm)
    fig, axes = plt.subplots(1, len(models),
                             figsize=(6.2 * len(models), 5), sharey=True)
    if len(models) == 1:
        axes = [axes]
    for ax, m in zip(axes, models):
        vals = [mm[m].get(k) or 0 for k in METRICS]
        bars = ax.bar(range(len(METRICS)), vals, color=METRIC_COLORS,
                      edgecolor="white", linewidth=0.5)
        ax.bar_label(bars, fmt="%.1f", fontsize=8, padding=2)
        ax.set_title(disp_model(m), fontweight="bold")
        ax.set_xticks(range(len(METRICS)))
        ax.set_xticklabels(METRICS, rotation=35, ha="right")
        ax.set_ylim(0, 108)
        ax.grid(axis="y", alpha=0.3)
    axes[0].set_ylabel("Prozent (%)")
    fig.suptitle("Gesamtuebersicht je Modell (Mittel ueber alle Funktionen)",
                 fontweight="bold")
    _save(fig, out_dir, "fig1_model_overview")


# --------------------------------------------------------------------------
# Diagramm 2: LLM-Performance (Mittel ueber Codebases), Modelle nebeneinander
# --------------------------------------------------------------------------
def fig_llm_performance(mm, out_dir):
    series = {disp_model(m): [mm[m].get(k) for k in METRICS] for m in mm}
    fig, ax = plt.subplots(figsize=(12, 5.5))
    _grouped_bar(ax, METRICS, series, MODEL_COLORS)
    ax.set_title("LLM-Performance je Metrik (Mittel ueber alle Codebases)",
                 fontweight="bold")
    _save(fig, out_dir, "fig2_llm_performance")


# --------------------------------------------------------------------------
# Diagramm 3: Codebase-Performance (Mittel ueber LLMs), Codebases nebeneinander
# --------------------------------------------------------------------------
def fig_codebase_performance(cm, out_dir):
    codebases = list(cm)
    series = {disp_codebase(c): [cm[c].get(k) for k in METRICS] for c in codebases}
    fig, ax = plt.subplots(figsize=(12, 5.5))
    _grouped_bar(ax, METRICS, series, CODEBASE_COLORS)
    ax.set_title("Codebase-Performance je Metrik (Mittel ueber alle LLMs)",
                 fontweight="bold")
    _save(fig, out_dir, "fig3_codebase_performance")


# --------------------------------------------------------------------------
# Tabelle: FUT -> Codebase-Summe -> Gesamtsumme (pro Modell)
# --------------------------------------------------------------------------
def _row_from_mean(mean):
    lc = mean.get("line_coverage") or {}
    bc = mean.get("branch_coverage") or {}
    return {
        "Lines Executed": lc.get("covered", 0.0) or 0.0,
        "Lines": lc.get("total", 0.0) or 0.0,
        "Branches Executed": bc.get("covered", 0.0) or 0.0,
        "Branches": bc.get("total", 0.0) or 0.0,
        "Compile Errors": mean.get("compile_errors", 0.0) or 0.0,
        "Linker Errors": mean.get("linker_errors", 0.0) or 0.0,
        "Runtime Errors": mean.get("runtime_errors", 0.0) or 0.0,
        "Assertion Errors": mean.get("assertion_errors", 0.0) or 0.0,
        "Passes": mean.get("passed", 0.0) or 0.0,
    }


def build_table(functions_report):
    """DataFrame: FUT-Zeilen, je Codebase eine Summenzeile, am Ende Gesamtsumme."""
    funcs = functions_report["functions"]
    by_cb = {}
    for fut, blk in funcs.items():
        by_cb.setdefault(blk.get("codebase", "unknown"), []).append((fut, blk))

    rows, index = [], []
    grand = {c: 0.0 for c in TABLE_COLUMNS}

    for cb in sorted(by_cb):
        cb_sum = {c: 0.0 for c in TABLE_COLUMNS}
        for fut, blk in sorted(by_cb[cb]):
            r = _row_from_mean(blk["mean"])
            rows.append(r)
            index.append(fut)
            for c in TABLE_COLUMNS:
                cb_sum[c] += r[c]
                grand[c] += r[c]
        rows.append(cb_sum)
        index.append(f"== {disp_codebase(cb)} (Summe) ==")

    rows.append(grand)
    index.append("== Alle Funktionen (Summe) ==")

    df = pd.DataFrame(rows, index=index, columns=TABLE_COLUMNS).round(2)
    return df


def render_table_png(df, out_path, title):
    n = len(df)
    fig, ax = plt.subplots(figsize=(13, 0.5 + 0.42 * n))
    ax.axis("off")
    ax.set_title(title, fontweight="bold", pad=12)
    tbl = ax.table(cellText=df.values, rowLabels=df.index,
                   colLabels=df.columns, cellLoc="center", loc="center")
    tbl.auto_set_font_size(False)
    tbl.set_fontsize(8)
    tbl.scale(1, 1.3)
    # Summenzeilen hervorheben.
    for (r, _), cell in tbl.get_celld().items():
        if r == 0:  # Header
            cell.set_facecolor("#0072B2")
            cell.set_text_props(color="white", fontweight="bold")
        elif r - 1 < n and str(df.index[r - 1]).startswith("=="):
            cell.set_facecolor("#E8EEF5")
            cell.set_text_props(fontweight="bold")
    fig.savefig(out_path, dpi=160, bbox_inches="tight")
    plt.close(fig)


def write_tables(metrics_root, models, out_dir):
    for model in models:
        ffile = metrics_root / f"{model}_modell" / "metric_functions.json"
        if not ffile.exists():
            print(f"  [WARN] {ffile} fehlt – Tabelle uebersprungen.")
            continue
        report = json.loads(ffile.read_text())
        df = build_table(report)

        df.to_csv(out_dir / f"table_{model}.csv")
        try:
            df.to_excel(out_dir / f"table_{model}.xlsx")
        except Exception as e:  # openpyxl evtl. nicht installiert
            print(f"  [WARN] xlsx nicht geschrieben ({e})")
        render_table_png(df, out_dir / f"table_{model}.png",
                         f"FUT-Detailtabelle – {disp_model(model)}")
        print(f"  -> Tabelle {model}: csv/xlsx/png")


# --------------------------------------------------------------------------
# Orchestrierung
# --------------------------------------------------------------------------
def run_analysis(metrics_root, out_dir):
    metrics_root = Path(metrics_root)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    gfile = metrics_root / "global_metric_summary.json"
    if not gfile.exists():
        raise FileNotFoundError(
            f"{gfile} fehlt – bitte zuerst 02_compute_metrics.py ausfuehren.")
    glob = json.loads(gfile.read_text())

    mm = model_metrics(glob)
    cm = codebase_metrics_over_models(glob)

    fig_model_overview(mm, out_dir)
    print("  -> fig1_model_overview")
    fig_llm_performance(mm, out_dir)
    print("  -> fig2_llm_performance")
    fig_codebase_performance(cm, out_dir)
    print("  -> fig3_codebase_performance")

    write_tables(metrics_root, list(glob["per_model"].keys()), out_dir)
    print(f"\n-> Analyse abgeschlossen: {out_dir}")


def main():
    cfg = load_config()
    metrics_root = Path(cfg["paths"]["output_metrics"])
    out_dir = metrics_root / "analysis"
    run_analysis(metrics_root, out_dir)


if __name__ == "__main__":
    main()
