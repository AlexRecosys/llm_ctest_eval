"""Kompiliert generierte Tests und misst Coverage via gcov."""

import subprocess
import json
import re
import shutil
from pathlib import Path
import yaml
import traceback
import subprocess
import gzip


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
    print(f"Looking for source files in: {codebase_path}")
    return list(codebase_path.glob("*.c"))


def codebase_for_function(func_name, functions_dir):
    """Ermittelt die Codebase anhand des Ordners, in dem die Funktion liegt."""
    for codebase_dir in Path(functions_dir).iterdir():
        if not codebase_dir.is_dir():
            continue
        
        for f in codebase_dir.rglob("*.txt"):
            if f.stem == func_name:
                return codebase_dir.name
                
    return None



def compile_test(test_file, src_files, unity_c, build_dir, gcc_flags, cfg, func_name):
    # Eigenes Unterverzeichnis pro Test
    test_build_dir = (build_dir / f"test_{func_name}").resolve()
    test_build_dir.mkdir(parents=True, exist_ok=True)

    output_binary = test_build_dir / f"test_{func_name}"
    unity_include = Path(cfg["paths"]["unity_dir"]).resolve()
    src_root = Path(cfg["paths"]["src_dir"]).resolve()
    unique_codebase_dirs = {f.parent.resolve() for f in src_files}

    compiler = ["gcc"]
    flags    = gcc_flags
    includes = [f"-I{unity_include}", f"-I{src_root}"] + [f"-I{d}" for d in unique_codebase_dirs]
    sources  = [str(test_file.resolve()), str(unity_c.resolve())]
    output   = ["-o", str(output_binary), "-lm"]

    cmd = compiler + flags + includes + sources + output
    print(f"Compiling with command: {cmd}")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30, cwd=test_build_dir)
    log = result.stdout + result.stderr
    print(f"Compilation log:\n{log}")
    return result.returncode == 0, log, output_binary, test_build_dir

def run_test(exe, build_dir):
    """Führt das kompilierte Test-Binary aus."""
    result = subprocess.run(
        [str(exe)], capture_output=True, text=True, timeout=30, cwd=str(build_dir)
    )
    return result.returncode, result.stdout + result.stderr


def classify_test_result(returncode, test_output):
    if returncode == 0:
        return "passed"
    # Unkontrollierter Crash (Signal), kein Unity-Output
    if returncode < 0 or not test_output.strip():
        return "runtime_error"
    # Unity hat Segfault abgefangen – steht im Output
    if "Segmentation fault" in test_output or "SIGSEGV" in test_output:
        return "runtime_error"
    # Normale fehlgeschlagene Assertions
    if "FAIL" in test_output or "Expected" in test_output:
        return "assertion_error"
    return "runtime_error"  # Fallback


def parse_function_coverage(gcov_text, func_name):
    """Sucht im gcov-Output den 'Function <name>'-Block und liest dessen Coverage."""
    lines = gcov_text.splitlines()
    func_header = f"Function '{func_name}'"
 
    start = next((i for i, line in enumerate(lines) if func_header in line), None)
    if start is None:
        return None, None
 
    end = start + 1
    while end < len(lines) and not (
        lines[end].startswith("Function '") or lines[end].startswith("File '")
    ):
        end += 1
 
    block = "\n".join(lines[start:end])
    line_cov   = _extract_metric(block, r"Lines executed:(\d+\.\d+)% of (\d+)")
    branch_cov = _extract_metric(block, r"Branches executed:(\d+\.\d+)% of (\d+)")
    return line_cov, branch_cov
 
 
def _extract_metric(text, pattern):
    """Extrahiert percent/total/covered aus einer gcov-Zeile."""
    m = re.search(pattern, text)
    if not m:
        return None
    percent = float(m.group(1))
    total   = int(m.group(2))
    covered = round(percent / 100 * total)
    return {"percent": percent, "total": total, "covered": covered}


def measure_coverage(build_dir, func_name):
    """Line- und Branch-Coverage der Funktion via gcov --json-format."""
    gcda_files = list(build_dir.glob("*.gcda"))
    if not gcda_files:
        return None, None

    for gcda in gcda_files:
        result = subprocess.run(
            ["gcov", "-b", "--json-format", str(gcda.name)],
            capture_output=True, text=True, cwd=str(build_dir)
        )
        if result.returncode != 0:
            continue

        json_gz = build_dir / f"{gcda.stem}.gcov.json.gz"
        if not json_gz.exists():
            continue

        try:
            with gzip.open(json_gz, "rt", encoding="utf-8") as f:
                data = json.load(f)
        except (OSError, json.JSONDecodeError):
            continue

        cov = _coverage_from_json(data, func_name)
        if cov is not None:
            return cov

    return None, None

def _coverage_from_json(data, func_name):
    for file_entry in data.get("files", []):
        func_names = {fn["name"] for fn in file_entry.get("functions", [])}
        if func_name not in func_names:
            continue

        lines = [l for l in file_entry.get("lines", [])
                 if l.get("function_name") == func_name]
        if not lines:
            continue

        total_lines   = len(lines)
        covered_lines = sum(1 for l in lines if l["count"] > 0)
        branches      = [b for l in lines for b in l.get("branches", [])]
        total_br      = len(branches)
        covered_br    = sum(1 for b in branches if b["count"] > 0)

        def metric(covered, total):
            if total == 0:
                return None
            return {"percent": round(100 * covered / total, 2),
                    "total": total, "covered": covered}

        return metric(covered_lines, total_lines), metric(covered_br, total_br)

    return None

def clean_build(build_dir):
    if build_dir.exists():
        shutil.rmtree(build_dir)



def process_model(model_name, cfg):
    """Verarbeitet alle Tests eines Modells."""
    tests_base = Path(cfg["paths"]["output_tests"]) / f"{model_name}_modell"
    metrics_dir = Path(cfg["paths"]["output_metrics"]) / f"{model_name}_modell"
    metrics_dir.mkdir(parents=True, exist_ok=True)

    build_dir = Path(cfg["paths"]["build_dir"]) / f"{model_name}_modell"
    build_dir.mkdir(parents=True, exist_ok=True)
    
    unity_c = Path(cfg["paths"]["unity_dir"]) / "unity.c"
    gcc_flags = cfg["experiment"]["gcc_flags"]

    all_metrics = {}

    if not tests_base.exists():
        print(f"  Keine Tests gefunden für {model_name}")
        return

    for func_dir in sorted(tests_base.iterdir()):
        if not func_dir.is_dir():
            continue
        func_name = func_dir.name
        codebase = codebase_for_function(func_name, cfg["paths"]["functions_dir"])
        print(f'CODEBASE: {codebase}')
        print(f'FUNC_DIR: {func_dir}')
        print(f'FUNC_NAME: {func_name}')
        if not codebase:
            print(f"  [WARN] Keine Codebase für {func_name}")
            continue

        src_files = find_source_files(cfg["paths"]["src_dir"], codebase)
        func_metrics = []

        for test_file in sorted(func_dir.glob("run_*_test.c")):
            run_name = test_file.stem

            compiled, compile_log, exe, test_build_dir = compile_test(
                test_file, src_files, unity_c, build_dir, gcc_flags, cfg, f"{func_name}-{run_name}"
            )

            run_result = {
                "run": run_name,
                "compiled": compiled,
                "returncode": None,
                "test_status": None,
                "test_output": None,
                "line_coverage": None,
                "branch_coverage": None,
            }

            if compiled:
                try:
                    returncode, test_output = run_test(exe, test_build_dir)
                    line_cov, branch_cov = measure_coverage(test_build_dir, func_name)

                    run_result["returncode"]      = returncode
                    run_result["test_output"]     = test_output
                    run_result["test_status"]     = classify_test_result(returncode, test_output)
                    run_result["line_coverage"]   = line_cov
                    run_result["branch_coverage"] = branch_cov

                    print(f"   Run {run_name} | status: {run_result['test_status']} (exit {returncode})")
                    print(f"   Test output:\n{test_output}")
                except subprocess.TimeoutExpired:
                    run_result["test_status"] = "timeout"
                    run_result["error"] = "timeout"
                except Exception:
                    error_stack = traceback.format_exc()
                    run_result["test_status"] = "error"
                    run_result["error"] = error_stack
            else:
                run_result["test_status"] = "compile_error"
                run_result["error"] = f"Compilation failed: {compile_log.strip()}"
                print(compile_log)

            # Aufräumen: .gcda und .gcno entfernen
            if test_build_dir.exists():
                for f in test_build_dir.glob("*.gcda"):
                    f.unlink()
                for f in test_build_dir.glob("*.gcno"):
                    f.unlink()

            error_msg = ""
            if run_result.get("error"):
                lines = [line for line in run_result["error"].strip().splitlines() if line.strip()]
                error_msg = f" | Msg: {lines[-1]}" if lines else " | Msg: Unknown error"

            func_metrics.append(run_result)
            status = "OK" if compiled else "FAIL"
            cov = run_result["line_coverage"]
            cov_str = f"{cov['percent']}%" if cov else "n/a"
            print(f"  [{status}] {model_name}/{func_name}/{run_name} -> line_cov: {cov_str} | test_status: {run_result['test_status']}{error_msg}")

        all_metrics[func_name] = func_metrics

    metrics_file = metrics_dir / "model_metric_function.json"
    metrics_file.write_text(json.dumps(all_metrics, indent=2))
    print(f"  -> Metriken: {metrics_file}")


def main():
    import sys
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "deepseek", "qwen"]
    for model in models:
        print(f"\n=== Kompiliere & messe {model} ===")
        process_model(model, cfg)


if __name__ == "__main__":
    main()
