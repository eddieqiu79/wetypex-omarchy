#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import hashlib
import json
from pathlib import Path
import runpy
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
m = runpy.run_path(str(ROOT/'omarchy/wetypex-omarchy-package'))
p = m['policy']()
TAG = 'v0.1.0-rc.1'
NAME = 'wetypex-omarchy-0.1.0rc1-1-x86_64.pkg.tar.zst'
DATA = b'synthetic package'
MANIFEST = {'schema':1, 'repository':m['REPO'], 'tag':TAG, 'version':'0.1.0-rc.1', 'architecture':'x86_64', 'channel':'preview', 'commit':'a'*40, 'runtime_version':'2.2.3.657', 'package':{'name':NAME, 'size':len(DATA), 'sha256':hashlib.sha256(DATA).hexdigest(), 'pacman_version':'1:0.1.0rc1-1'}}
RELEASE = {'tag_name':TAG, 'immutable':True, 'draft':False, 'prerelease':True, 'assets':[{'name':NAME, 'size':len(DATA)}]}

class PackageTests(unittest.TestCase):
    def fake_policy(self, release=RELEASE):
        return dict(p, fetch_json=lambda url: release)
    def test_preview_requires_opt_in(self):
        with self.assertRaises(ValueError): m['get_release'](self.fake_policy(), TAG, 'stable')
        self.assertEqual(m['get_release'](self.fake_policy(), TAG, 'preview'), RELEASE)
    def test_release_identity_and_immutability(self):
        for overrides in ({'tag_name':'v1.0.0'}, {'immutable':False}, {'draft':True}):
            with self.assertRaises(ValueError): m['get_release'](self.fake_policy(dict(RELEASE, **overrides)), TAG, 'preview')
        with self.assertRaises(ValueError): m['get_release'](self.fake_policy(), '../../x', 'preview')
    def operation(self, fail_verify=False):
        calls = []
        def run(args, **kwargs):
            calls.append(args)
            if args[0] == 'bsdtar': return b'pkgname = wetypex-omarchy\npkgver = 1:0.1.0rc1-1\narch = x86_64\n'
            if args[1:3] == ['release', 'download']:
                directory = Path(args[args.index('--dir')+1]); name = args[args.index('--pattern')+1]
                (directory/name).write_bytes(json.dumps(MANIFEST).encode() if name=='release-manifest.json' else DATA)
            if args[1:3] == ['release', 'verify-asset'] and fail_verify: raise ValueError('synthetic verification failure')
            return b''
        return run, calls
    def test_verified_download_atomic_and_no_privilege(self):
        with tempfile.TemporaryDirectory() as d:
            run, calls = self.operation()
            with patch.dict(m['download'].__globals__, {'run':run}):
                package, manifest = m['download'](self.fake_policy(), TAG, 'preview', Path(d))
            self.assertEqual(package.read_bytes(), DATA)
            self.assertEqual(manifest, MANIFEST)
            self.assertEqual(len(list(Path(d).iterdir())), 1)
            self.assertEqual(sum(a[1:3]==['release','verify-asset'] for a in calls), 3) # includes capability probe
            self.assertFalse(any(a[0] in ('sudo','pacman','systemctl') for a in calls))
    def test_failed_verification_cleans_partial_files(self):
        with tempfile.TemporaryDirectory() as d:
            run, calls = self.operation(True)
            with patch.dict(m['download'].__globals__, {'run':run}), self.assertRaises(ValueError):
                m['download'](self.fake_policy(), TAG, 'preview', Path(d))
            self.assertEqual(list(Path(d).iterdir()), [])
            self.assertFalse(any(NAME in a for a in calls))
    def test_tampered_or_wrong_arch_package_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            file = Path(d)/NAME; file.write_bytes(DATA+b'!')
            with self.assertRaises(ValueError): m['verify_package'](file, MANIFEST)
            file.write_bytes(DATA)
            with patch.dict(m['verify_package'].__globals__, {'run':lambda *a, **k: b'pkgname = wetypex-omarchy\npkgver = 1:0.1.0rc1-1\narch = aarch64\n'}), self.assertRaises(ValueError):
                m['verify_package'](file, MANIFEST)
    def test_install_requires_confirmation_before_network(self):
        with patch.object(m['os'], 'geteuid', return_value=1000), patch.dict(m['main'].__globals__, {'policy':lambda: self.fail('must not query release')}), self.assertRaises(SystemExit):
            m['main'](['install'])
    def test_existing_download_not_overwritten(self):
        with tempfile.TemporaryDirectory() as d:
            destination = Path(d)/TAG; destination.mkdir(); (destination/'keep').write_text('retained')
            run, _ = self.operation()
            with patch.dict(m['download'].__globals__, {'run':run}), self.assertRaises(ValueError):
                m['download'](self.fake_policy(), TAG, 'preview', Path(d))
            self.assertEqual((destination/'keep').read_text(), 'retained')

if __name__ == '__main__': unittest.main()
