#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build/test/preview the working tree without installing or touching live state."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('build', 'test', 'preview'))
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--kqueue-source', type=Path, help='Optional existing pinned dependency source for offline builds')
    args = parser.parse_args()
    build = args.build_dir.expanduser().resolve()
    if build == ROOT or ROOT in build.parents:
        parser.error('Choose a build directory outside the source checkout')
    binding = build/'wetypex-development.json'
    if build.exists() and not binding.is_file():
        parser.error('Existing directory is not owned by this development tool')
    if binding.exists() and json.loads(binding.read_text()).get('source') != str(ROOT):
        parser.error('Build directory belongs to another checkout')
    if args.action == 'build':
        build.mkdir(parents=True, exist_ok=True)
        binding.write_text(json.dumps({'source': str(ROOT), 'role': 'development'}))
        command = ['cmake', '-S', str(ROOT), '-B', str(build), '-DCMAKE_BUILD_TYPE=Debug',
                   '-DCMAKE_INSTALL_PREFIX='+str(build/'private-prefix'), '-DCMAKE_INSTALL_LIBDIR=lib',
                   '-DFCITX_INSTALL_USE_FCITX_SYS_PATHS=OFF', '-DWETYPE_QT_VERSION=6',
                   '-DWETYPE_LAYER_SHELL=OFF', '-DWETYPE_BUNDLED_CURL=OFF',
                   '-DWETYPE_DEVELOPMENT_BUILD=ON', '-DPython3_EXECUTABLE=/usr/bin/python3']
        if args.kqueue_source:
            command.append('-DFETCHCONTENT_SOURCE_DIR_KQUEUE='+str(args.kqueue_source.resolve()))
        subprocess.run(command, check=True)
        subprocess.run(['cmake', '--build', str(build), '--parallel', '2'], check=True)
    elif not binding.exists():
        parser.error('Build this checkout first')
    elif args.action == 'test':
        subprocess.run(['ctest', '--test-dir', str(build), '--output-on-failure'], check=True)
    else:
        with tempfile.TemporaryDirectory(prefix='wetypex-dev-preview-') as d:
            root = Path(d)
            (root/'runtime').mkdir(mode=0o700)
            env = dict(os.environ, XDG_CONFIG_HOME=str(root/'config'), XDG_DATA_HOME=str(root/'data'),
                       XDG_STATE_HOME=str(root/'state'), XDG_CACHE_HOME=str(root/'cache'),
                       XDG_RUNTIME_DIR=str(root/'runtime'), XDG_DATA_DIRS='/nonexistent-wetypex-preview',
                       DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-wetypex-preview-bus',
                       WETYPE_BACKEND_LAUNCHER='/nonexistent-wetypex-preview-backend',
                       WETYPE_ACCOUNT_TOOL='/nonexistent-wetypex-preview-account')
            # Retain the actual Wayland socket path, without sharing app state.
            display = os.environ.get('WAYLAND_DISPLAY', '')
            if display and not display.startswith('/'):
                env['WAYLAND_DISPLAY'] = str(Path(os.environ.get('XDG_RUNTIME_DIR', '/nonexistent'))/display)
            subprocess.run([str(build/'fcitx5-wetypex-settings')], env=env, check=True)


if __name__ == '__main__':
    main()
