#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Report upstream drift without fetching code, merging or changing local state."""
import json
from pathlib import Path
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def get(path):
    request = urllib.request.Request("https://api.github.com/repos/panxuc/fcitx5-wetypex/" + path,
                                     headers={"User-Agent": "wetypex-omarchy-upstream-watch"})
    with urllib.request.urlopen(request, timeout=15) as response:
        body = response.read(2 * 1024 * 1024 + 1)
    if len(body) > 2 * 1024 * 1024:
        raise ValueError("metadata too large")
    return json.loads(body)


def main():
    base = json.loads((ROOT / "docs/upstream-source-manifest.json").read_text())["commit"]
    latest = get("commits?per_page=1")[0]["sha"]
    if not all(re.fullmatch(r"[a-f0-9]{40}", sha) for sha in (base, latest)):
        raise ValueError("invalid source commit")
    result = {"upstream": "panxuc/fcitx5-wetypex", "integrated_base": base,
              "upstream_head": latest, "review_required": base != latest,
              "compare": f"https://github.com/panxuc/fcitx5-wetypex/compare/{base}...{latest}",
              "automatic_merge": False}
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, IndexError, TypeError):
        raise SystemExit("Upstream check failed; no up-to-date conclusion can be drawn")
