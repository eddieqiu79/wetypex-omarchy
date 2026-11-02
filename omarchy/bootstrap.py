# SPDX-License-Identifier: GPL-3.0-only
"""Standalone first-install entry point, bundled as wetypex-install.pyz.

Requires system Python, pacman, bsdtar and a GitHub CLI that can verify releases.
This bootstrap uses pacman's normal interactive transaction, because the
fixed-purpose desktop helper is not installed yet. Subsequent updates use it.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--channel', choices=('stable', 'preview'), default='stable')
    parser.add_argument('--tag')
    parser.add_argument('--check', action='store_true', help='Check local prerequisites without network or installation')
    parser.add_argument('--download-only', action='store_true')
    args = parser.parse_args()
    if os.geteuid() == 0:
        parser.error('Run as your normal desktop user')
    with tempfile.TemporaryDirectory(prefix='wetypex-installer-') as temporary:
        root = Path(temporary)
        with zipfile.ZipFile(sys.argv[0]) as bundle:
            # Fixed allowlist: no arbitrary archive extraction or writable imports.
            for name in ('omarchy/wetypex-omarchy-package', 'scripts/fcitx5-wetypex-update.sh.in'):
                target = root/name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(bundle.read(name))
        core = runpy.run_path(str(root/'omarchy/wetypex-omarchy-package'))
        if args.check:
            return core['main'](['preflight'])
        command = ['download' if args.download_only else 'install', '--channel', args.channel]
        if args.tag: command += ['--tag', args.tag]
        if not args.download_only:
            command += ['--confirm', '--authorization', 'terminal-bootstrap']
        return core['main'](command)



if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError, EOFError, subprocess.SubprocessError):
        print('安装未完成。已下载的软件包会保留，请检查网络、依赖或授权后重试。')
        raise SystemExit(1)
