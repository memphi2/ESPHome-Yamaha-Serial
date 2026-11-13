"""Check relative documentation links and their anchors without network dependencies."""

import re
from functools import cache
from pathlib import Path
from urllib.parse import unquote, urlsplit

from yamllint import linter
from yamllint.config import YamlLintConfig

ROOT = Path(__file__).resolve().parents[1]
YAML_BLOCK = re.compile(r"^```ya?ml\n(.*?)^```", re.M | re.S)

FENCE = re.compile(r"^(```|~~~).*?^\1", re.M | re.S)
HEADING = re.compile(r"^#{1,6}\s+(.+?)\s*#*\s*$", re.M)
HTML_ANCHOR = re.compile(r"<a\s+(?:id|name)=\"([^\"]+)\"")


def github_slug(heading: str) -> str:
    """Return the anchor GitHub generates for a Markdown heading."""
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", heading)  # links keep their text
    text = re.sub(r"[^\w\- ]", "", text.lower())
    return text.replace(" ", "-")


@cache
def anchors(path: Path) -> frozenset[str]:
    text = FENCE.sub("", path.read_text(encoding="utf-8"))
    found: set[str] = set(HTML_ANCHOR.findall(text))
    seen: dict[str, int] = {}
    for heading in HEADING.findall(text):
        slug = github_slug(heading)
        count = seen.get(slug, 0)
        seen[slug] = count + 1
        found.add(slug if count == 0 else f"{slug}-{count}")
    return frozenset(found)


def _links():
    for path in [*ROOT.glob("*.md"), *(ROOT / "docs").glob("*.md")]:
        text = FENCE.sub("", path.read_text(encoding="utf-8"))
        for target in re.findall(r"\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)", text):
            url = urlsplit(target.strip("<>"))
            if url.scheme or url.netloc:
                continue
            yield path, target, url


def test_relative_markdown_links_resolve():
    missing = []
    for path, target, url in _links():
        if url.path and not (path.parent / unquote(url.path)).exists():
            missing.append(f"{path.relative_to(ROOT)}: {target}")
    assert not missing, "\n".join(missing)


def test_markdown_link_anchors_exist():
    missing = []
    for path, target, url in _links():
        if not url.fragment:
            continue
        linked = (path.parent / unquote(url.path)) if url.path else path
        if linked.suffix != ".md" or not linked.exists():
            continue
        if unquote(url.fragment) not in anchors(linked.resolve()):
            missing.append(f"{path.relative_to(ROOT)}: {target}")
    assert not missing, "\n".join(missing)


def test_markdown_yaml_blocks_pass_yamllint():
    # CI lints examples/ with yamllint; YAML that lives in the docs gets the same rules.
    config = YamlLintConfig(file=str(ROOT / ".yamllint.yml"))
    problems = []
    for path in [*ROOT.glob("*.md"), *(ROOT / "docs").glob("*.md")]:
        text = path.read_text(encoding="utf-8")
        for block in YAML_BLOCK.finditer(text):
            first_line = text.count("\n", 0, block.start(1)) + 1
            for problem in linter.run(block[1], config):
                if problem.level == "error":
                    line = first_line + problem.line - 1
                    problems.append(f"{path.relative_to(ROOT)}:{line}: {problem.desc} ({problem.rule})")
    assert not problems, "\n".join(problems)


def test_github_slug_matches_github_rules():
    assert github_slug("RX-Vx700/x800") == "rx-vx700x800"
    assert github_slug("The `yamaha_serial` Block") == "the-yamaha_serial-block"
    assert github_slug("Support, Security and Privacy") == "support-security-and-privacy"
    assert github_slug("[0.2.4] - 2026-11-01") == "024---2026-11-01"
