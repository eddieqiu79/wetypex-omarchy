#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Execute the actual sync loop against deterministic offline peers/clipboard."""
import ast
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'scripts/fcitx5-wetypex-syncd.sh.in'
tree = ast.parse(SOURCE.read_text())
loop = next(i for i, node in enumerate(tree.body) if isinstance(node, ast.While))
prefix = compile(ast.Module(tree.body[:loop], []), str(SOURCE), 'exec')
body = compile(ast.Module(tree.body[loop:], []), str(SOURCE), 'exec')


def clip(value, kind='text'):
    return kind, value, hashlib.sha256(value).hexdigest()


def run_case(*, observations, kind=1, write_ok=True, normalize=0, baseline=False,
             source=2, expired=False, send_ok=True, send_raises=False, iterations=1):
    with tempfile.TemporaryDirectory(prefix='wetypex-sync-regression-') as tmp:
        prior = os.environ.get('WETYPE_STATE_DIR')
        os.environ['WETYPE_STATE_DIR'] = tmp
        namespace = {}
        try:
            exec(prefix, namespace)
        finally:
            if prior is None:
                os.environ.pop('WETYPE_STATE_DIR', None)
            else:
                os.environ['WETYPE_STATE_DIR'] = prior
        current = clip(b'old-local')
        namespace.update(once=True, last_group_id=7, last_clipboard_enabled=True,
                         baseline_pending=baseline, last_remote_version=10,
                         last_local_kind=current[0], last_local_digest=current[2])
        queue = iter(observations)
        namespace['local_clipboard'] = lambda _env: next(queue)
        sent, written, states = [], [], []
        group = dict(ok=True, group_id=7, func_switch=1, self_uin=1,
                     clipboard_from_uin=source, clipboard_version=11,
                     clipboard_type=kind, clipboard='remote-test' if kind == 1 else '',
                     clipboard_binary=base64.b64encode(b'fake-encrypted-reference').decode(),
                     clipboard_expire_at=1 if expired else 0)

        def call(*args):
            if args[0] == 'group-info':
                return group
            sent.append(args[0])
            if args[0] == 'send-image':
                assert Path(args[2]).read_bytes() == b'new-image'
            if send_raises:
                raise OSError('synthetic failure')
            return {'ok': send_ok, 'version': 12}

        def write(kind, value, _env):
            written.append(kind)
            return write_ok

        namespace.update(call=call, set_remote_clipboard=write,
            atomic_json=lambda path, value: states.append((path.name, value)),
            clipboard_command=lambda *args, **kw: subprocess.CompletedProcess([], normalize, b'normalized-image', b''))
        failed = False
        try:
            for _ in range(iterations):
                exec(body, namespace)
        except OSError:
            if not send_raises:
                raise
            failed = True
        assert not list(Path(tmp).glob('clipboard-send*')), 'temporary image retained'
        for name, value in states:
            if name == 'sync-state.json':
                assert set(value) <= {'group_id', 'func_switch', 'hotword_version'}
        result = {'sent': sent, 'written': written, 'cursor': namespace['last_remote_version'],
                  'digest': namespace['last_local_digest'], 'failed': failed}
        namespace['lock'].close()
        return result


def main():
    old, new = clip(b'old-local'), clip(b'new-local')
    checks = {}
    for label, after in [('unreadable_after_query', None), ('file_after_query', None)]:
        value = run_case(observations=[after])
        checks[label] = not value['written'] and not value['sent'] and value['cursor'] == 10
    for label, kind in [('text', 1), ('image', 6)]:
        value = run_case(observations=[old, new], kind=kind)
        checks[label + '_late_local_copy_wins'] = not value['written'] and value['cursor'] == 10
        value = run_case(observations=[old, old], kind=kind, write_ok=False)
        checks[label + '_write_failure_keeps_cursor'] = value['cursor'] == 10
        value = run_case(observations=[old, old], kind=kind)
        checks[label + '_successful_write_advances_cursor'] = value['cursor'] == 11 and len(value['written']) == 1
    value = run_case(observations=[old], kind=6, normalize=75)
    checks['transient_image_error_retries'] = not value['written'] and value['cursor'] == 10
    value = run_case(observations=[old], kind=6, normalize=2)
    checks['invalid_image_is_not_retried_forever'] = not value['written'] and value['cursor'] == 11
    value = run_case(observations=[old], source=1)
    checks['own_message_not_pasted_back'] = not value['written'] and value['cursor'] == 11
    value = run_case(observations=[new], baseline=True, source=1)
    checks['startup_never_uploads_old_clipboard'] = not value['sent']
    value = run_case(observations=[new], send_ok=False)
    checks['failed_upload_keeps_local_digest'] = value['digest'] == old[2]
    value = run_case(observations=[clip(b'new-image', 'image')], send_raises=True)
    checks['temporary_image_removed_on_exception'] = value['failed']
    value = run_case(observations=[old], expired=True)
    checks['expired_remote_not_pasted'] = not value['written'] and value['cursor'] == 11
    remote = clip(b'remote-test')
    value = run_case(observations=[old, old, remote], iterations=2)
    checks['received_text_not_uploaded_next_cycle'] = value['written'] == ['text'] and not value['sent']
    remote = clip(b'normalized-image', 'image')
    value = run_case(observations=[old, old, remote], kind=6, iterations=2)
    checks['received_image_not_uploaded_next_cycle'] = value['written'] == ['image'] and not value['sent']
    report = {'offline': True, 'real_desktop_used': False, 'official_requests': 0,
              'source': str(SOURCE.relative_to(ROOT)), 'checks': checks,
              'passed': all(checks.values())}
    print(json.dumps(report))
    assert report['passed']


if __name__ == '__main__':
    main()
