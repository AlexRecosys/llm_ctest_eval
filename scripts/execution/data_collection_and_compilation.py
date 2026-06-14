
import subprocess
from pathlib import Path
import yaml

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


def find_source_files(src_dir, codebase_name):
    """Findet alle .c-Dateien einer Codebase fuer die Kompilierung."""
    codebase_path = Path(src_dir) / codebase_name
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


def build_include_flags(cfg, src_files):
    unity_include = Path(cfg["paths"]["unity_dir"]).resolve()
    src_root = Path(cfg["paths"]["src_dir"]).resolve()
    unique_dirs = sorted({str(f.parent.resolve()) for f in src_files})
    return [f"-I{unity_include}", f"-I{src_root}"] + [f"-I{d}" for d in unique_dirs]


def ensure_coverage_flags(gcc_flags):
    """Stellt sicher, dass die Instrumentierung (--coverage) aktiv ist."""
    flags = list(gcc_flags)
    joined = " ".join(flags)
    if "--coverage" not in joined and "-fprofile-arcs" not in joined:
        flags.append("--coverage")
    return flags



def compile_test(test_file, src_files, unity_c, build_dir, gcc_flags, cfg, tag):
    """Kompiliert einen Testlauf in ein eigenes Unterverzeichnis."""
    test_build_dir = (build_dir / f"test_{tag}").resolve()
    test_build_dir.mkdir(parents=True, exist_ok=True)
    output_binary = test_build_dir / f"test_{tag}"

    includes = build_include_flags(cfg, src_files)
    cmd = (["gcc"] + list(gcc_flags) + includes
           + [str(test_file.resolve()), str(unity_c.resolve())]
           + ["-o", str(output_binary), "-lm"])

    try:
        result = subprocess.run(cmd, capture_output=True, text=True,
                                timeout=60, cwd=test_build_dir)
    except subprocess.TimeoutExpired:
        return False, "compiler timeout", output_binary, test_build_dir

    log = result.stdout + result.stderr
    return result.returncode == 0, log, output_binary, test_build_dir


def run_test(exe, build_dir):
    """Fuehrt das kompilierte Test-Binary aus."""
    result = subprocess.run([str(exe)], capture_output=True, text=True,
                            timeout=30, cwd=str(build_dir))
    return result.returncode, result.stdout + result.stderr