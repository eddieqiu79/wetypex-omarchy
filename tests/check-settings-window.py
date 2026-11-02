#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import os
from pathlib import Path
import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory(prefix='wetypex-gui-test-') as d:
    root = Path(d)
    runtime = root/'runtime'; runtime.mkdir(mode=0o700)
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='',
               XDG_CONFIG_HOME=str(root/'config'), XDG_DATA_HOME=str(root/'data'),
               XDG_CACHE_HOME=str(root/'cache'), XDG_STATE_HOME=str(root/'state'), XDG_RUNTIME_DIR=str(runtime),
               WETYPE_TEST_PACKAGE_TOOL=str(Path(__file__).resolve().parents[1]/'omarchy/wetypex-omarchy-package'),
               XDG_DATA_DIRS='/nonexistent-wetypex-data',
               DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-wetypex-test-bus',
               WETYPE_BACKEND_LAUNCHER='/nonexistent-synthetic-engine', WETYPE_ACCOUNT_TOOL='/nonexistent-synthetic-account')
    for scale in ('1', '1.25', '1.5', '2'):
        env['QT_SCALE_FACTOR'] = scale
        if os.environ.get('WETYPE_TEST_GUI_SCREENSHOT'):
            env['WETYPE_TEST_GUI_SCREENSHOT'] = os.environ['WETYPE_TEST_GUI_SCREENSHOT'] + '.scale-' + scale + '.png'
        result = subprocess.run([sys.argv[1]], env=env, timeout=15, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(f'Settings window failed at scale {scale}: {result.returncode}\n{result.stderr}')
    assert not (root/'config/fcitx5/wetypex.json').exists(), 'Opening settings changed preferences'
    assert not (root/'data/fcitx5-wetypex').exists(), 'Opening settings created backend state'
print('Actual settings window: 5 widths, 2 heights at 100/125/150/200% scale, navigation, no startup engine/account/config writes')
