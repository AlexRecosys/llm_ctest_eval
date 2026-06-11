"""Aggregiert Metriken aller Modelle und erzeugt Vergleichs-Diagramme.

Aggregationslogik (wichtig!):
  Coverage wird ERST pro Run berechnet, DANN über die Runs gemittelt.

    LC_run   = (Summe ausgefuehrter Zeilen aller FUTs in diesem Run)
               / (Summe ausfuehrbarer Zeilen aller FUTs)
    LineCov  = mean(LC_run1, LC_run2, ...)   (+ Standardabweichung)

  Analog fuer Branch Coverage und fuer die Compile-Error-Rate.

Sonderfaelle:
  - Compile-Fehler einer FUT in einem Run zaehlt als 0 abgedeckte
    Zeilen/Branches (also 0 %) gegen die volle Zeilen-/Branch-Zahl der
    Funktion -> wirkt sich korrekt negativ aus.
  - Funktionen ohne Branches werden bei der Branch Coverage komplett
    ignoriert (kein 0/0), damit sie das Ergebnis nicht faelschlich druecken.
"""

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


# ─── Hilfsfunktionen ─────────────────────────────────────────────────────

def build_reference_totals(metrics):
    """Ermittelt pro Funktion die 'wahre' Anzahl ausfuehrbarer Zeilen/Branches.

    Das ist eine Eigenschaft der Quellfunktion (modellunabhaengig), variiert
    also ueber Runs/Modelle nicht. Wir nehmen das Maximum der je beobachteten
    Totals als Referenz-Nenner. So koennen auch Runs, in denen die Funktion
    NICHT kompiliert (kein eigenes Total), gegen diesen Nenner als 0 % zaehlen.

    Gibt (line_ref, branch_ref) als Dicts {func_name: total} zurueck.
    branch_ref enthaelt nur Funktionen, die tatsaechlich Branches haben (>0).
    """
    line_ref, branch_ref = {}, {}
    for funcs in metrics.values():
        for func, runs in funcs.items():
            for r in runs:
                lc = r.get("line_coverage")
                if lc and lc.get("total"):
                    line_ref[func] = max(line_ref.get(func, 0), lc["total"])
                bc = r.get("branch_coverage")
                if bc and bc.get("total"):
                    branch_ref[func] = max(branch_ref.get(func, 0), bc["total"])
    return line_ref, branch_ref


def _covered(run, key):
    """Abgedeckte Zeilen/Branches eines Runs. Compile-Fehler / fehlende Daten -> 0."""
    if not run.get("compiled"):
        return 0
    cov = run.get(key)
    if not cov:
        return 0
    if cov.get("covered") is not None:
        return cov["covered"]
    # Fallback fuer alte JSONs ohne 'covered'
    if cov.get("percent") is not None and cov.get("total"):
        return round(cov["percent"] / 100 * cov["total"])
    return 0


def _mean_std(values):
    """Mittelwert + (Stichproben-)Standardabweichung einer Werteliste."""
    if not values:
        return {"mean": None, "std": None, "n": 0}
    arr = np.asarray(values, dtype=float)
    return {
        "mean": float(arr.mean()),
        "std": float(arr.std(ddof=1)) if len(arr) > 1 else 0.0,
        "n": len(arr),
    }


def _run_names(funcs):
    return sorted({r["run"] for runs in funcs.values() for r in runs})


def _per_run_pcts_single(runs, key, ref_total):
    """Pro-Run-Prozente EINER Funktion (Compile-Fehler -> 0 %)."""
    if not ref_total:
        return []
    return [_covered(r, key) / ref_total * 100 for r in runs]


# ─── Aggregation ───────────────────────────────────────────────────────────

def aggregate_function(runs, func, line_ref, branch_ref):
    """Pro-Funktion: pro Run rechnen, dann mitteln (+ Std)."""
    branch_total = branch_ref.get(func, 0)
    line_pcts = _per_run_pcts_single(runs, "line_coverage", line_ref.get(func, 0))
    branch_pcts = _per_run_pcts_single(runs, "branch_coverage", branch_total)

    n_compiled = sum(1 for r in runs if r.get("compiled"))
    return {
        "n_runs": len(runs),
        "compile_rate": n_compiled / len(runs) * 100 if runs else 0.0,
        "line_coverage": _mean_std(line_pcts),
        # None = Funktion hat ueberhaupt keine Branches -> bei Branch ignorieren
        "branch_coverage": _mean_std(branch_pcts) if branch_total > 0 else None,
        "has_branches": branch_total > 0,
    }


def aggregate_model(funcs, line_ref, branch_ref):
    """Overall pro Modell: Coverage je Run ueber ALLE FUTs, dann mitteln (+ Std).

    Compile-Error-Rate je Run = Tests mit Compile-Fehler / generierte Tests.
    """
    line_run_pcts, branch_run_pcts, cer_run_pcts = [], [], []

    for rn in _run_names(funcs):
        l_cov = l_tot = b_cov = b_tot = 0
        n_tests = n_fail = 0

        for func, runs in funcs.items():
            run = next((r for r in runs if r["run"] == rn), None)
            if run is None:
                continue

            n_tests += 1
            if not run.get("compiled"):
                n_fail += 1

            lt = line_ref.get(func, 0)
            if lt:                                  # Funktion mit bekannten Zeilen
                l_tot += lt
                l_cov += _covered(run, "line_coverage")

            bt = branch_ref.get(func, 0)
            if bt:                                  # nur Funktionen MIT Branches
                b_tot += bt
                b_cov += _covered(run, "branch_coverage")

        if l_tot:
            line_run_pcts.append(l_cov / l_tot * 100)
        if b_tot:
            branch_run_pcts.append(b_cov / b_tot * 100)
        if n_tests:
            cer_run_pcts.append(n_fail / n_tests * 100)

    return {
        "n_runs": len(_run_names(funcs)),
        "line_coverage": _mean_std(line_run_pcts),
        "branch_coverage": _mean_std(branch_run_pcts),
        "compile_error_rate": _mean_std(cer_run_pcts),
    }


def aggregate(metrics):
    line_ref, branch_ref = build_reference_totals(metrics)
    summary = {}
    for model, funcs in metrics.items():
        summary[model] = {
            "overall": aggregate_model(funcs, line_ref, branch_ref),
            "per_function": {
                f: aggregate_function(runs, f, line_ref, branch_ref)
                for f, runs in funcs.items()
            },
        }
    return summary


# ─── Plotting ────────────────────────────────────────────────────────────

def grouped_bar_chart(categories, series, ylabel, title, output_path, errors=None):
    """Gruppiertes Balkendiagramm mit optionalen Fehlerbalken (Std)."""
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


def _function_series(summary, metric_key):
    """Baut (functions, series, errors) fuer eine Pro-Funktion-Metrik.

    Nimmt nur Funktionen auf, fuer die die Metrik in mind. einem Modell
    definiert ist (bei Branch: nur Funktionen mit Branches).
    """
    functions = sorted({
        f for d in summary.values()
        for f, fm in d["per_function"].items()
        if fm.get(metric_key) is not None
    })

    series, errors = {}, {}
    for model, d in summary.items():
        vals, errs = [], []
        for f in functions:
            fm = d["per_function"].get(f)
            m = fm.get(metric_key) if fm else None
            if m and m.get("mean") is not None:
                vals.append(m["mean"])
                errs.append(m.get("std") or 0.0)
            else:
                vals.append(0.0)
                errs.append(0.0)
        series[model] = vals
        errors[model] = errs
    return functions, series, errors


def plot_per_function_line(summary, output_path):
    functions, series, errors = _function_series(summary, "line_coverage")
    grouped_bar_chart(
        functions, series, ylabel="Line Coverage (%)",
        title="Line Coverage pro Funktion (Mittel ueber Runs, +/- Std)",
        output_path=output_path, errors=errors,
    )


def plot_per_function_branch(summary, output_path):
    functions, series, errors = _function_series(summary, "branch_coverage")
    grouped_bar_chart(
        functions, series, ylabel="Branch Coverage (%)",
        title="Branch Coverage pro Funktion (Mittel ueber Runs, +/- Std)",
        output_path=output_path, errors=errors,
    )


def plot_model_overall(summary, output_path):
    """Gesamt-Line- und Branch-Coverage pro Modell (Mittel ueber Runs, +/- Std)."""
    categories = ["Line Coverage", "Branch Coverage"]
    series, errors = {}, {}
    for model, d in summary.items():
        lc = d["overall"]["line_coverage"]
        bc = d["overall"]["branch_coverage"]
        series[model] = [lc["mean"] or 0.0, bc["mean"] or 0.0]
        errors[model] = [lc["std"] or 0.0, bc["std"] or 0.0]
    grouped_bar_chart(
        categories, series, ylabel="Coverage (%)",
        title="Gesamt-Coverage pro Modell (Mittel ueber Runs, +/- Std)",
        output_path=output_path, errors=errors,
    )


def plot_compile_error_rate(summary, output_path):
    """Compile-Error-Rate pro Modell (Mittel ueber Runs, +/- Std)."""
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


# ─── Main ────────────────────────────────────────────────────────────────

def main():
    cfg = load_config()
    metrics = load_all_metrics(cfg)
    if not metrics:
        print("Keine Metriken gefunden. Zuerst compile_and_run.py ausfuehren.")
        return

    summary = aggregate(metrics)

    output_dir = Path(cfg["paths"]["output_metrics"])
    summary_path = output_dir / "summary.json"
    summary_path.write_text(json.dumps(summary, indent=2))
    print(f"  -> Zusammenfassung: {summary_path}")

    plot_per_function_line(summary, output_dir / "per_function_line_coverage.png")
    plot_per_function_branch(summary, output_dir / "per_function_branch_coverage.png")
    plot_model_overall(summary, output_dir / "model_overall_coverage.png")
    plot_compile_error_rate(summary, output_dir / "compile_error_rate.png")


if __name__ == "__main__":
    main()
