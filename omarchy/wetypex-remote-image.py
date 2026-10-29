#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 wetypex-omarchy contributors
"""Resolve official encrypted image references in memory; never log URLs or keys."""
import hashlib
import http.client
from pathlib import Path
import subprocess
import sys
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
COMPAT = ROOT / 'share/fcitx5-wetypex/compat/cloud'
if not COMPAT.is_dir(): COMPAT = ROOT / 'compat/cloud'
sys.path.insert(0, str(COMPAT))
from 剪贴板协议 import image_reference, image_cipher, cos_url
from 图片格式 import sniff_image

LIMIT = 8 * 1024 * 1024


class OfficialRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        cos_url(newurl)
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def resolve(data, opener=None):
    if not data or len(data) > LIMIT:
        raise ValueError('invalid size')
    if sniff_image(data):
        return data
    if len(data) > 4096:
        raise ValueError('invalid reference size')
    reference = image_reference(data)
    if reference is None:
        raise ValueError('invalid reference')
    if opener is None:
        opener = urllib.request.build_opener(OfficialRedirect()).open
    with opener(urllib.request.Request(reference['url']), timeout=25) as response:
        if response.status != 200:
            raise ValueError('invalid response')
        encrypted = response.read(LIMIT + 1)
        declared = getattr(response, 'headers', {}).get('Content-Length')
        if declared is not None:
            expected = int(declared)
            if expected < 1 or expected > LIMIT:
                raise ValueError('invalid size')
            if len(encrypted) != expected:
                raise http.client.IncompleteRead(b'', expected - len(encrypted))
    if not encrypted or len(encrypted) > LIMIT:
        raise ValueError('invalid size')
    image = image_cipher(reference['key'], encrypted)
    if hashlib.md5(image).hexdigest() != reference['md5'] or not sniff_image(image):
        raise ValueError('invalid image integrity')
    return image


def main():
    try:
        data = resolve(sys.stdin.buffer.read(LIMIT + 1))
        result = subprocess.run(
            [str((ROOT / 'omarchy/wetypex-image-helper.py' if (ROOT / 'omarchy/wetypex-image-helper.py').is_file() else ROOT / 'bin/wetypex-image-helper')), '--normalize-image'],
            input=data, capture_output=True, timeout=15)
        if result.returncode or not result.stdout or len(result.stdout) > LIMIT:
            return 2
        sys.stdout.buffer.write(result.stdout)
        return 0
    except (urllib.error.URLError, http.client.IncompleteRead, TimeoutError, OSError, subprocess.TimeoutExpired):
        return 75
    except (ValueError, TypeError):
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
