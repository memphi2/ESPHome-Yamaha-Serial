#!/usr/bin/env python3
from __future__ import annotations

import re
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


# Receiver configurations in the user docs are overlays on an example: each top-level
# block replaces or adds that block of the example, as the docs tell the reader to do.
# CI validates the merged result, so the documented YAML cannot silently go stale.
DOC_CONFIG_SOURCES = ("docs/supported_models.md",)
DOC_CONFIG = re.compile(
    r"<!-- ci-config: (?P<name>yamaha_rx_v[\w-]*\.yaml) from (?P<base>yamaha_rx_v[\w-]*\.yaml) -->\n"
    r"```yaml\n(?P<body>.*?)\n```",
    re.S,
)


def load_config(text: str):
    return yaml.load(text, Loader=SecretSafeLoader)


def doc_configs(root: Path) -> dict[str, dict]:
    """Return the documented receiver configurations, merged onto their base example."""
    configs: dict[str, dict] = {}
    for source in DOC_CONFIG_SOURCES:
        for match in DOC_CONFIG.finditer((root / source).read_text(encoding="utf-8")):
            name = match["name"]
            if name in configs or (root / "examples" / name).exists():
                raise ValueError(f"Duplicate CI config name in {source}: {name}")
            base = load_config((root / "examples" / match["base"]).read_text(encoding="utf-8"))
            configs[name] = {**base, **load_config(match["body"])}
    return configs


PROFILE_FAMILIES = (("RX-V1500", "500"), ("RX-V1600", "600"), ("RX-V1700", "700"), ("RX-V1800", "800"))


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    out = root / ".ci"
    out.mkdir(exist_ok=True)
    (out / ".gitignore").write_text("/.esphome/\n", encoding="utf-8")

    secrets = yaml.safe_load((root / "examples" / "secrets.example.yaml").read_text(encoding="utf-8")) or {}

    configs = {
        src.name: load_config(src.read_text(encoding="utf-8"))
        for src in sorted((root / "examples").glob("yamaha_rx_v*.yaml"))
    }
    configs.update(doc_configs(root))
    outputs: dict[str, dict] = {}

    def emit(name: str, data: dict) -> None:
        if name in outputs:
            raise ValueError(f"Two CI configs would be written to .ci/{name}")
        outputs[name] = data

    for name, data in configs.items():
        data = replace_secrets(data, secrets)
        data["external_components"] = [{"source": {"type": "local", "path": "../components"}, "components": ["yamaha_serial"]}]
        emit(name, data)
        if name == "yamaha_rx_v_basic.yaml":
            # Every profile compiles against the basic example's entity set.
            profiles = [(model, f"rx_vx{family}") for model, family in PROFILE_FAMILIES] + [("RX-V1900", "rx_v1900")]
            for model, profile in profiles:
                for suffix in ("", "_extended"):
                    variant = deepcopy(data)
                    variant["yamaha_serial"].update(model=model, receiver_profile=f"{profile}{suffix}")
                    emit(f"yamaha_{profile}{suffix}_profile.yaml", variant)

    for name, data in outputs.items():
        (out / name).write_text(yaml.dump(data, Dumper=ConfigDumper, sort_keys=False), encoding="utf-8")


if __name__ == "__main__":
    main()
