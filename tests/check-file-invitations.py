#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the account wrapper with synthetic service responses, no identity/network."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='wetypex-invitation-test-') as tmp:
    root = Path(tmp)
    for name in ('support', 'runtime', 'bin'):
        (root/name).mkdir()
    (root/'runtime/manifest.txt').touch()
    host = root/'support/wetypex-engine-host'
    host.write_text('#!/bin/sh\nexit 0\n')
    host.chmod(0o700)
    # bwrap is replaced only inside this isolated test's PATH.
    wrapper = root/'bin/bwrap'
    wrapper.write_text('#!/usr/bin/env python3\nimport os,sys,json\n'
                       'open(os.environ["TEST_ARGS"], "w").write(json.dumps(sys.argv[1:]))\n'
                       'print(os.environ["TEST_RESPONSE"])\n')
    wrapper.chmod(0o700)
    env = dict(os.environ, PATH=str(root/'bin')+':'+os.environ['PATH'],
               WETYPE_SUPPORT_DIR=str(root/'support'), WETYPE_RUNTIME_DIR=str(root/'runtime'),
               WETYPE_ACCOUNT_STATE_DIR=str(root/'synthetic-account'), TEST_ARGS=str(root/'args'))
    data = dict(pairing_error=0, response_error_code=0, uin=1, devices=[{'uin':1},{'uin':42}],
                clipboard='synthetic-clipboard-must-not-be-returned')
    for sender, code, accepted in [(42,'synthetic-code',True), (1,'self',False),
                                    (43,'stranger',False), (0,'zero',False), (42,'',False)]:
        data['pending_invite'] = dict(sender_uin=sender, transfer_code=code, device_name='Synthetic Mac')
        env['TEST_RESPONSE'] = json.dumps(data)
        result = subprocess.run(['sh', str(ROOT/'scripts/fcitx5-wetypex-account.sh.in'),
                                 'pending-file-invite'], env=env, text=True, capture_output=True, check=True)
        value = json.loads(result.stdout)
        assert value['ok'] and bool(value['pending_invite']) == accepted
        assert 'clipboard' not in result.stdout
        assert value['self_uin'] == 1 and len(value['devices']) == 2
        assert 'WETYPE_PENDING_FILE_INVITE' in json.loads((root/'args').read_text())
    subprocess.run(['sh', str(ROOT/'scripts/fcitx5-wetypex-account.sh.in'), 'group-info'],
                   env=env, capture_output=True, check=True)
    assert 'WETYPE_PENDING_FILE_INVITE' not in json.loads((root/'args').read_text())
print('File invitation wrapper: paired sender filtering, scoped query, no clipboard payload: passed')
