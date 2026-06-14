#!/usr/bin/env python3
"""03_visualize_results.py

Reines Visualisierungsskript fuer die in 01_measure_coverage.py /
02_compute_metrics.py gewonnenen Daten. Es findet KEINE Neuberechnung der
Kennzahlen statt - das Skript liest die bereits erzeugten JSON-Dateien und
stellt sie deskriptiv dar. Die inhaltliche Interpretation erfolgt im
Auswertungskapitel.

Visualisiert werden vier Ebenen:

  Ebene 1  Modellvergleich (Claude vs. Qwen / Cloud vs. On-Premise) auf den
           abhaengigen Variablen: Line Coverage, Branch Coverage, Human Eval
           (Platzhalter, noch nicht erhoben), Compilation Error Rate,
           Linker Error Rate.
  Ebene 2  Modellvergleich auf ergaenzenden Performance-Metriken:
           Pass Rate, Assertion Error Rate, Runtime Error Rate.
  Ebene 3  Codebase-Vergleich (cJSON vs. CSV) auf den abhaengigen Variablen.
  Ebene 4  Codebase-Vergleich auf ergaenzenden Metriken.

Jede Gruppen-Gegenueberstellung wird sowohl als Balkendiagramm (aggregierter
Punktwert) als auch - wo sinnvoll (stetige Metriken) - als Boxplot ueber die
Verteilung der einzelnen Laeufe gezeigt.

Datenquellen
------------
  * model_comparison.json   -> offizielle, im Skript 02 berechneten
    Modell-Aggregate (Makro-Mittel). Quelle fuer die MODELL-Balken.
  * <model>_modell/function_level.json -> Lauf-Datensaetze je FUT. Quelle fuer
    ALLE Boxplots (Verteilungen) sowie fuer die Codebase-Aggregate, da fuer
    die Codebase-Ebene keine offizielle Raten-Datei existiert.

Aufruf:  python3 03_visualize_results.py [modell1 modell2 ...]
         (Default: claude qwen)
"""

import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd
import yaml

import matplotlib
matplotlib.use("Agg")          # rein dateibasiert, kein Display noetig
import matplotlib.pyplot as plt
from matplotlib.patches import Patch


# ---------------------------------------------------------------------------
# Darstellungs-Konfiguration
# ---------------------------------------------------------------------------

# Anzeigenamen
MODEL_LABELS = {"claude": "Claude (Cloud)", "qwen": "Qwen (On-Premise)"}
CODEBASE_LABELS = {"cjson_codebase": "cJSON", "csv_codebase": "CSV"}

# Farben (colorblind-freundliche Palette)
MODEL_COLORS = {"claude": "#4C72B0", "qwen": "#DD8452"}
CODEBASE_COLORS = {"cjson_codebase": "#55A868", "csv_codebase": "#8172B3"}

PLACEHOLDER_COLOR = "#BBBBBB"

# Abhaengige Variablen (Kern der Forschungsfrage). human_eval ist noch nicht
# erhoben -> als Platzhalter gefuehrt (Wert None).
DEPENDENT_VARS = [
    ("line_coverage",       "Line\nCoverage"),
    ("branch_coverage",     "Branch\nCoverage"),
    ("human_eval",          "Human\nEval"),
    ("compile_error_rate",  "Compilation\nError Rate"),
    ("linker_error_rate",   "Linker\nError Rate"),
]

# Ergaenzende Metriken (interessant, aber keine abhaengigen Variablen)
ADDITIONAL_VARS = [
    ("pass_rate",          "Pass Rate"),
    ("assert_error_rate",  "Assertion\nError Rate"),
    ("runtime_error_rate", "Runtime\nError Rate"),
]

# Stetige Metriken, die als Boxplot ueber die Verteilung sinnvoll sind.
# (Compile-/Linker-Fehler sind binaer je Lauf -> nur als Balken dargestellt.)
BOX_COVERAGE = [("line_coverage", "Line Coverage (%)"),
                ("branch_coverage", "Branch Coverage (%)")]
BOX_RATES = [("pass_rate", "Pass Rate (%)"),
             ("assert_error_rate", "Assertion Error Rate (%)"),
             ("runtime_error_rate", "Runtime Error Rate (%)")]

plt.rcParams.update({
    "figure.dpi": 110,
    "savefig.dpi": 300,
    "font.size": 11,
    "axes.titlesize": 12,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "grid.linestyle": "--",
})


# ---------------------------------------------------------------------------
# Konfiguration / Pfade  (analog zu 02_compute_metrics.py)
# ---------------------------------------------------------------------------

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


def figures_dir(cfg):
    """Ausgabeverzeichnis fuer die Diagramme (config -> 'plots', sonst
    <output_metrics>/figures)."""
    d = cfg["paths"].get("plots") or str(
        Path(cfg["paths"]["output_metrics"]) / "figures")
    Path(d).mkdir(parents=True, exist_ok=True)
    return Path(d)


# ---------------------------------------------------------------------------
# Daten laden
# ---------------------------------------------------------------------------

def load_model_comparison(cfg):
    f = Path(cfg["paths"]["output_metrics"]) / "model_comparison.json"
    if not f.exists():
        print(f"  ! model_comparison.json nicht gefunden ({f})")
        return None
    return json.loads(f.read_text())


def _safe_pct(num, den):
    return 100.0 * num / den if den else np.nan


def load_run_dataframe(models, cfg):
    """Liest die function_level.json aller verfuegbaren Modelle und baut einen
    DataFrame mit einem Datensatz pro (Modell, FUT, Lauf) sowie den daraus
    abgeleiteten, ungerundeten Metriken.

    Konvention (konsistent zur Methodik in Skript 02):
      * Coverage je Lauf = covered / total * 100. Nicht kompilierende/linkende
        Laeufe haben covered = 0 bei vollem statischem Nenner -> 0 % (0%-Strafe).
      * Raten je Lauf = count / tests_total * 100; Laeufe ohne Tests liefern
        NaN und fallen aus Verteilung/Mittel heraus.
      * compile_error / linker_error sind binaere Lauf-Flags (0 / 100 %).
    """
    rows = []
    for model in models:
        f = (Path(cfg["paths"]["output_metrics"]) / f"{model}_modell"
             / "function_level.json")
        if not f.exists():
            print(f"  ! function_level.json fehlt fuer '{model}' ({f}) "
                  f"-> Boxplots/Codebase-Aggregate ohne dieses Modell.")
            continue
        data = json.loads(f.read_text())
        for fut, runs in data.items():
            for r in runs:
                tt = int(r.get("tests_total") or 0)
                rows.append({
                    "model": model,
                    "fut": fut,
                    "codebase": r.get("codebase", "unknown"),
                    "run": r.get("run"),
                    "line_coverage": _safe_pct(r.get("lines_covered") or 0,
                                               r.get("lines_total") or 0),
                    "branch_coverage": _safe_pct(r.get("branches_covered") or 0,
                                                  r.get("branches_total") or 0),
                    "pass_rate": _safe_pct(r.get("tests_passed") or 0, tt),
                    "assert_error_rate": _safe_pct(r.get("assertion_errors") or 0, tt),
                    "runtime_error_rate": _safe_pct(r.get("runtime_errors") or 0, tt),
                    "compile_error_rate": 100.0 * int(r.get("compile_error_status") or 0),
                    "linker_error_rate": 100.0 * int(r.get("linker_error_status") or 0),
                    # Rohwerte fuer summenbasierte Coverage-Aggregation
                    "lines_total": r.get("lines_total") or 0,
                    "lines_covered": r.get("lines_covered") or 0,
                    "branches_total": r.get("branches_total") or 0,
                    "branches_covered": r.get("branches_covered") or 0,
                })
    return pd.DataFrame(rows)


# ---------------------------------------------------------------------------
# Aggregation fuer die Balken
# ---------------------------------------------------------------------------

def model_bar_values(comparison, metric):
    """Offizielle Modell-Aggregate aus model_comparison.json.
    Gibt {model: wert|None} zurueck. human_eval -> None (Platzhalter)."""
    out = {}
    for model, summ in comparison["per_model"].items():
        if metric == "human_eval":
            out[model] = None
        elif metric in ("line_coverage", "branch_coverage"):
            out[model] = summ["coverage"].get(metric)
        else:
            key = {"compile_error_rate": "compile_error_rate",
                   "linker_error_rate": "linker_error_rate",
                   "pass_rate": "pass_rate",
                   "assert_error_rate": "assert_error_rate",
                   "runtime_error_rate": "runtime_error_rate"}[metric]
            out[model] = summ["rates"].get(key)
    return out


def codebase_bar_values(df, metric):
    """Deskriptive Codebase-Aggregate aus den Lauf-Datensaetzen (beide Modelle
    gepoolt). Coverage summenbasiert (sum covered / sum total), Raten als
    Mittel der gueltigen Lauf-Raten. human_eval -> None."""
    out = {}
    for cb, g in df.groupby("codebase"):
        if metric == "human_eval":
            out[cb] = None
        elif metric == "line_coverage":
            out[cb] = _safe_pct(g["lines_covered"].sum(), g["lines_total"].sum())
        elif metric == "branch_coverage":
            out[cb] = _safe_pct(g["branches_covered"].sum(), g["branches_total"].sum())
        else:
            out[cb] = g[metric].mean(skipna=True)
        if isinstance(out[cb], float) and np.isnan(out[cb]):
            out[cb] = None
    return out


# ---------------------------------------------------------------------------
# Plot-Bausteine
# ---------------------------------------------------------------------------

def _annotate(ax, x, value, placeholder=False):
    if placeholder:
        ax.text(x, 3, "n/a", ha="center", va="bottom", fontsize=8,
                rotation=90, color="#555555")
    elif value is not None:
        ax.text(x, value + 1.0, f"{value:.1f}", ha="center", va="bottom",
                fontsize=8.5)


def grouped_bars(metrics, group_values, group_order, group_labels,
                 group_colors, title, path, ylabel="Wert (%)", ylim=(0, 105)):
    """metrics: Liste (key, anzeige_label).
    group_values: {group: {metric_key: wert|None}}.
    Zeichnet gruppierte Balken; None-Werte werden als schraffierter
    'n/a'-Platzhalter dargestellt (z. B. Human Eval)."""
    x = np.arange(len(metrics))
    n = len(group_order)
    width = 0.8 / n
    fig, ax = plt.subplots(figsize=(1.7 * len(metrics) + 2, 5))

    for i, g in enumerate(group_order):
        offset = (i - (n - 1) / 2) * width
        pos = x + offset
        vals = [group_values[g].get(k) for k, _ in metrics]
        for p, v in zip(pos, vals):
            if v is None:
                ax.bar(p, ylim[1], width, color=PLACEHOLDER_COLOR, alpha=0.18,
                       hatch="//", edgecolor=PLACEHOLDER_COLOR)
                _annotate(ax, p, None, placeholder=True)
            else:
                ax.bar(p, v, width, color=group_colors[g], edgecolor="white",
                       label=group_labels[g] if p == pos[0] else None)
                _annotate(ax, p, v)

    ax.set_xticks(x)
    ax.set_xticklabels([lbl for _, lbl in metrics])
    ax.set_ylabel(ylabel)
    ax.set_ylim(*ylim)
    ax.set_title(title)
    # eindeutige Legende
    handles = [Patch(facecolor=group_colors[g], label=group_labels[g])
               for g in group_order]
    if any(group_values[g].get(k) is None for g in group_order
           for k, _ in metrics):
        handles.append(Patch(facecolor=PLACEHOLDER_COLOR, alpha=0.18,
                             hatch="//", label="noch nicht erhoben"))
    ax.legend(handles=handles, frameon=False, ncol=min(len(handles), 3),
              loc="upper center", bbox_to_anchor=(0.5, -0.12))
    fig.tight_layout()
    fig.savefig(path, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {path.name}")


def grouped_boxplots(df, metrics, group_col, group_order, group_labels,
                     group_colors, title, path, ylim=(-2, 105)):
    """Ein Subplot je Metrik; je Subplot ein Box-and-Whisker pro Gruppe,
    ueberlagert mit gejitterten Rohpunkten (Transparenz der Verteilung).
    Gruppen ohne gueltige Werte werden uebersprungen."""
    present = [g for g in group_order
               if not df[df[group_col] == g].empty]
    if not present:
        print(f"  ! keine Daten fuer Boxplot '{title}' -> uebersprungen")
        return

    n = len(metrics)
    fig, axes = plt.subplots(1, n, figsize=(3.6 * n, 4.8), squeeze=False)
    rng = np.random.default_rng(0)

    for ax, (key, label) in zip(axes[0], metrics):
        data, positions, used = [], [], []
        for j, g in enumerate(present):
            vals = df.loc[df[group_col] == g, key].dropna().values
            data.append(vals)
            positions.append(j + 1)
            used.append(g)
        bp = ax.boxplot(data, positions=positions, widths=0.55,
                        patch_artist=True, showmeans=True, showfliers=False,
                        medianprops=dict(color="black", linewidth=1.4),
                        meanprops=dict(marker="D", markerfacecolor="white",
                                       markeredgecolor="black", markersize=6))
        for patch, g in zip(bp["boxes"], used):
            patch.set_facecolor(group_colors[g])
            patch.set_alpha(0.55)
        # Rohpunkte als Jitter
        for pos, vals, g in zip(positions, data, used):
            if len(vals):
                jit = rng.uniform(-0.12, 0.12, size=len(vals))
                ax.scatter(np.full(len(vals), pos) + jit, vals, s=18,
                           color=group_colors[g], edgecolor="white",
                           linewidth=0.4, alpha=0.8, zorder=3)
                ax.text(pos, ylim[0] + 2, f"n={len(vals)}", ha="center",
                        va="bottom", fontsize=7.5, color="#555555")
        ax.set_xticks(positions)
        ax.set_xticklabels([group_labels[g] for g in used], rotation=12,
                           ha="right")
        ax.set_title(label)
        ax.set_ylim(*ylim)
    axes[0][0].set_ylabel("Wert (%)")
    fig.suptitle(title, fontsize=13)
    # gemeinsame Legende (Median / Mittelwert)
    handles = [plt.Line2D([0], [0], color="black", lw=1.4, label="Median"),
               plt.Line2D([0], [0], marker="D", color="black",
                          markerfacecolor="white", lw=0, label="Mittelwert")]
    fig.legend(handles=handles, frameon=False, ncol=2,
               loc="lower center", bbox_to_anchor=(0.5, -0.02))
    fig.tight_layout(rect=(0, 0.04, 1, 0.96))
    fig.savefig(path, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {path.name}")


# ---------------------------------------------------------------------------
# Deskriptive Kennzahlen exportieren (fuer Zitierbarkeit im Text)
# ---------------------------------------------------------------------------

def export_descriptive_stats(df, path):
    out = {"by_model": {}, "by_codebase": {}}
    metric_cols = ["line_coverage", "branch_coverage", "pass_rate",
                   "assert_error_rate", "runtime_error_rate",
                   "compile_error_rate", "linker_error_rate"]
    for col_group, key in [("model", "by_model"), ("codebase", "by_codebase")]:
        for grp, g in df.groupby(col_group):
            out[key][grp] = {}
            for m in metric_cols:
                vals = g[m].dropna()
                out[key][grp][m] = {
                    "n": int(vals.size),
                    "mean": round(float(vals.mean()), 2) if vals.size else None,
                    "median": round(float(vals.median()), 2) if vals.size else None,
                    "std": round(float(vals.std(ddof=1)), 2) if vals.size > 1 else None,
                    "min": round(float(vals.min()), 2) if vals.size else None,
                    "max": round(float(vals.max()), 2) if vals.size else None,
                }
    Path(path).write_text(json.dumps(out, indent=2))
    print(f"  -> {Path(path).name}")


# ---------------------------------------------------------------------------
# Hauptablauf
# ---------------------------------------------------------------------------

def main():
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "qwen"]
    figdir = figures_dir(cfg)

    comparison = load_model_comparison(cfg)
    df = load_run_dataframe(models, cfg)

    if comparison is None and df.empty:
        print("Keine Eingabedaten gefunden. Bitte zuerst 02_compute_metrics.py "
              "ausfuehren.")
        return

    print(f"\nDiagramme werden gespeichert unter: {figdir}\n")

    # ---- Ebene 1: Modell x abhaengige Variablen -------------------------
    print("Ebene 1  Modellvergleich - abhaengige Variablen")
    if comparison:
        mvals = {m: {k: model_bar_values(comparison, k)[m]
                     for k, _ in DEPENDENT_VARS}
                 for m in comparison["per_model"]}
        order = [m for m in models if m in mvals] or list(mvals)
        grouped_bars(
            DEPENDENT_VARS, mvals, order,
            {m: MODEL_LABELS.get(m, m) for m in order},
            {m: MODEL_COLORS.get(m, "#777777") for m in order},
            "Ebene 1  Modellvergleich auf den abhaengigen Variablen",
            figdir / "ebene1_modell_abhaengige_variablen_balken.png")
    if not df.empty:
        grouped_boxplots(
            df, BOX_COVERAGE, "model", models,
            {m: MODEL_LABELS.get(m, m) for m in models},
            {m: MODEL_COLORS.get(m, "#777777") for m in models},
            "Ebene 1  Modellvergleich - Coverage-Verteilung je Lauf",
            figdir / "ebene1_modell_coverage_boxplot.png")

    # ---- Ebene 2: Modell x ergaenzende Metriken -------------------------
    print("Ebene 2  Modellvergleich - ergaenzende Metriken")
    if comparison:
        mvals2 = {m: {k: model_bar_values(comparison, k)[m]
                      for k, _ in ADDITIONAL_VARS}
                  for m in comparison["per_model"]}
        order = [m for m in models if m in mvals2] or list(mvals2)
        grouped_bars(
            ADDITIONAL_VARS, mvals2, order,
            {m: MODEL_LABELS.get(m, m) for m in order},
            {m: MODEL_COLORS.get(m, "#777777") for m in order},
            "Ebene 2  Modellvergleich auf ergaenzenden Metriken",
            figdir / "ebene2_modell_ergaenzende_metriken_balken.png")
    if not df.empty:
        grouped_boxplots(
            df, BOX_RATES, "model", models,
            {m: MODEL_LABELS.get(m, m) for m in models},
            {m: MODEL_COLORS.get(m, "#777777") for m in models},
            "Ebene 2  Modellvergleich - Verteilung der Raten je Lauf",
            figdir / "ebene2_modell_raten_boxplot.png")

    # ---- Ebene 3: Codebase x abhaengige Variablen -----------------------
    print("Ebene 3  Codebase-Vergleich - abhaengige Variablen")
    if not df.empty:
        cb_order = [c for c in CODEBASE_LABELS if c in df["codebase"].unique()]
        cb_order += [c for c in df["codebase"].unique() if c not in cb_order]
        cvals = {c: {k: codebase_bar_values(df, k).get(c)
                     for k, _ in DEPENDENT_VARS} for c in cb_order}
        grouped_bars(
            DEPENDENT_VARS, cvals, cb_order,
            {c: CODEBASE_LABELS.get(c, c) for c in cb_order},
            {c: CODEBASE_COLORS.get(c, "#777777") for c in cb_order},
            "Ebene 3  Codebase-Vergleich auf den abhaengigen Variablen",
            figdir / "ebene3_codebase_abhaengige_variablen_balken.png")
        grouped_boxplots(
            df, BOX_COVERAGE, "codebase", cb_order,
            {c: CODEBASE_LABELS.get(c, c) for c in cb_order},
            {c: CODEBASE_COLORS.get(c, "#777777") for c in cb_order},
            "Ebene 3  Codebase-Vergleich - Coverage-Verteilung je Lauf",
            figdir / "ebene3_codebase_coverage_boxplot.png")

        # ---- Ebene 4: Codebase x ergaenzende Metriken -------------------
        print("Ebene 4  Codebase-Vergleich - ergaenzende Metriken")
        cvals2 = {c: {k: codebase_bar_values(df, k).get(c)
                      for k, _ in ADDITIONAL_VARS} for c in cb_order}
        grouped_bars(
            ADDITIONAL_VARS, cvals2, cb_order,
            {c: CODEBASE_LABELS.get(c, c) for c in cb_order},
            {c: CODEBASE_COLORS.get(c, "#777777") for c in cb_order},
            "Ebene 4  Codebase-Vergleich auf ergaenzenden Metriken",
            figdir / "ebene4_codebase_ergaenzende_metriken_balken.png")
        grouped_boxplots(
            df, BOX_RATES, "codebase", cb_order,
            {c: CODEBASE_LABELS.get(c, c) for c in cb_order},
            {c: CODEBASE_COLORS.get(c, "#777777") for c in cb_order},
            "Ebene 4  Codebase-Vergleich - Verteilung der Raten je Lauf",
            figdir / "ebene4_codebase_raten_boxplot.png")

        # ---- deskriptive Kennzahlen ------------------------------------
        print("Export  deskriptive Kennzahlen")
        export_descriptive_stats(df, figdir / "descriptive_stats.json")

    print(f"\nFertig. Alle Ausgaben unter: {figdir}")


if __name__ == "__main__":
    main()
