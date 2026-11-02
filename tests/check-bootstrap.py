#!/usr/bin/python3
# SPDX-License-Identifier: GPL-3.0-only
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
builder = runpy.run_path(str(ROOT/'tools/build-installer.py'))


class BootstrapTests(unittest.TestCase):
    def test_reproducible_allowlisted_artifact_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as d:
            first, second = Path(d)/'first.pyz', Path(d)/'second.pyz'
            builder['build'](first); builder['build'](second)
            self.assertEqual(hashlib.sha256(first.read_bytes()).digest(), hashlib.sha256(second.read_bytes()).digest())
            with zipfile.ZipFile(first) as z:
                self.assertEqual(set(z.namelist()), set(builder['FILES']))
            with self.assertRaises(FileExistsError): builder['build'](first)
            env = dict(os.environ, HOME=d, XDG_CONFIG_HOME=d+'/config', XDG_DATA_HOME=d+'/data', XDG_STATE_HOME=d+'/state')
            result = subprocess.run([sys.executable, str(first), '--help'], env=env, capture_output=True)
            self.assertEqual(result.returncode, 0)
            self.assertFalse((Path(d)/'state').exists())


if __name__ == '__main__': unittest.main()
