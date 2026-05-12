"""Generiert Unit-Tests über LLM-APIs für alle konfigurierten Modelle."""

import os
import sys
import yaml
import requests
import json
import re
from pathlib import Path


def load_config():
    with open("config.yaml") as f:
        return yaml.safe_load(f)


def load_prompt_template(cfg):
    prompt_path = Path(cfg["paths"]["prompt_dir"]) / cfg["experiment"]["prompt_version"]
    return prompt_path.read_text()


def get_functions(cfg):
    """Sammelt alle Funktions-Dateien aus allen Codebases."""
    functions = []
    func_dir = Path(cfg["paths"]["functions_dir"])
    for codebase_dir in sorted(func_dir.iterdir()):
        if not codebase_dir.is_dir():
            continue
        codebase_name = codebase_dir.name
        header_name = cfg["codebases"].get(codebase_name, "unknown.h")
        for func_file in sorted(codebase_dir.glob("*.txt")):
            functions.append({
                "name": func_file.stem,
                "code": func_file.read_text(),
                "codebase": codebase_name,
                "header": header_name,
            })
    return functions


def call_claude(prompt, cfg):
    model_cfg = cfg["models"]["claude"]
    headers = {
        "x-api-key": cfg["api_keys"]["claude"],
        "content-type": "application/json",
        "anthropic-version": "2023-06-01",
    }
    body = {
        "model": model_cfg["model_name"],
        "max_tokens": 4096,
        "temperature": model_cfg["temperature"],
        "messages": [{"role": "user", "content": prompt}],
    }
    resp = requests.post(model_cfg["api_url"], headers=headers, json=body, timeout=120)
    resp.raise_for_status()
    return resp.json()["content"][0]["text"]


def call_deepseek(prompt, cfg):
    model_cfg = cfg["models"]["deepseek"]
    headers = {
        "Authorization": f"Bearer {cfg['api_keys']['deepseek']}",
        "Content-Type": "application/json",
    }
    body = {
        "model": model_cfg["model_name"],
        "temperature": model_cfg["temperature"],
        "messages": [{"role": "user", "content": prompt}],
    }
    resp = requests.post(model_cfg["api_url"], headers=headers, json=body, timeout=120)
    resp.raise_for_status()
    return resp.json()["choices"][0]["message"]["content"]


def call_qwen(prompt, cfg):
    """Qwen lokal via OpenAI-kompatible API (z.B. Ollama)."""
    model_cfg = cfg["models"]["qwen"]
    headers = {"Content-Type": "application/json"}
    body = {
        "model": model_cfg["model_name"],
        "temperature": model_cfg["temperature"],
        "messages": [{"role": "user", "content": prompt}],
    }
    resp = requests.post(model_cfg["api_url"], headers=headers, json=body, timeout=300)
    resp.raise_for_status()
    return resp.json()["choices"][0]["message"]["content"]


API_DISPATCH = {
    "claude": call_claude,
    "deepseek": call_deepseek,
    "qwen": call_qwen,
}


def extract_c_code(response_text):
    """Extrahiert C-Code aus der LLM-Antwort, entfernt ggf. Markdown-Blöcke."""
    match = re.search(r"```c?\s*\n(.*?)```", response_text, re.DOTALL)
    if match:
        return match.group(1).strip()
    return response_text.strip()


def generate_tests(model_name, cfg):
    prompt_template = load_prompt_template(cfg)
    functions = get_functions(cfg)
    call_api = API_DISPATCH[model_name]
    output_base = Path(cfg["paths"]["output_tests"]) / f"{model_name}_modell"
    runs = cfg["experiment"]["runs_per_function"]

    for func in functions:
        prompt = prompt_template.format(
            function_under_test=func["code"],
            header_name=func["header"],
        )
        func_dir = output_base / func["name"]
        func_dir.mkdir(parents=True, exist_ok=True)

        for run_idx in range(1, runs + 1):
            out_file = func_dir / f"run_{run_idx:02d}_test.c"
            if out_file.exists():
                print(f"  [skip] {out_file} existiert bereits")
                continue
            try:
                print(f"  [{model_name}] {func['name']} run {run_idx}/{runs} ...")
                raw = call_api(prompt, cfg)
                code = extract_c_code(raw)
                out_file.write_text(code)
                print(f"  -> {out_file}")
            except Exception as e:
                print(f"  [FEHLER] {func['name']} run {run_idx}: {e}")


def main():
    cfg = load_config()
    models = sys.argv[1:] if len(sys.argv) > 1 else list(API_DISPATCH.keys())
    for model in models:
        if model not in API_DISPATCH:
            print(f"Unbekanntes Modell: {model}")
            continue
        print(f"\n=== Generiere Tests mit {model} ===")
        generate_tests(model, cfg)


if __name__ == "__main__":
    main()