from __future__ import annotations

import pathlib
import re

import pytest

ROOT = pathlib.Path(__file__).resolve().parents[1]
WORKFLOWS_DIR = ROOT / ".github" / "workflows"

# Any spelling GitHub honours for a "uses" key: block style, a quoted key, an
# extra space before the colon, or a flow mapping where uses is not first.
USES_BLOCK_KEY = re.compile(r"""^(?:-\s+)?\{?\s*["']?uses["']?\s*:""")
USES_FLOW_KEY = re.compile(r"""[{,]\s*["']?uses["']?\s*:""")
USES_VALUE = re.compile(r"""["']?uses["']?\s*:\s*["']?(?P<value>[^\s,}"']+)""")
# Local and container actions are referenced by path or image, never by SHA.
UNPINNABLE_PREFIXES = ("./", "../", ".\\", "docker://")
# The one spelling this audit reads in full, because only here can it tell
# which release a SHA belongs to. Anything else is reported, never skipped.
CANONICAL_USES = re.compile(
    r"^\s*(?:-\s+)?uses:\s*(?P<action>[^@\s]+)@(?P<ref>\S+)"
    r"(?:\s+#\s*(?P<comment>\S+).*)?$"
)
ACTION_SHA = re.compile(r"^[0-9a-f]{40}$")
# A SHA says nothing about which release it is, so the trailing comment has to
# name one. Branch-pinned actions name the branch instead of a version.
ACTION_VERSION = re.compile(r"^(?:v\d+(?:\.\d+)*|main|master)$")


def declares_uses_key(line: str) -> bool:
    """Return whether the line declares a `uses` key GitHub would honour.

    Prose and run-step text are not keys, so a match is tied to a position a
    key can occupy: the start of a block entry, or after `{` or `,` inside a
    flow mapping on the same line.
    """

    stripped = line.lstrip()
    if stripped.startswith("#"):
        return False
    if USES_BLOCK_KEY.match(stripped):
        return True
    return "{" in line and USES_FLOW_KEY.search(line) is not None


def workflow_paths() -> list[pathlib.Path]:
    """Every workflow file, so a file added later is covered as well."""

    return sorted(
        path for path in WORKFLOWS_DIR.iterdir() if path.suffix in {".yml", ".yaml"}
    )


def test_workflows_are_present() -> None:
    assert workflow_paths(), "no workflow files found to audit"


@pytest.mark.parametrize(
    "workflow_path", workflow_paths(), ids=lambda path: path.name
)
def test_every_action_is_pinned_to_a_labelled_sha(workflow_path: pathlib.Path) -> None:
    """A tag is a moving pointer; only a SHA says what CI actually runs."""

    for number, line in enumerate(
        workflow_path.read_text(encoding="utf-8").splitlines(), start=1
    ):
        if not declares_uses_key(line):
            continue
        values = [found.group("value") for found in USES_VALUE.finditer(line)]
        if values and all(value.startswith(UNPINNABLE_PREFIXES) for value in values):
            continue
        where = f"{workflow_path.name}:{number}"
        match = CANONICAL_USES.match(line)
        assert match is not None, (
            f"{where}: write this as 'uses: <action>@<sha> # <version>' so the "
            f"pin audit can read it: {line.strip()!r}"
        )
        action, ref = match.group("action"), match.group("ref")
        assert ACTION_SHA.match(ref), (
            f"{where}: {action}@{ref} is not a commit SHA"
        )
        comment = match.group("comment")
        assert comment is not None and ACTION_VERSION.match(comment), (
            f"{where}: {action} is pinned to {ref[:12]} without a readable "
            f"'# <version>' comment, so the pin cannot be traced to a release"
        )
