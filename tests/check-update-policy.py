#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard against accidentally restoring upstream downloads in the fork updater."""

import contextlib
import io
import json
import os
from pathlib import Path
import runpy
import socket
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import urllib.request


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "scripts/fcitx5-wetypex-update.sh.in"


class UpdatePolicyTest(unittest.TestCase):
    def invoke(self, arguments):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            environment = {
                "HOME": directory,
                "XDG_CONFIG_HOME": str(root / "config"),
                "XDG_CACHE_HOME": str(root / "cache"),
                "WETYPE_UPDATE_API": "https://example.invalid/upstream",
                "WETYPE_UPDATE_DIR": str(root / "updates"),
            }
            output = io.StringIO()
            with patch.dict(os.environ, environment), \
                    patch.object(urllib.request, "urlopen", side_effect=AssertionError("network call")), \
                    patch.object(socket, "socket", side_effect=AssertionError("network socket")), \
                    patch.object(subprocess, "Popen", side_effect=AssertionError("external process")), \
                    contextlib.redirect_stdout(output):
                updater = runpy.run_path(str(SOURCE), run_name="updater_test")
                self.assertEqual(updater["main"](arguments), 0)
            self.assertEqual(list(root.iterdir()), [], "Updater wrote local state")
            return output.getvalue()

    def test_automatic_invocation_is_silent_and_inert(self):
        self.assertEqual(self.invoke([]), "")

    def test_force_cannot_resume_upstream_downloads(self):
        output = self.invoke(["--force"])
        self.assertIn("No release check", output)
        self.assertIn("eddieqiu79/wetypex-omarchy", output)

    def test_status_is_policy_not_a_release_check(self):
        status = json.loads(self.invoke(["--status"]))
        self.assertFalse(status["automatic_updates"])
        self.assertFalse(status["release_check_performed"])
        self.assertEqual(status["update_mode"], "manual-arch-package")
        self.assertEqual(status["repository"], "https://github.com/eddieqiu79/wetypex-omarchy")


if __name__ == "__main__":
    unittest.main()
