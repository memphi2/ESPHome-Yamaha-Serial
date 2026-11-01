"""Check relative documentation links without network dependencies."""

import re
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def test_relative_markdown_links_resolve():
    missing = []
    for path in [*ROOT.glob("*.md"), *(ROOT / "docs").glob("*.md")]:
        for target in re.findall(r"\]\(([^)]+)\)", path.read_text()):
            url = urlsplit(target.strip("<>"))
            if url.scheme or url.netloc or not url.path:
                continue
            if not (path.parent / unquote(url.path)).exists():
                missing.append(f"{path.relative_to(ROOT)}: {target}")
    assert not missing, "\n".join(missing)
