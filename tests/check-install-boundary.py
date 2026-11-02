#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-only
"""Privileged-boundary tests with synthetic files; never calls sudo/pkexec/pacman."""
import hashlib
import io
import json
import os
from pathlib import Path
import runpy
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
helper = runpy.run_path(str(ROOT/'omarchy/install-package'))
core = runpy.run_path(str(ROOT/'omarchy/wetypex-omarchy-package'))


class BoundaryTests(unittest.TestCase):
    def test_copy_checks_owner_size_and_file_type(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            source = root/'package'; source.write_bytes(b'package')
            with self.assertRaises(ValueError): helper['copy_owned_file'](source, root/'out', os.getuid()+1, 7)
            with self.assertRaises(ValueError): helper['copy_owned_file'](source, root/'out', os.getuid(), 8)
            link = root/'link'; link.symlink_to(source)
            with self.assertRaises(OSError): helper['copy_owned_file'](link, root/'out', os.getuid(), 7)
            fifo = root/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): helper['copy_owned_file'](fifo, root/'out', os.getuid(), 0)
            helper['copy_owned_file'](source, root/'out', os.getuid(), 7)
            source.write_bytes(b'changed')
            self.assertEqual((root/'out').read_bytes(), b'package')

    def test_root_validation_requires_server_digest_and_pinned_repository(self):
        tag = 'v0.1.0-rc.4'
        name = 'wetypex-omarchy-1_0.1.0rc4-1-x86_64.pkg.tar.zst'
        manifest = {'schema':1, 'repository':core['REPO'], 'tag':tag, 'version':tag[1:],
                    'architecture':'x86_64', 'channel':'preview', 'commit':'a'*40,
                    'runtime_version':core['RUNTIME'],
                    'package':{'name':name, 'sha256':'b'*64, 'size':100, 'pacman_version':'1:0.1.0rc4-1'}}
        raw = json.dumps(manifest).encode()
        release = {'assets':[
            {'name':'release-manifest.json', 'digest':'sha256:'+hashlib.sha256(raw).hexdigest(),
             'browser_download_url':f'https://github.com/{core["REPO"]}/releases/download/{tag}/release-manifest.json'},
            {'name':name, 'digest':'sha256:'+'b'*64, 'size':100}]}
        class Opener:
            def open(self, request, timeout): return io.BytesIO(raw)
        fake = dict(core, get_release=lambda *args: release)
        with patch.object(helper['urllib'].request, 'build_opener', return_value=Opener()):
            self.assertEqual(helper['validated_manifest'](fake, tag, 'preview')[0], manifest)
            release['assets'][1]['digest'] = 'sha256:'+'c'*64
            with self.assertRaises(ValueError): helper['validated_manifest'](fake, tag, 'preview')
            release['assets'][0]['digest'] = None
            with self.assertRaises(ValueError): helper['validated_manifest'](fake, tag, 'preview')

    def test_auth_policy_is_specific_and_not_retained(self):
        import xml.etree.ElementTree as ET
        action = ET.parse(ROOT/'packaging/polkit/org.wetypex.omarchy.install.policy').getroot().find('action')
        self.assertEqual(action.find('defaults/allow_active').text, 'auth_admin')
        self.assertEqual(action.find('defaults/allow_any').text, 'no')
        annotations = {n.attrib['key']:n.text for n in action.findall('annotate')}
        self.assertEqual(annotations['org.freedesktop.policykit.exec.path'], core['INSTALLER'])
        self.assertNotIn('org.freedesktop.policykit.exec.allow_gui', annotations)


if __name__ == '__main__': unittest.main()
