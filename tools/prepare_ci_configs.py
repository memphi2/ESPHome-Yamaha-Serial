#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

import yaml


class SecretSafeLoader(yaml.SafeLoader):
    pass


def secret_constructor(loader: yaml.SafeLoader, node: yaml.Node) -> str:
    return f"!secret {loader.construct_scalar(node)}"


SecretSafeLoader.add_constructor("!secret", secret_constructor)
SecretSafeLoader.add_constructor("!lambda", secret_constructor)


def replace_secrets(value, secrets):
    if isinstance(value, str) and value.startswith("!secret "):
        key = value.split(" ", 1)[1]
        return secrets.get(key, value)
    if isinstance(value, list):
        return [replace_secrets(item, secrets) for item in value]
    if isinstance(value, dict):
        return {k: replace_secrets(v, secrets) for k, v in value.items()}
    return value


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    out = root / ".ci"
    out.mkdir(exist_ok=True)
    (out / ".gitignore").write_text("/.esphome/\n", encoding="utf-8")

    secrets = yaml.safe_load((root / "examples" / "secrets.example.yaml").read_text(encoding="utf-8")) or {}

    for src in sorted((root / "examples").glob("yamaha_rx_v*.yaml")):
        name = src.name
        data = yaml.load(src.read_text(encoding="utf-8"), Loader=SecretSafeLoader)
        data = replace_secrets(data, secrets)
        data["external_components"] = [{"source": {"type": "local", "path": "../components"}, "components": ["yamaha_serial"]}]
        (out / name).write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")


if __name__ == "__main__":
    main()
