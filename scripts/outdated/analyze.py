"""Aggregiert Metriken aller Modelle, erzeugt Vergleichs-Diagramme und Tabellen.

Evaluation On-Premise- vs. Cloud-LLM zur C-Unit-Test-Generierung.

Aggregationslogik (WICHTIG):
  Coverage wird pro Funktion als Prozentwert je Run bestimmt und dann
  gleichgewichtet gemittelt (Macro-Average ueber Funktionen). Begruendung:
  Jede zu testende Funktion soll gleich zaehlen, auch solche, die NIE
  erfolgreich vermessen wurden -> die zaehlen als 0 %.

    pct(f, run)   = Line-/Branch-% der Funktion f in diesem Run,
                    bei Compile-Fehler / fehlender Messung / fehlendem
                    Test = 0 %.
    Pro Run:  mean ueber alle Funktionen der pct-Werte
    Gesamt:   mean(Run_1, Run_2, Run_3)  (+ Standardabweichung)

Regeln fuer Fehlversuche und Sonderfaelle:
  - Eine Funktion, die in KEINEM Run irgendeines Modells je erfolgreich
    vermessen wurde, zaehlt als 0 % (kein Ausschluss).
  - Hat ein Modell bei einer Funktion 0 % (Compile-Fehler, Laufzeitfehler,
    nie vermessen, kein Test erzeugt), zieht dieser Wert Grafik UND
    Gesamtschnitt dieses Modells nach unten.
  - AUSNAHME nur fuer strukturell branchlose Funktionen: Lief eine Funktion
    irgendwo erfolgreich, zeigte aber nie Branches, hat sie keine Branches.
    Solche Funktionen werden bei der BRANCH-Coverage ignoriert (n/a),
    damit sie das Branch-Ergebnis nicht faelschlich druecken. Bei Line
    zaehlen sie normal mit.
"""

import csv
import json
from pathlib import Path
import yaml
import matplotlib.pyplot as plt
import numpy as np


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


def load_all_metrics(cfg):
    """Laedt die Metriken aller Modelle aus output_metrics/."""
    metrics = {}
    metrics_base = Path(cfg["paths"]["output_metrics"])

    for model_dir in sorted(metrics_base.iterdir()):
        if not model_dir.is_dir():
            continue
        metrics_file = model_dir / "model_metric_function.json"
        if not metrics_file.exists():
            continue
        try:
            data = json.loads(metrics_file.read_text())
        except json.JSONDecodeError as e:
            print(f"  [WARN] {metrics_file}: kein gueltiges JSON ({e})")
            continue

        data = {func: runs for func, runs in data.items() if runs}
        if not data:
            print(f"  [WARN] {model_dir.name}: keine Runs vorhanden")
            continue

        model_name = model_dir.name.removesuffix("_modell")
        metrics[model_name] = data

    return metrics


# ─── Universum & Klassifikation ─────────────────────────────────────────────

def collect_universe(metrics):
    """Alle zu testenden Funktionen (Union ueber Modelle) und Run-Namen."""
    funcs = sorted({f for m in metrics.values() for f in m})
    run_names = sorted({
        r["run"] for m in metrics.values() for runs in m.values() for r in runs
    })
    return funcs, run_names


def classify_functions(metrics):
    """Bestimmt strukturell branchlose Funktionen.

    ever_ran      : Funktion lief irgendwo erfolgreich (kompiliert + Line-Daten)
    has_branches  : Funktion zeigte irgendwo Branches (>0)
    branchless    : ever_ran UND nicht has_branches  -> strukturell ohne Branches
    """
    ever_ran, has_branches = set(), set()
    for m in metrics.values():
        for f, runs in m.items():
            for r in runs:
                if r.get("compiled") and r.get("line_coverage"):
                    ever_ran.add(f)
                bc = r.get("branch_coverage")
                if bc and bc.get("total"):
                    has_branches.add(f)
    return ever_ran - has_branches


# ─── Pro-Funktion-pro-Run-Werte ─────────────────────────────────────────────

def _get_run(model_funcs, func, run_name):
    return next((r for r in model_funcs.get(func, []) if r["run"] == run_name), None)


def line_pct(run):
    """Line-Coverage % eines Runs. Fehlversuch / kein Test -> 0 %."""
    if run is None or not run.get("compiled"):
        return 0.0
    lc = run.get("line_coverage")
    if not lc:
        return 0.0
    if lc.get("percent") is not None:
        return float(lc["percent"])
    t = lc.get("total")
    return float(lc["covered"]) / t * 100 if t else 0.0


def branch_pct(run):
    """Branch-Coverage % eines Runs. Fehlversuch / kein Test -> 0 %."""
    if run is None or not run.get("compiled"):
        return 0.0
    bc = run.get("branch_coverage")
    if not bc or not bc.get("total"):
        return 0.0
    if bc.get("percent") is not None:
        return float(bc["percent"])
    return float(bc["covered"]) / bc["total"] * 100


def compile_error(run):
    """100 % wenn Compile-Fehler oder gar kein Test erzeugt, sonst 0 %."""
    if run is None:
        return 100.0
    return 0.0 if run.get("compiled") else 100.0


# ─── Statistik-Helfer ────────────────────────────────────────────────────────

def _row_stats(values):
    arr = np.asarray(values, dtype=float)
    return {
        "runs": [float(v) for v in values],
        "mean": float(arr.mean()) if len(arr) else 0.0,
        "std": float(arr.std(ddof=1)) if len(arr) > 1 else 0.0,
    }


def _overall(per_function, funcs, key):
    """Pro Run ueber Funktionen mitteln, dann ueber Runs mitteln (+ Std).

    funcs: Funktionen, die in diese Metrik einfliessen (Branch: ohne branchlose).
    """
    funcs = [f for f in funcs if per_function[f][key] is not None]
    if not funcs:
        return {"per_run": [], "mean": None, "std": None}

    n_runs = len(per_function[funcs[0]][key]["runs"])
    per_run = [
        float(np.mean([per_function[f][key]["runs"][i] for f in funcs]))
        for i in range(n_runs)
    ]
    arr = np.asarray(per_run, dtype=float)
    return {
        "per_run": per_run,
        "mean": float(arr.mean()),
        "std": float(arr.std(ddof=1)) if len(arr) > 1 else 0.0,
    }


# ─── Aggregation ─────────────────────────────────────────────────────────────

def aggregate_model(model_funcs, all_funcs, run_names, branchless):
    per_function = {}
    for f in all_funcs:
        line_vals = [line_pct(_get_run(model_funcs, f, rn)) for rn in run_names]
        cer_vals = [compile_error(_get_run(model_funcs, f, rn)) for rn in run_names]
        entry = {
            "line": _row_stats(line_vals),
            "compile_error": _row_stats(cer_vals),
        }
        if f in branchless:
            entry["branch"] = None              # branchlos -> bei Branch ignorieren
        else:
            branch_vals = [branch_pct(_get_run(model_funcs, f, rn)) for rn in run_names]
            entry["branch"] = _row_stats(branch_vals)
        per_function[f] = entry

    branch_funcs = [f for f in all_funcs if f not in branchless]
    overall = {
        "line": _overall(per_function, all_funcs, "line"),
        "branch": _overall(per_function, branch_funcs, "branch"),
        "compile_error_rate": _overall(per_function, all_funcs, "compile_error"),
    }
    return {"overall": overall, "per_function": per_function}


def aggregate(metrics):
    all_funcs, run_names = collect_universe(metrics)
    branchless = classify_functions(metrics)
    summary = {
        model: aggregate_model(funcs, all_funcs, run_names, branchless)
        for model, funcs in metrics.items()
    }
    return summary, all_funcs, run_names, branchless


# ─── Tabellen ────────────────────────────────────────────────────────────────

def write_table(path, title, summary_model, all_funcs, run_names, key):
    """Schreibt eine CSV: Titel, Header, dann je Funktion eine Zeile.

    Spalten: Funktion | Run_1 | Run_2 | Run_3 | Mittel | Std
    """
    run_labels = [f"Run_{i + 1}" for i in range(len(run_names))]
    with open(path, "w", newline="", encoding="utf-8") as fh:
        w = csv.writer(fh)
        w.writerow([title])
        w.writerow(["Funktion", *run_labels, "Mittel", "Std"])
        for f in all_funcs:
            entry = summary_model["per_function"][f][key]
            if entry is None:                      # branchlos bei Branch-Tabelle
                w.writerow([f, *(["n/a"] * len(run_labels)), "n/a", "n/a"])
            else:
                cells = [f"{v:.2f}" for v in entry["runs"]]
                w.writerow([f, *cells, f"{entry['mean']:.2f}", f"{entry['std']:.2f}"])
    print(f"  -> Tabelle: {path}")


def write_all_tables(summary, all_funcs, run_names, output_dir):
    tables = [
        ("line", "Mittel über Runs Line Coverage", "line_coverage"),
        ("branch", "Mittel über Runs Branch Coverage", "branch_coverage"),
        ("compile_error", "Mittel über Runs Compilation Error", "compilation_error"),
    ]
    for model, data in summary.items():
        for key, title, filestem in tables:
            path = output_dir / f"tabelle_{filestem}_{model}.csv"
            write_table(path, title, data, all_funcs, run_names, key)


# ─── Plotting ────────────────────────────────────────────────────────────────

def grouped_bar_chart(categories, series, ylabel, title, output_path, errors=None):
    n_series = len(series)
    if n_series == 0 or not categories:
        print(f"  [WARN] Keine Daten fuer '{title}'")
        return

    x = np.arange(len(categories))
    width = 0.8 / n_series
    offset = (n_series - 1) * width / 2

    fig_width = max(10, len(categories) * 1.2)
    fig, ax = plt.subplots(figsize=(fig_width, 6))

    for i, (name, values) in enumerate(series.items()):
        err = errors.get(name) if errors else None
        ax.bar(
            x + i * width - offset, values, width, label=name.capitalize(),
            yerr=err, capsize=4 if err else 0,
        )

    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.set_xticks(x)
    ax.set_xticklabels(categories, rotation=45 if len(categories) > 4 else 0, ha="right")
    ax.legend()
    ax.set_ylim(0, 105)
    ax.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"  -> Diagramm: {output_path}")


def _function_series(summary, all_funcs, key):
    """Pro-Funktion-Serien fuers Diagramm.

    Bei Branch fliegen branchlose Funktionen (entry None in ALLEN Modellen)
    als X-Kategorie raus.
    """
    functions = [
        f for f in all_funcs
        if any(d["per_function"][f][key] is not None for d in summary.values())
    ]
    series, errors = {}, {}
    for model, d in summary.items():
        vals, errs = [], []
        for f in functions:
            entry = d["per_function"][f][key]
            if entry is None:
                vals.append(0.0)
                errs.append(0.0)
            else:
                vals.append(entry["mean"])
                errs.append(entry["std"])
        series[model] = vals
        errors[model] = errs
    return functions, series, errors


def plot_per_function_line(summary, all_funcs, output_path):
    functions, series, errors = _function_series(summary, all_funcs, "line")
    grouped_bar_chart(
        functions, series, ylabel="Line Coverage (%)",
        title="Line Coverage pro Funktion (Mittel ueber Runs, +/- Std)",
        output_path=output_path, errors=errors,
    )


def plot_per_function_branch(summary, all_funcs, output_path):
    functions, series, errors = _function_series(summary, all_funcs, "branch")
    grouped_bar_chart(
        functions, series, ylabel="Branch Coverage (%)",
        title="Branch Coverage pro Funktion (Mittel ueber Runs, +/- Std)",
        output_path=output_path, errors=errors,
    )


def plot_model_overall(summary, output_path):
    categories = ["Line Coverage", "Branch Coverage"]
    series, errors = {}, {}
    for model, d in summary.items():
        lc, bc = d["overall"]["line"], d["overall"]["branch"]
        series[model] = [lc["mean"] or 0.0, bc["mean"] or 0.0]
        errors[model] = [lc["std"] or 0.0, bc["std"] or 0.0]
    grouped_bar_chart(
        categories, series, ylabel="Coverage (%)",
        title="Gesamt-Coverage pro Modell (Mittel ueber Runs, +/- Std)",
        output_path=output_path, errors=errors,
    )


def plot_compile_error_rate(summary, output_path):
    categories = ["Compile Error Rate"]
    series, errors = {}, {}
    for model, d in summary.items():
        cer = d["overall"]["compile_error_rate"]
        series[model] = [cer["mean"] or 0.0]
        errors[model] = [cer["std"] or 0.0]
    grouped_bar_chart(
        categories, series, ylabel="Compile Error Rate (%)",
        title="Compile Error Rate pro Modell (Mittel ueber Runs, +/- Std)",
        output_path=output_path, errors=errors,
    )


# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    cfg = load_config()
    metrics = load_all_metrics(cfg)
    if not metrics:
        print("Keine Metriken gefunden. Zuerst compile_and_run.py ausfuehren.")
        return

    summary, all_funcs, run_names, branchless = aggregate(metrics)

    output_dir = Path(cfg["paths"]["output_metrics"])
    (output_dir / "summary.json").write_text(json.dumps(summary, indent=2))
    print(f"  -> Zusammenfassung: {output_dir / 'summary.json'}")
    if branchless:
        print(f"  [INFO] branchlose Funktionen (bei Branch ignoriert): {sorted(branchless)}")

    write_all_tables(summary, all_funcs, run_names, output_dir)

    plot_per_function_line(summary, all_funcs, output_dir / "per_function_line_coverage.png")
    plot_per_function_branch(summary, all_funcs, output_dir / "per_function_branch_coverage.png")
    plot_model_overall(summary, output_dir / "model_overall_coverage.png")
    plot_compile_error_rate(summary, output_dir / "compile_error_rate.png")


if __name__ == "__main__":
    main()
