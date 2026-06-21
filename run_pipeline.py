
import subprocess
import sys


STEPS = {
    "generate": "scripts/generation/testgen.py",
    "compile": "scripts/execution/data_collection_and_compilation.py",
    "measure": "scripts/execution/01_measure_coverage.py",
    "compute": 'scripts/execution/02_compute_metrics.py',
    "analyze": "scripts/analyze/03_visualize.py",
}


def run_step(name, script, extra_args):
    print(f"\n{'='*60}")
    print(f" STEP: {name}")
    print(f"{'='*60}")
    cmd = [sys.executable, script] + extra_args
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print(f"\n[ABBRUCH] {name} fehlgeschlagen (exit {result.returncode})")
        sys.exit(result.returncode)


def main():
    # Optional: nur bestimmte Modelle als Argument (z.B. python run_pipeline.py claude)
    model_args = sys.argv[1:]

    for step_name, script in STEPS.items():
        args = model_args if step_name != "analyze" else []
        run_step(step_name, script, args)

    print(f"\n{'='*60}")
    print(" Pipeline abgeschlossen.")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()