"""Aggregiert Metriken aller Modelle und erzeugt Vergleichs-Diagramme."""

import json
from pathlib import Path
import yaml

try:
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("Bitte installieren: pip install matplotlib numpy")
    exit(1)


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
    """Lädt die Metriken aller Modelle aus output_metrics/."""
    metrics = {}
    metrics_base = Path(cfg["paths"]["output_metrics"])
    for model_dir in sorted(metrics_base.iterdir()):
        if not model_dir.is_dir():
            continue
        model_name = model_dir.name.replace("_modell", "")
        metrics_file = model_dir / "model_metric_function.json"
        if metrics_file.exists():
            metrics[model_name] = json.loads(metrics_file.read_text())
    return metrics


def aggregate(metrics):
    """Berechnet Durchschnittswerte pro Modell und pro Funktion."""
    summary = {}
    for model, functions in metrics.items():
        model_stats = {"compile_rate": [], "line_coverage": [], "branch_coverage": []}
        per_function = {}

        for func_name, runs in functions.items():
            compiled = [r for r in runs if r["compiled"]]
            model_stats["compile_rate"].append(len(compiled) / len(runs) * 100)

            line_covs = [r["line_coverage"]["percent"] for r in runs if r.get("line_coverage")]
            branch_covs = [r["branch_coverage"]["percent"] for r in runs if r.get("branch_coverage")]

            avg_line = sum(line_covs) / len(line_covs) if line_covs else 0
            avg_branch = sum(branch_covs) / len(branch_covs) if branch_covs else 0
            model_stats["line_coverage"].append(avg_line)
            model_stats["branch_coverage"].append(avg_branch)

            per_function[func_name] = {
                "compile_rate": len(compiled) / len(runs) * 100,
                "avg_line_coverage": avg_line,
                "avg_branch_coverage": avg_branch,
            }

        summary[model] = {
            "overall": {
                "compile_rate": sum(model_stats["compile_rate"]) / len(model_stats["compile_rate"]) if model_stats["compile_rate"] else 0,
                "avg_line_coverage": sum(model_stats["line_coverage"]) / len(model_stats["line_coverage"]) if model_stats["line_coverage"] else 0,
                "avg_branch_coverage": sum(model_stats["branch_coverage"]) / len(model_stats["branch_coverage"]) if model_stats["branch_coverage"] else 0,
            },
            "per_function": per_function,
        }
    return summary


def plot_model_comparison(summary, output_path):
    """Erzeugt ein Balkendiagramm: Compile Rate, Line Cov, Branch Cov pro Modell."""
    models = list(summary.keys())
    metrics_names = ["compile_rate", "avg_line_coverage", "avg_branch_coverage"]
    labels = ["Compile Rate", "Line Coverage", "Branch Coverage"]

    x = np.arange(len(labels))
    width = 0.25

    fig, ax = plt.subplots(figsize=(10, 6))
    for i, model in enumerate(models):
        values = [summary[model]["overall"][m] for m in metrics_names]
        ax.bar(x + i * width, values, width, label=model.capitalize())

    ax.set_ylabel("Prozent (%)")
    ax.set_title("Modellvergleich: Compile Rate & Coverage")
    ax.set_xticks(x + width)
    ax.set_xticklabels(labels)
    ax.legend()
    ax.set_ylim(0, 105)
    ax.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    print(f"  -> Diagramm: {output_path}")
    plt.close()


def plot_per_function(summary, output_path):
    """Erzeugt ein Diagramm: Line Coverage pro Funktion pro Modell."""
    models = list(summary.keys())
    all_funcs = set()
    for model in models:
        all_funcs.update(summary[model]["per_function"].keys())
    functions = sorted(all_funcs)

    if not functions:
        print("  Keine Funktionen zum Plotten.")
        return

    x = np.arange(len(functions))
    width = 0.25

    fig, ax = plt.subplots(figsize=(max(10, len(functions) * 2), 6))
    for i, model in enumerate(models):
        values = [
            summary[model]["per_function"].get(f, {}).get("avg_line_coverage", 0)
            for f in functions
        ]
        ax.bar(x + i * width, values, width, label=model.capitalize())

    ax.set_ylabel("Line Coverage (%)")
    ax.set_title("Line Coverage pro Funktion")
    ax.set_xticks(x + width)
    ax.set_xticklabels(functions, rotation=45, ha="right")
    ax.legend()
    ax.set_ylim(0, 105)
    ax.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    print(f"  -> Diagramm: {output_path}")
    plt.close()


def main():
    cfg = load_config()
    metrics = load_all_metrics(cfg)
    if not metrics:
        print("Keine Metriken gefunden. Zuerst compile_and_run.py ausführen.")
        return

    summary = aggregate(metrics)

    # Zusammenfassung als JSON
    summary_path = Path(cfg["paths"]["output_metrics"]) / "summary.json"
    summary_path.write_text(json.dumps(summary, indent=2))
    print(f"  -> Zusammenfassung: {summary_path}")

    output_dir = Path(cfg["paths"]["output_metrics"])
    plot_model_comparison(summary, output_dir / "model_comparison.png")
    plot_per_function(summary, output_dir / "per_function_coverage.png")


if __name__ == "__main__":
    main()