"""Kompiliert generierte Tests und misst Coverage via gcov."""

import subprocess
import json
import re
import shutil
from pathlib import Path
import yaml
import traceback
import subprocess


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


def find_source_files(src_dir, codebase_name):
    """Findet alle .c-Dateien einer Codebase für die Kompilierung."""
    codebase_path = Path(src_dir) / codebase_name
    return list(codebase_path.glob("*.c"))


def codebase_for_function(func_name, functions_dir):
    """Ermittelt die Codebase anhand des Ordners, in dem die Funktion liegt."""
    for codebase_dir in Path(functions_dir).iterdir():
        if not codebase_dir.is_dir():
            continue
        for f in codebase_dir.glob("*.txt"):
            if f.stem == func_name:
                return codebase_dir.name
    return None


def compile_test(test_file, src_files, unity_c, build_dir, gcc_flags, cfg):
    build_dir.mkdir(parents=True, exist_ok=True)
    exe = (build_dir / "test_exe").resolve()
    unity_include = Path(cfg["paths"]["unity_dir"]).resolve()
    src_root = Path(cfg["paths"]["src_dir"]).resolve()
    unique_codebase_dirs = {f.parent.resolve() for f in src_files}

    compiler        = ["gcc"]
    flags           = gcc_flags
    includes        = [f"-I{unity_include}", f"-I{src_root}"] + [f"-I{d}" for d in unique_codebase_dirs]
    sources         = [str(test_file.resolve()), str(unity_c.resolve())] + [str(f.resolve()) for f in src_files]
    output          = ["-o", str(exe), "-lm"]

    cmd = compiler + flags + includes + sources + output

    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    log = result.stdout + result.stderr
    return result.returncode == 0, log, exe


def run_test(exe, build_dir):
    """Führt das kompilierte Test-Binary aus."""
    result = subprocess.run(
        [str(exe)], capture_output=True, text=True, timeout=30, cwd=str(build_dir)
    )
    return result.returncode, result.stdout + result.stderr


def parse_gcov_output(gcov_text):
    """Extrahiert Line- und Branch-Coverage aus gcov -b Output."""
    line_cov = None
    branch_cov = None
    for line in gcov_text.splitlines():
        if "Lines executed:" in line:
            m = re.search(r"Lines executed:(\d+\.\d+)% of (\d+)", line)
            if m:
                line_cov = {"percent": float(m.group(1)), "total": int(m.group(2))}
        if "Branches executed:" in line:
            m = re.search(r"Branches executed:(\d+\.\d+)% of (\d+)", line)
            if m:
                branch_cov = {"percent": float(m.group(1)), "total": int(m.group(2))}
    return line_cov, branch_cov


def measure_coverage(build_dir):
    """Führt gcov aus und gibt Coverage-Daten zurück."""
    gcda_files = list(build_dir.glob("*.gcda"))
    if not gcda_files:
        return None, None

    results_line = None
    results_branch = None
    for gcda in gcda_files:
        result = subprocess.run(
            ["gcov", "-b", "-f", str(gcda.name)],
            capture_output=True, text=True, cwd=str(build_dir)
        )
        lc, bc = parse_gcov_output(result.stdout)
        # Nimm Coverage der Source-Datei (nicht unity.c / test)
        if lc and (results_line is None or lc["total"] > results_line["total"]):
            results_line = lc
        if bc and (results_branch is None or bc["total"] > results_branch["total"]):
            results_branch = bc

    return results_line, results_branch


def clean_build(build_dir):
    if build_dir.exists():
        shutil.rmtree(build_dir)


def process_model(model_name, cfg):
    """Verarbeitet alle Tests eines Modells."""
    tests_base = Path(cfg["paths"]["output_tests"]) / f"{model_name}_modell"
    metrics_dir = Path(cfg["paths"]["output_metrics"]) / f"{model_name}_modell"
    metrics_dir.mkdir(parents=True, exist_ok=True)
    build_dir = Path(cfg["paths"]["build_dir"])
    unity_c = Path(cfg["paths"]["unity_dir"]) / "unity.c"
    gcc_flags = cfg["experiment"]["gcc_flags"]

    print(f'unity_c: {unity_c}')
    print(f'tests_base: {tests_base}')
    print(f'tests_base exists: {tests_base.exists()}')
    print(f'{tests_base.absolute()}')

    all_metrics = {}

    if not tests_base.exists():
        print(f"  Keine Tests gefunden für {model_name}")
        return

    for func_dir in sorted(tests_base.iterdir()):
        if not func_dir.is_dir():
            continue
        func_name = func_dir.name
        codebase = codebase_for_function(func_name, cfg["paths"]["functions_dir"])
        if not codebase:
            print(f"  [WARN] Keine Codebase für {func_name}")
            continue

        src_files = find_source_files(cfg["paths"]["src_dir"], codebase)
        func_metrics = []

        for test_file in sorted(func_dir.glob("run_*_test.c")):
            run_name = test_file.stem
            clean_build(build_dir)

            compiled, compile_log, exe = compile_test(
                test_file, src_files, unity_c, build_dir, gcc_flags, cfg
            )

            run_result = {
                "run": run_name,
                "compiled": compiled,
                "line_coverage": None,
                "branch_coverage": None,
            }

            if compiled:
                try:
                    returncode, test_output = run_test(exe, build_dir)
                    line_cov, branch_cov = measure_coverage(build_dir)
                    run_result["line_coverage"] = line_cov
                    run_result["branch_coverage"] = branch_cov
                    run_result["tests_passed"] = returncode == 0
                except subprocess.TimeoutExpired:
                    run_result["tests_passed"] = False
                    run_result["error"] = "timeout"
                except Exception:
                    error_stack = traceback.format_exc()
                    run_result["tests_passed"] = False
                    run_result["error"] = error_stack
            else:
                run_result["compiled"] = False
                run_result["error"] = f"Compilation failed: {compile_log.strip()}"
                print(compile_log)

            error_msg = ""
            if run_result.get("error"):
                lines = [line for line in run_result["error"].strip().splitlines() if line.strip()]
                error_msg = f" | Msg: {lines[-1]}" if lines else " | Msg: Unknown error"
                   
            func_metrics.append(run_result)
            status = "OK" if compiled else "FAIL"
            cov = run_result["line_coverage"]
            cov_str = f"{cov['percent']}%" if cov else "n/a"
            print(f"  [{status}] {model_name}/{func_name}/{run_name} -> line_cov: {cov_str} {error_msg}")

        all_metrics[func_name] = func_metrics

    metrics_file = metrics_dir / "model_metric_function.json"
    metrics_file.write_text(json.dumps(all_metrics, indent=2))
    print(f"  -> Metriken: {metrics_file}")
    clean_build(build_dir)


def main():
    import sys
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "deepseek", "qwen"]
    for model in models:
        print(f"\n=== Kompiliere & messe {model} ===")
        process_model(model, cfg)


if __name__ == "__main__":
    main()