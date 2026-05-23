"""Aggregiert Metriken aller Modelle und erzeugt Vergleichs-Diagramme."""

import json
from pathlib import Path
import yaml
import matplotlib.pyplot as plt
import numpy as np


def load_config():

    script_dir = Path(__file__).parent
    config_path = script_dir.parent.parent / "config.yaml"
    
    with open(config_path) as f:
        cfg =  yaml.safe_load(f)
    
    project_root = config_path.parent

    for key, value in cfg["paths"].items():
        p = Path(value)
        if not p.is_absolute():
            cfg["paths"][key] = str(project_root / p)
    return cfg


def load_all_metrics(cfg):
    """Lädt die Metriken aller Modelle aus output_metrics/.

    Überspringt Modell-Ordner ohne JSON-Datei sowie leere Funktions-Einträge.
    """
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
            print(f"  [WARN] {metrics_file}: kein gültiges JSON ({e})")
            continue

        # Leere Run-Listen rausfiltern, sonst kippt die Aggregation um
        data = {func: runs for func, runs in data.items() if runs}
        if not data:
            print(f"  [WARN] {model_dir.name}: keine Runs vorhanden")
            continue

        model_name = model_dir.name.removesuffix("_modell")
        metrics[model_name] = data

    return metrics


def aggregate_function(runs):
    """Aggregiert die Runs einer einzelnen Funktion via Micro-Averaging.

    Statt Prozente zu mitteln werden 'covered' und 'total' aufsummiert
    und erst danach dividiert. Das ist mathematisch korrekt, auch wenn
    sich 'total' zwischen Runs unterscheidet (z.B. bei verschiedenen
    Test-Includes).
    """
    compiled_runs = [r for r in runs if r.get("compiled")]

    return {
        "n_runs":         len(runs),
        "n_compiled":     len(compiled_runs),
        "compile_rate":   len(compiled_runs) / len(runs) * 100,
        "line_coverage":  _micro_average(runs, "line_coverage"),
        "branch_coverage": _micro_average(runs, "branch_coverage"),
    }


def _micro_average(runs, key):
    """Summiert covered/total über alle Runs und gibt den Prozentwert zurück.

    Erwartet, dass jeder Run unter `key` ein Dict mit 'covered' und 'total'
    hat (oder None). Fehlende oder None-Einträge werden übersprungen.
    Fallback auf 'percent * total / 100' falls 'covered' fehlt (für alte JSONs).
    """
    covered_sum = 0
    total_sum   = 0
    n_valid     = 0

    for run in runs:
        cov = run.get(key)
        if not cov:
            continue
        total = cov.get("total", 0)
        if total == 0:
            continue
        covered = cov.get("covered")
        if covered is None:
            covered = round(cov.get("percent", 0) / 100 * total)
        covered_sum += covered
        total_sum   += total
        n_valid     += 1

    if total_sum == 0:
        return {"percent": 0.0, "covered": 0, "total": 0, "n_valid_runs": 0}
    return {
        "percent":       covered_sum / total_sum * 100,
        "covered":       covered_sum,
        "total":         total_sum,
        "n_valid_runs":  n_valid,
    }


def aggregate_model(functions):
    """Aggregiert alle Funktionen eines Modells.

    Pro-Funktion: wie aggregate_function.
    Overall: erneut Micro-Averaging über *alle* Runs aller Funktionen,
    nicht der Durchschnitt der Funktions-Mittelwerte. So zählen längere
    Funktionen / Funktionen mit mehr Runs entsprechend ihrer Größe mit.
    """
    per_function = {f: aggregate_function(runs) for f, runs in functions.items()}
    all_runs     = [run for runs in functions.values() for run in runs]

    return {
        "overall":      aggregate_function(all_runs),
        "per_function": per_function,
    }


def aggregate(metrics):
    return {model: aggregate_model(funcs) for model, funcs in metrics.items()}


# ─── Plotting ──────────────────────────────────────────────────────────────

def grouped_bar_chart(categories, series, ylabel, title, output_path):
    """Erzeugt ein gruppiertes Balkendiagramm.

    categories: Labels auf der X-Achse (z.B. Metrik-Namen oder Funktions-Namen)
    series:     Dict {serien_name: [werte_pro_kategorie]}
    """
    n_series = len(series)
    if n_series == 0 or not categories:
        print(f"  [WARN] Keine Daten für '{title}'")
        return

    x      = np.arange(len(categories))
    width  = 0.8 / n_series                  # zusammen 80% der Slot-Breite
    offset = (n_series - 1) * width / 2      # Gruppe zentrieren

    fig_width = max(10, len(categories) * 1.2)
    fig, ax = plt.subplots(figsize=(fig_width, 6))

    for i, (name, values) in enumerate(series.items()):
        ax.bar(x + i * width - offset, values, width, label=name.capitalize())

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


def plot_model_comparison(summary, output_path):
    """Overall Compile Rate, Line Coverage, Branch Coverage pro Modell."""
    categories = ["Compile Rate", "Line Coverage", "Branch Coverage"]
    series = {
        model: [
            data["overall"]["compile_rate"],
            data["overall"]["line_coverage"]["percent"],
            data["overall"]["branch_coverage"]["percent"],
        ]
        for model, data in summary.items()
    }
    grouped_bar_chart(
        categories, series,
        ylabel="Prozent (%)",
        title="Modellvergleich: Compile Rate & Coverage (Micro-Average)",
        output_path=output_path,
    )


def plot_per_function(summary, output_path):
    """Line Coverage pro Funktion pro Modell."""
    functions = sorted({
        f for data in summary.values() for f in data["per_function"]
    })
    series = {
        model: [
            data["per_function"].get(f, {}).get("line_coverage", {}).get("percent", 0)
            for f in functions
        ]
        for model, data in summary.items()
    }
    grouped_bar_chart(
        functions, series,
        ylabel="Line Coverage (%)",
        title="Line Coverage pro Funktion (Micro-Average über Runs)",
        output_path=output_path,
    )


# ─── Main ──────────────────────────────────────────────────────────────────

def main():
    cfg     = load_config()
    metrics = load_all_metrics(cfg)
    if not metrics:
        print("Keine Metriken gefunden. Zuerst compile_and_run.py ausführen.")
        return

    summary = aggregate(metrics)

    output_dir = Path(cfg["paths"]["output_metrics"])
    summary_path = output_dir / "summary.json"
    summary_path.write_text(json.dumps(summary, indent=2))
    print(f"  -> Zusammenfassung: {summary_path}")

    plot_model_comparison(summary, output_dir / "model_comparison.png")
    plot_per_function(summary, output_dir / "per_function_coverage.png")


if __name__ == "__main__":
    main()
