import gzip
import json
import re
import subprocess
import traceback
from pathlib import Path

import yaml

from data_collection_and_compilation  import (build_include_flags, find_source_files, codebase_for_function, ensure_coverage_flags,
                                               compile_test, run_test)

RUN_FILE_PATTERN = "run_*_test.c"



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




def classify_test_result(returncode, test_output):
    if returncode == 0:
        return "passed"
    if returncode < 0 or not test_output.strip():
        return "runtime_error"
    if "Segmentation fault" in test_output or "SIGSEGV" in test_output:
        return "runtime_error"
    if "FAIL" in test_output or "Expected" in test_output:
        return "assertion_error"
    return "runtime_error"


def classify_translation_error(compile_log):
    """Linker-Fehler vs. Compile-Fehler (binaere Status-Flags im Schema)."""
    if compile_log and "undefined reference" in compile_log:
        return "linker_error"
    return "compile_error"


def classify_fut_run_outcome(run_result):
    """FUT-Lauf-Klassifikation  RE + AE + P = 1.
    """
    if run_result.get("all_tests_passed"):
        return {"runtime_error_status": 0,
                "assertion_error_status": 0,
                "pass_status": 1}

    is_assertion = (
        run_result.get("compiled")
        and run_result.get("test_status") == "assertion_error"
        and not run_result.get("runtime_errors")
    )
    if is_assertion:
        return {"runtime_error_status": 0,
                "assertion_error_status": 1,
                "pass_status": 0}

    return {"runtime_error_status": 1,
            "assertion_error_status": 0,
            "pass_status": 0}


def count_test_metrics(test_output):
    """Zaehlt Runtime-Fehler (Segfaults), Assertion-Fehler, bestandene und
    insgesamt ausgefuehrte Tests aus dem Unity-Output."""
    runtime = assertion = passed = 0
    for line in (test_output or "").splitlines():
        if "SIGSEGV" in line or "Segmentation fault" in line:
            runtime += 1
        elif ":PASS" in line:
            passed += 1
        elif ":FAIL" in line:
            assertion += 1

    summary = re.search(r"(\d+)\s+Tests\s+(\d+)\s+Failures\s+(\d+)\s+Ignored",
                        test_output or "")
    tests_total = int(summary.group(1)) if summary else 0
    # Identitaet RE + AE + P <= T absichern (Crash vor der Summary-Zeile).
    tests_total = max(tests_total, passed + assertion + runtime)

    return {"tests_total": tests_total, "tests_passed": passed,
            "runtime_errors": runtime, "assertion_errors": assertion}



def _gcov_generate_json(build_dir, note_or_data_files):
    """Erzeugt fuer die uebergebenen .gcno/.gcda-Dateien die .gcov.json.gz."""
    for f in note_or_data_files:
        subprocess.run(["gcov", "-b", "--json-format", str(f.name)],
                       capture_output=True, text=True, cwd=str(build_dir))


def _load_gcov_json(build_dir):
    for gz in sorted(build_dir.glob("*.gcov.json.gz")):
        try:
            with gzip.open(gz, "rt", encoding="utf-8") as fh:
                yield json.load(fh)
        except (OSError, json.JSONDecodeError):
            continue


def _function_lines(file_entry, func_name):
    return [l for l in file_entry.get("lines", [])
            if l.get("function_name") == func_name]


def _line_and_branch_sets(lines):
    """Extrahiert Zeilennummern und Branch-IDs """
    line_numbers, covered_lines = set(), set()
    branch_ids, covered_branches = set(), set()
    for l in lines:
        ln = l.get("line_number")
        line_numbers.add(ln)
        if (l.get("count") or 0) > 0:
            covered_lines.add(ln)
        for i, b in enumerate(l.get("branches", [])):
            bid = f"{ln}:{i}"
            branch_ids.add(bid)
            if (b.get("count") or 0) > 0:
                covered_branches.add(bid)
    return (sorted(line_numbers), sorted(covered_lines),
            sorted(branch_ids), sorted(covered_branches))


def extract_function_coverage(data, func_name):
    for file_entry in data.get("files", []):
        names = {fn.get("name") for fn in file_entry.get("functions", [])}
        if func_name not in names:
            continue
        lines = _function_lines(file_entry, func_name)
        if not lines:
            continue
        line_numbers, covered_lines, branch_ids, covered_branches = \
            _line_and_branch_sets(lines)
        return {"line_numbers": line_numbers, "covered_lines": covered_lines,
                "branch_ids": branch_ids, "covered_branches": covered_branches}
    return None



def measure_static_totals(cfg, codebase, src_files, build_dir, gcc_flags):
    """Liest lines_total, branches_total """
    static_dir = (build_dir / f"_static_{codebase}").resolve()
    static_dir.mkdir(parents=True, exist_ok=True)
    includes = build_include_flags(cfg, src_files)
    flags = ensure_coverage_flags(gcc_flags)

    gcno_files = []
    for src in src_files:
        obj = static_dir / (src.stem + ".o")
        cmd = ["gcc"] + flags + includes + ["-c", str(src.resolve()), "-o", str(obj)]
        try:
            subprocess.run(cmd, capture_output=True, text=True,
                           timeout=60, cwd=static_dir)
        except subprocess.TimeoutExpired:
            continue
        gcno = static_dir / (src.stem + ".gcno")
        if gcno.exists():
            gcno_files.append(gcno)

    _gcov_generate_json(static_dir, gcno_files)

    totals = {}
    for data in _load_gcov_json(static_dir):
        for file_entry in data.get("files", []):
            for fn in file_entry.get("functions", []):
                name = fn.get("name")
                if not name or name in totals:
                    continue
                lines = _function_lines(file_entry, name)
                if not lines:
                    continue
                line_numbers, _, branch_ids, _ = _line_and_branch_sets(lines)
                totals[name] = {
                    "lines_total": len(line_numbers),
                    "branches_total": len(branch_ids),
                    "line_numbers": line_numbers,
                    "branch_ids": branch_ids,
                }
    return totals



def measure_run_coverage(test_build_dir, func_name):
    """Coverage eines Laufs aus den .gcda-Daten der FUT extrahieren."""
    gcda_files = list(test_build_dir.glob("*.gcda"))
    if not gcda_files:
        return None
    _gcov_generate_json(test_build_dir, gcda_files)
    for data in _load_gcov_json(test_build_dir):
        cov = extract_function_coverage(data, func_name)
        if cov is not None:
            return cov
    return None


def apply_coverage(run_result, run_cov, static):

    if static:
        run_result["lines_total"] = static["lines_total"]
        run_result["branches_total"] = static["branches_total"]

    if run_cov:
        covered_lines = set(run_cov["covered_lines"])
        covered_branches = set(run_cov["covered_branches"])
        if static:
            covered_lines &= set(static["line_numbers"])
            covered_branches &= set(static["branch_ids"])
        else:
            run_result["lines_total"] = len(run_cov["line_numbers"])
            run_result["branches_total"] = len(run_cov["branch_ids"])
        run_result["covered_lines"] = sorted(covered_lines)
        run_result["covered_branches"] = sorted(covered_branches)
        run_result["lines_covered"] = len(covered_lines)
        run_result["branches_covered"] = len(covered_branches)
        run_result["coverage_source"] = "gcov"
    else:
        run_result["covered_lines"] = []
        run_result["covered_branches"] = []
        has_totals = run_result["lines_total"] is not None
        run_result["lines_covered"] = 0 if has_totals else None
        run_result["branches_covered"] = 0 if run_result["branches_total"] is not None else None
        run_result["coverage_source"] = "static_penalty" if static else "unavailable"



def process_model(model_name, cfg):
    """Misst alle Tests eines Modells und schreibt raw_metrics.json."""
    tests_base = Path(cfg["paths"]["output_tests"]) / f"{model_name}_modell"
    metrics_dir = Path(cfg["paths"]["output_metrics"]) / f"{model_name}_modell"
    metrics_dir.mkdir(parents=True, exist_ok=True)

    build_dir = Path(cfg["paths"]["build_dir"]) / f"{model_name}_modell"
    build_dir.mkdir(parents=True, exist_ok=True)

    unity_c = Path(cfg["paths"]["unity_dir"]) / "unity.c"
    gcc_flags = ensure_coverage_flags(cfg["experiment"]["gcc_flags"])

    if not tests_base.exists():
        print(f"  Keine Tests gefunden fuer {model_name}")
        return None

    all_metrics = {}
    fut_totals = {}
    static_cache = {}

    for func_dir in sorted(tests_base.iterdir()):
        if not func_dir.is_dir():
            continue
        func_name = func_dir.name
        codebase = codebase_for_function(func_name, cfg["paths"]["functions_dir"])
        if not codebase:
            print(f"  [WARN] Keine Codebase fuer {func_name}")
            continue

        src_files = find_source_files(cfg["paths"]["src_dir"], codebase)
        if codebase not in static_cache:
            static_cache[codebase] = measure_static_totals(
                cfg, codebase, src_files, build_dir, gcc_flags)
        static = static_cache[codebase].get(func_name)
        if static is None:
            print(f"  [WARN] Keine statischen Totals fuer {func_name} "
                  f"(Codebase {codebase})")

        fut_totals[func_name] = {
            "codebase": codebase,
            "lines_total": static["lines_total"] if static else None,
            "branches_total": static["branches_total"] if static else None,
        }

        func_metrics = []
        for test_file in sorted(func_dir.glob(RUN_FILE_PATTERN)):
            run_name = test_file.stem
            compiled, compile_log, exe, test_build_dir = compile_test(
                test_file, src_files, unity_c, build_dir, gcc_flags, cfg,
                f"{func_name}-{run_name}")

            run_result = {
                "run": run_name,
                "codebase": codebase,
                "compiled": compiled,
                "compile_error_status": 0,
                "linker_error_status": 0,
                "return_code": None,
                "test_status": None,
                "all_tests_passed": False,
                "runtime_error_status": 0,
                "assertion_error_status": 0,
                "pass_status": 0,
                "tests_total": 0,
                "tests_passed": 0,
                "runtime_errors": 0,
                "assertion_errors": 0,
                "test_output": "",
                "error_message": "",
                "lines_total": None,
                "lines_covered": None,
                "branches_total": None,
                "branches_covered": None,
                "covered_lines": [],
                "covered_branches": [],
                "coverage_source": None,
            }

            run_cov = None
            if compiled:
                try:
                    return_code, test_output = run_test(exe, test_build_dir)
                    run_result["return_code"] = return_code
                    run_result["test_output"] = test_output
                    run_result["test_status"] = classify_test_result(return_code, test_output)
                    run_result.update(count_test_metrics(test_output))
                    run_cov = measure_run_coverage(test_build_dir, func_name)
                except subprocess.TimeoutExpired:
                    run_result["test_status"] = "timeout"
                    run_result["error_message"] = "timeout"
                except Exception:
                    run_result["test_status"] = "error"
                    run_result["error_message"] = traceback.format_exc()
            else:
                err_type = classify_translation_error(compile_log)
                run_result["test_status"] = err_type
                run_result["compile_error_status"] = int(err_type == "compile_error")
                run_result["linker_error_status"] = int(err_type == "linker_error")
                run_result["error_message"] = (compile_log or "").strip()

            apply_coverage(run_result, run_cov, static)
            run_result["all_tests_passed"] = bool(
                compiled
                and run_result["test_status"] == "passed"
                and run_result["tests_total"] > 0
                and run_result["tests_passed"] == run_result["tests_total"])


            run_result.update(classify_fut_run_outcome(run_result))

            for pattern in ("*.gcda", "*.gcno", "*.gcov.json.gz"):
                for f in test_build_dir.glob(pattern):
                    f.unlink()

            func_metrics.append(run_result)

            status = "OK" if compiled else "FAIL"
            lt, lc = run_result["lines_total"], run_result["lines_covered"]
            cov_str = f"{lc}/{lt}" if lt is not None else "n/a"
            print(f"  [{status}] {model_name}/{func_name}/{run_name} "
                  f"-> lines: {cov_str} | test_status: {run_result['test_status']}")

        all_metrics[func_name] = func_metrics

    raw = {"model": model_name, "fut_totals": fut_totals, "all_metrics": all_metrics}
    raw_file = metrics_dir / "raw_metrics.json"
    raw_file.write_text(json.dumps(raw, indent=2))
    print(f"  -> Rohmetriken: {raw_file}")
    print("  -> Naechster Schritt: 02_compute_metrics.py fuer die drei "
          "Ziel-JSONs ausfuehren.")
    return raw


def main():
    import sys
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else ["claude", "qwen"]

    for model in models:
        print(f"\n=== Kompiliere & messe {model} ===")
        process_model(model, cfg)


if __name__ == "__main__":
    main()
