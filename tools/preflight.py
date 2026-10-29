#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only host check; never reads clipboard, account state, or private config."""
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def command(args):
    try:
        result = subprocess.run(args, text=True, capture_output=True, timeout=5, check=False)
        return {'ok': result.returncode == 0, 'value': result.stdout.strip().splitlines()[:1]}
    except (OSError, subprocess.TimeoutExpired):
        return {'ok': False, 'value': []}

names = ('cmake', 'cc', 'c++', 'clang++', 'pkg-config', 'fcitx5',
         'fcitx5-remote', 'wl-copy', 'wl-paste', 'bwrap', 'systemctl', 'git')
result = {
    'kind': 'read-only installation preflight',
    'architecture': platform.machine(),
    'kernel': platform.release(),
    'python': platform.python_version(),
    'session': {name: bool(os.environ.get(name)) for name in
                ('WAYLAND_DISPLAY', 'DISPLAY', 'XDG_RUNTIME_DIR', 'DBUS_SESSION_BUS_ADDRESS')},
    'commands_available': {name: bool(shutil.which(name)) for name in names},
    'commit': command(['git', '-C', str(ROOT), 'rev-parse', 'HEAD']),
    'versions': {name: command([name, '--version']) for name in ('cmake', 'clang++', 'fcitx5')},
    'pkg_config_available': {name: command(['pkg-config', '--exists', name])['ok']
                             for name in ('json-c', 'fontconfig', 'wayland-client', 'libcurl')},
    'python_modules': {},
    'notes': ['This check does not modify the system, inspect clipboard contents, or validate device connectivity.',
              'Missing or present tools do not establish that the complete build or installation will work.'],
}
for name in ('cryptography', 'gi'):
    try:
        __import__(name)
        result['python_modules'][name] = True
    except ImportError:
        result['python_modules'][name] = False
print(json.dumps(result, indent=2))
if __name__ == '__main__':
    sys.exit(0)
