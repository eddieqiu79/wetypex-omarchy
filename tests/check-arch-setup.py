#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise deactivation against a fake service manager and temporary files."""
import importlib.machinery
import importlib.util
import json
from pathlib import Path
import tempfile
from unittest.mock import patch
root = Path(__file__).resolve().parents[1]
loader = importlib.machinery.SourceFileLoader('setup', str(root/'omarchy/wetypex-omarchy-setup'))
spec = importlib.util.spec_from_loader(loader.name, loader)
m = importlib.util.module_from_spec(spec)
loader.exec_module(m)
with tempfile.TemporaryDirectory() as tmp:
    config = Path(tmp)/'config'
    record = Path(tmp)/'activation.json'
    state = {'fcitx_service': 'test-fcitx.service'}
    drop = config/'systemd/user/test-fcitx.service.d/90-wetypex-omarchy-sync.conf'
    drop.parent.mkdir(parents=True)
    record.write_text(json.dumps(state))
    drop.write_text('user-modified configuration')
    with patch.object(m, 'ctl') as ctl:
        try:
            m.restore(config, record, state)
            raise AssertionError('Modified user configuration overwritten')
        except RuntimeError:
            pass
        ctl.assert_not_called()
    assert record.exists() and drop.read_text() == 'user-modified configuration'
    drop.write_text(m.DROP_CONTENT)
    with patch.object(m, 'ctl') as ctl:
        m.restore(config, record, state)
        assert [c.args for c in ctl.call_args_list] == [
            ('disable','--now','wetypex-sync.service'),('daemon-reload',),('restart','test-fcitx.service')]
    assert not drop.exists() and not record.exists()
print('Setup deactivation ordering and modified-file protection passed; no real services touched.')
