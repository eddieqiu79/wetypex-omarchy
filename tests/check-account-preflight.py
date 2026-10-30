#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Missing native dependencies must fail before accessing account state."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    support = root/'support'
    support.mkdir()
    host = support/'wetypex-engine-host'
    host.write_text('#!/bin/sh\nexit 2\n')
    host.chmod(0o700)
    state = root/'must-not-be-created'
    env = dict(os.environ, WETYPE_SUPPORT_DIR=str(support), WETYPE_ACCOUNT_STATE_DIR=str(state))
    result = subprocess.run(['sh', str(ROOT/'scripts/fcitx5-wetypex-account.sh.in'), 'group-info'], env=env, capture_output=True, text=True, timeout=15)
    assert result.returncode == 78
    assert json.loads(result.stdout)['failure_stage'] == 'runtime'
    assert not state.exists()
    assert not result.stderr
print('Missing runtime dependency is classified before identity/network operations')
