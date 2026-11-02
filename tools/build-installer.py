#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build a source-only bootstrap zipapp; never download, install or publish."""
import argparse
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FILES = {'__main__.py': 'omarchy/bootstrap.py',
         'omarchy/wetypex-omarchy-package': 'omarchy/wetypex-omarchy-package',
         'scripts/fcitx5-wetypex-update.sh.in': 'scripts/fcitx5-wetypex-update.sh.in',
         'LICENSE': 'compat/LICENSE', 'LICENSES/WeTypeX-MIT.txt': 'LICENSES/WeTypeX-MIT.txt', 'NOTICE': 'NOTICE'}


def build(output):
    # Exclusive creation also protects unrelated output in shared workspaces.
    with output.open('xb') as stream:
        stream.write(b'#!/usr/bin/python3\n')
        with zipfile.ZipFile(stream, 'w', zipfile.ZIP_DEFLATED) as archive:
            for name, source in FILES.items():
                info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
                info.external_attr = 0o644 << 16
                archive.writestr(info, (ROOT/source).read_bytes())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    build(parser.parse_args().output)
