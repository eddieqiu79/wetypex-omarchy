#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Export a committed source snapshot and optionally build it with makepkg."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True, help='New output directory')
p.add_argument('--build', action='store_true', help='Build with installed dependencies; never install or sudo')
a = p.parse_args()
def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()
if git('status', '--porcelain', '--untracked-files=normal'):
    p.error('Commit or isolate changes first: only a clean committed source is exported.')
if a.build and os.geteuid() == 0:
    p.error('makepkg must run as an unprivileged build user.')
head = git('rev-parse', 'HEAD')
version = '2.2.3.657.r' + git('rev-list', '--count', 'HEAD') + '.g' + head[:12]
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
archive = out / 'wetypex-omarchy-source.tar.gz'
subprocess.run(['git', '-C', str(ROOT), 'archive', '--format=tar.gz', '--prefix=source/', '-o', str(archive), head], check=True)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
recipe = (ROOT / 'packaging/arch/PKGBUILD.in').read_text().replace('@VERSION@', version).replace('@SHA256@', digest)
(out / 'PKGBUILD').write_text(recipe)
shutil.copy2(ROOT / 'packaging/arch/wetypex-omarchy.install', out)
(out / 'source-manifest.json').write_text(json.dumps({'commit': head, 'version': version, 'source_sha256': digest}, indent=2)+'\n')
print(json.dumps({'build_directory': str(out), 'commit': head, 'version': version}))
if a.build:
    subprocess.run(['makepkg', '--cleanbuild', '--clean'], cwd=out, check=True)
