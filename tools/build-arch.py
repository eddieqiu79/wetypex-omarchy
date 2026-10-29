#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Export a committed source snapshot and optionally build it with makepkg."""
import argparse
import ast
import hashlib
import json
import os
import re
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
runtime_tree = ast.parse((ROOT / 'scripts/setup_runtime.py').read_text())
runtime_version = next(ast.literal_eval(n.value) for n in runtime_tree.body
                       if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'UPSTREAM_VERSION' for t in n.targets))
project_version = (ROOT / 'VERSION').read_text().strip()
if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-dev)?', project_version):
    p.error('VERSION must be MAJOR.MINOR.PATCH or MAJOR.MINOR.PATCH-dev')
version = project_version.replace('-dev', 'dev')
if project_version.endswith('-dev'):
    version += '.r' + git('rev-list', '--count', 'HEAD') + '.g' + head[:12]
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
archive = out / 'wetypex-omarchy-source.tar.gz'
subprocess.run(['git', '-C', str(ROOT), 'archive', '--format=tar.gz', '--prefix=source/', '-o', str(archive), head], check=True)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
recipe = (ROOT / 'packaging/arch/PKGBUILD.in').read_text().replace('@VERSION@', version).replace('@SHA256@', digest)
(out / 'PKGBUILD').write_text(recipe)
shutil.copy2(ROOT / 'packaging/arch/wetypex-omarchy.install', out)
(out / 'source-manifest.json').write_text(json.dumps({'commit': head, 'project_version': project_version, 'version': version, 'epoch': 1, 'runtime_version': runtime_version, 'source_sha256': digest}, indent=2)+'\n')
print(json.dumps({'build_directory': str(out), 'commit': head, 'version': version}))
if a.build:
    subprocess.run(['makepkg', '--cleanbuild', '--clean'], cwd=out, check=True)
