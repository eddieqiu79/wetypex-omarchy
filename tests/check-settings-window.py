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
               XDG_CACHE_HOME=str(root/'cache'), XDG_RUNTIME_DIR=str(runtime),
               XDG_DATA_DIRS='/nonexistent-wetypex-data',
               DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-wetypex-test-bus',
               WETYPE_BACKEND_LAUNCHER='/nonexistent-synthetic-engine', WETYPE_ACCOUNT_TOOL='/nonexistent-synthetic-account')
    subprocess.run([sys.argv[1]], env=env, check=True, timeout=15, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    assert not (root/'config/fcitx5/wetypex.json').exists(), 'Opening settings changed preferences'
    assert not (root/'data/fcitx5-wetypex').exists(), 'Opening settings created backend state'
print('Actual settings window: native layout, navigation, no startup engine/account/config writes')
