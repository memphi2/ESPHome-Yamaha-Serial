#!/usr/bin/env python3
from __future__ import annotations

from copy import deepcopy
from pathlib import Path

import yaml


class SecretSafeLoader(yaml.SafeLoader):
    pass


class Lambda(str):
    """Preserve ESPHome code through the YAML round trip."""


class ConfigDumper(yaml.SafeDumper):
    pass


def lambda_constructor(loader: yaml.SafeLoader, node: yaml.Node) -> Lambda:
    return Lambda(loader.construct_scalar(node))


def lambda_representer(dumper: yaml.SafeDumper, value: Lambda):
    return dumper.represent_scalar("!lambda", str(value))


def secret_constructor(loader: yaml.SafeLoader, node: yaml.Node) -> str:
    return f"!secret {loader.construct_scalar(node)}"


SecretSafeLoader.add_constructor("!secret", secret_constructor)
SecretSafeLoader.add_constructor("!lambda", lambda_constructor)
ConfigDumper.add_representer(Lambda, lambda_representer)


def replace_secrets(value, secrets):
    if isinstance(value, str) and value.startswith("!secret "):
        key = value.split(" ", 1)[1]
        return secrets[key]
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
        (out / name).write_text(yaml.dump(data, Dumper=ConfigDumper, sort_keys=False), encoding="utf-8")
        if name == "yamaha_rx_v_basic.yaml":
            for model, family in (("RX-V1500", "500"), ("RX-V1600", "600"), ("RX-V1700", "700"), ("RX-V1800", "800")):
                for suffix in ("", "_extended"):
                    variant = deepcopy(data)
                    variant["yamaha_serial"].update(model=model, receiver_profile=f"rx_vx{family}{suffix}")
                    (out / f"yamaha_rx_vx{family}{suffix}_profile.yaml").write_text(
                        yaml.dump(variant, Dumper=ConfigDumper, sort_keys=False), encoding="utf-8"
                    )
            for suffix in ("", "_extended"):
                variant = deepcopy(data)
                variant["yamaha_serial"].update(model="RX-V1900", receiver_profile=f"rx_v1900{suffix}")
                (out / f"yamaha_rx_v1900{suffix}_profile.yaml").write_text(
                    yaml.dump(variant, Dumper=ConfigDumper, sort_keys=False), encoding="utf-8"
                )


if __name__ == "__main__":
    main()
