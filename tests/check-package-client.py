#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import hashlib
import contextlib
import copy
import urllib.request
import io
import json
import os
import subprocess
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
            self.assertEqual(sum(a[1:3]==['release','verify-asset'] for a in calls), 4) # includes capability probe and final cache validation
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


    def test_repeat_download_revalidates_without_downloading(self):
        with tempfile.TemporaryDirectory() as d:
            run, calls = self.operation()
            with patch.dict(m['download'].__globals__, {'run':run}):
                m['download'](self.fake_policy(), TAG, 'preview', Path(d))
                calls.clear()
                m['download'](self.fake_policy(), TAG, 'preview', Path(d))
            self.assertFalse(any(a[1:3] == ['release', 'download'] for a in calls))
            self.assertTrue(any(a[1:3] == ['release', 'verify-asset'] for a in calls))

    def test_interrupted_asset_download_resumes_from_verified_manifest(self):
        with tempfile.TemporaryDirectory() as d:
            run, calls = self.operation()
            def interrupted(args, **kwargs):
                if args[1:3] == ['release', 'download'] and NAME in args:
                    raise OSError('synthetic interruption')
                return run(args, **kwargs)
            with patch.dict(m['download'].__globals__, {'run':interrupted}), self.assertRaises(OSError):
                m['download'](self.fake_policy(), TAG, 'preview', Path(d))
            self.assertTrue((Path(d)/TAG/'release-manifest.json').exists())
            calls.clear()
            with patch.dict(m['download'].__globals__, {'run':run}):
                package, _ = m['download'](self.fake_policy(), TAG, 'preview', Path(d))
            self.assertEqual(package.read_bytes(), DATA)
            downloads = [a for a in calls if a[1:3] == ['release', 'download']]
            self.assertEqual(len(downloads), 1)
            self.assertIn(NAME, downloads[0])

    def test_tampered_cache_never_installed_or_overwritten(self):
        with tempfile.TemporaryDirectory() as d:
            run, calls = self.operation()
            with patch.dict(m['download'].__globals__, {'run':run}):
                file, _ = m['download'](self.fake_policy(), TAG, 'preview', Path(d))
                file.write_bytes(b'corrupt')
                with self.assertRaises(m['OperationError']):
                    m['download'](self.fake_policy(), TAG, 'preview', Path(d))
            self.assertEqual(file.read_bytes(), b'corrupt')

    def test_all_dependencies_preserved_and_checked(self):
        info = m['package_info']('depend = qt6-base>=6.12\ndepend = python\n')
        self.assertEqual(info['depend'], ['qt6-base>=6.12', 'python'])
        result = subprocess.CompletedProcess([], 127, 'qt6-base>=6.12\n', '')
        with patch.object(m['subprocess'], 'run', return_value=result) as command:
            with self.assertRaises(m['OperationError']) as error:
                m['dependencies_satisfied'](info['depend'])
            self.assertEqual(error.exception.code, 'incompatible-system')
            self.assertEqual(command.call_args.args[0], ['pacman', '-T', '--', *info['depend']])
        for bad in (['--root=/tmp'], ['a;rm'], 'python'):
            with self.assertRaises(m['OperationError']): m['dependencies_satisfied'](bad)

    def test_lock_excludes_other_operation_and_releases(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)
            with m['operation_lock'](path):
                with self.assertRaises(m['OperationError']) as error:
                    with m['operation_lock'](path): self.fail('lock acquired twice')
                self.assertEqual(error.exception.code, 'busy')
            with m['operation_lock'](path): pass

    def test_symlink_package_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            target = Path(d)/'real'; target.write_bytes(DATA)
            link = Path(d)/NAME; link.symlink_to(target)
            with self.assertRaises(m['OperationError']): m['verify_package'](link, MANIFEST)

    def test_dependency_mismatch_between_manifest_and_package(self):
        with tempfile.TemporaryDirectory() as d:
            file = Path(d)/NAME; file.write_bytes(DATA)
            manifest = dict(MANIFEST, compatibility={'dependencies':['python', 'qt6-base']})
            run, _ = self.operation()
            with patch.dict(m['verify_package'].__globals__, {'run':run}), self.assertRaises(m['OperationError']):
                m['verify_package'](file, manifest)

    def test_authorization_cancel_then_resume_pins_original_target(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d); helper = root/'helper'; helper.write_text('synthetic')
            cache = root/'cache'; cache.mkdir(); package = cache/NAME; package.write_bytes(DATA)
            prepared = []
            def download(policy, tag, channel, directory, emit):
                prepared.append((tag, channel, directory))
                return package, MANIFEST
            replacements = {'policy':self.fake_policy, 'download':download,
                            'check_install':lambda *args: None, 'INSTALLER':str(helper),
                            'state_directory':lambda: root/'state',
                            'installed_version':lambda: MANIFEST['package']['pacman_version']}
            with patch.dict(m['main'].__globals__, replacements), patch.object(m['shutil'], 'which', return_value='/usr/bin/synthetic'), contextlib.redirect_stdout(io.StringIO()) as output:
                with patch.object(m['subprocess'], 'run', return_value=subprocess.CompletedProcess([], 126)):
                    self.assertEqual(m['main'](['install', '--confirm', '--json', '--tag', TAG, '--channel', 'preview', '--directory', str(cache)]), 1)
                failed = json.loads((root/'state/operation.json').read_text())
                self.assertEqual(failed['error_code'], 'authorization-cancelled')
                self.assertEqual(package.read_bytes(), DATA)
                self.assertEqual(m['main'](['resume', '--confirm', '--json', '--tag', 'v9.0.0']), 1)
                self.assertEqual(json.loads((root/'state/operation.json').read_text()), failed)
                with patch.object(m['subprocess'], 'run', return_value=subprocess.CompletedProcess([], 0)):
                    self.assertEqual(m['main'](['resume', '--confirm', '--json']), 0)
            self.assertEqual(prepared[0], prepared[1])
            final = json.loads((root/'state/operation.json').read_text())
            self.assertEqual(final['stage'], 'installed')
            self.assertEqual(final['activation'], 'pending')
            self.assertNotIn('error_code', final)

    def test_anonymous_bundle_binds_tag_commit_and_workflow(self):
        release = copy.deepcopy(RELEASE)
        raw = json.dumps(MANIFEST).encode()
        content = {'release-manifest.json':raw, NAME:DATA, m['ReleaseVerifier'].BUNDLE:b'synthetic proof'}
        release['assets'] = [{'name':name, 'size':len(data), 'browser_download_url':f'https://github.com/{m["REPO"]}/releases/download/{TAG}/{name}'} for name,data in content.items()]
        commands, fetched = [], []
        class Opener:
            def open(self, request, timeout):
                name = request.full_url.rsplit('/',1)[-1]; fetched.append(name)
                return io.BytesIO(content[name])
        class VerifierProcess:
            returncode = 0
            def __init__(self, args, **kwargs): commands.append((args, kwargs['env']))
            def __enter__(self): return self
            def __exit__(self, *args): pass
            def communicate(self, timeout): return b'', b''
        run, legacy = self.operation()
        with tempfile.TemporaryDirectory() as d, patch.dict(os.environ, {'GH_TOKEN':'synthetic-do-not-forward'}), patch.object(urllib.request, 'build_opener', return_value=Opener()), patch.object(m['subprocess'], 'Popen', VerifierProcess), patch.dict(m['download'].__globals__, {'run':run}):
            package, _ = m['download'](self.fake_policy(release), TAG, 'preview', Path(d))
            self.assertEqual(package.read_bytes(), DATA)
        self.assertFalse(any(a[1:3] == ['release', 'verify'] or a[1:3] == ['release', 'download'] for a in legacy))
        self.assertGreaterEqual(len(commands), 2)
        for command, env in commands:
            self.assertIn('refs/tags/'+TAG, command)
            self.assertIn(MANIFEST['commit'], command)
            self.assertIn(f'https://github.com/{m["REPO"]}/.github/workflows/prepare-release.yml@refs/tags/{TAG}', command)
            self.assertIn('--deny-self-hosted-runners', command)
            self.assertNotIn('GH_TOKEN', env)
        VerifierProcess.returncode = 1
        fetched.clear()
        with tempfile.TemporaryDirectory() as d, patch.object(urllib.request, 'build_opener', return_value=Opener()), patch.object(m['subprocess'], 'Popen', VerifierProcess), patch.dict(m['download'].__globals__, {'run':run}):
            with self.assertRaises(m['OperationError']) as error:
                m['download'](self.fake_policy(release), TAG, 'preview', Path(d))
            self.assertEqual(error.exception.code, 'attestation-failed')
        self.assertNotIn(NAME, fetched)

if __name__ == '__main__': unittest.main()
