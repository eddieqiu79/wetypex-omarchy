#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline encrypted-image integrity, interrupted download and format checks."""
import base64
import hashlib
import http.client
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('resolver', ROOT / 'omarchy/wetypex-remote-image.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
png = (ROOT / 'fixtures/formats/synthetic.png').read_bytes()
key = b'0123456789abcdef'  # synthetic test key, never an account credential
encrypted = module.image_cipher(key, png)
reference = dict(version=1, key=base64.b64encode(key).decode(),
                 md5=hashlib.md5(png).hexdigest(),
                 path='https://synthetic-123.cos.ap-guangzhou.myqcloud.com/test')


class Response(io.BytesIO):
    status = 200

    def __init__(self, data, declared):
        super().__init__(data)
        self.headers = {'Content-Length': str(declared)}


checks = {}
encoded = json.dumps(reference).encode()
checks['raw_png'] = module.resolve(png) == png
checks['encrypted_integrity'] = module.resolve(encoded, lambda *a, **k: Response(encrypted, len(encrypted))) == png
try:
    module.resolve(encoded, lambda *a, **k: Response(encrypted[:-10], len(encrypted)))
except http.client.IncompleteRead:
    checks['truncated_download_is_transient'] = True
with patch.object(module, 'resolve', side_effect=http.client.IncompleteRead(b'', 10)), \
     patch.object(sys, 'stdin', type('Input', (), {'buffer': io.BytesIO(encoded)})()):
    checks['truncation_exit_code_retryable'] = module.main() == 75
for name, change in [('bad_md5', {'md5': '0' * 32}),
                     ('bad_origin', {'path': 'https://example.org/test'}),
                     ('bad_key', {'key': 'invalid'})]:
    rejected = False
    try:
        module.resolve(json.dumps({**reference, **change}).encode(),
                       lambda *a, **k: Response(encrypted, len(encrypted)))
    except ValueError:
        rejected = True
    checks[name + '_rejected'] = rejected
formats = []
for item in json.loads((ROOT / 'fixtures/formats/manifest.json').read_text()):
    data = (ROOT / 'fixtures/formats' / item['file']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == item['sha256']
    result = subprocess.run([sys.executable, str(ROOT / 'omarchy/wetypex-image-helper.py'), '--normalize-image'],
                            input=data, capture_output=True, timeout=20)
    formats.append({'fixture': item['file'], 'passed': result.returncode == 0
                    and module.sniff_image(result.stdout) == 'image/png'
                    and len(result.stdout) <= module.LIMIT})
report = {'offline': True, 'real_clipboard_used': False, 'checks': checks,
          'formats': formats, 'passed': all(checks.values()) and all(row['passed'] for row in formats)}
print(json.dumps(report))
assert report['passed']
