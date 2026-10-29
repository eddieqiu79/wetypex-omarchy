#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import contextlib
import copy
import io
import json
from pathlib import Path
import runpy
import tempfile
import unittest
import urllib.request
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
update = runpy.run_path(str(ROOT / "scripts/fcitx5-wetypex-update.sh.in"))
prepare = runpy.run_path(str(ROOT / "tools/prepare-release.py"))


def fixture(version="0.2.0", immutable=True):
    tag = "v" + version
    package = {"name": "wetypex-omarchy-1_" + version.replace("-rc.", "rc") + "-1-x86_64.pkg.tar.zst",
               "sha256": "a" * 64, "size": 42, "pacman_version": "1:" + version.replace("-rc.", "rc") + "-1"}
    manifest = {"schema": 1, "repository": update["REPOSITORY"], "tag": tag,
                "version": version, "commit": "b" * 40, "architecture": "x86_64",
                "channel": "preview" if "-rc." in version else "stable",
                "runtime_version": "2.2.3.657", "package": package}
    base = update["PROJECT_REPOSITORY"] + "/releases/download/" + tag + "/"
    release = {"tag_name": tag, "draft": False, "prerelease": "-rc." in version,
               "immutable": immutable, "assets": [
                   {"name": "release-manifest.json", "browser_download_url": base + "release-manifest.json"},
                   {"name": package["name"], "size": 42, "browser_download_url": base + package["name"]}]}
    return release, manifest


class ChannelTests(unittest.TestCase):
    def check(self, releases, manifests=None, channel="stable", current="0.1.0", runtime="2.2.3.657"):
        def fetch(url):
            if url.startswith("https://api.github.com/repos/eddieqiu79/wetypex-omarchy/releases?"):
                return releases
            for tag, manifest in (manifests or {}).items():
                if "/" + tag + "/release-manifest.json" in url:
                    return manifest
            self.fail("unexpected URL: " + url)
        with patch.dict(update["check_releases"].__globals__, {"fetch_json": fetch}):
            return update["check_releases"](channel, current, runtime, "x86_64")

    def test_empty_not_current(self):
        self.assertEqual(self.check([])["status"], "no-release")

    def test_channels_and_versions(self):
        stable, sm = fixture()
        rc, rm = fixture("0.3.0-rc.1")
        manifests = {sm["tag"]: sm, rm["tag"]: rm}
        self.assertEqual(self.check([rc, stable], manifests)["tag"], "v0.2.0")
        self.assertEqual(self.check([stable, rc], manifests, channel="preview")["tag"], "v0.3.0-rc.1")
        self.assertEqual(self.check([stable], manifests, current="0.2.0")["status"], "current")
        self.assertEqual(self.check([stable], manifests, current="0.3.0")["status"], "installed-newer")
        self.assertEqual(self.check([stable], manifests, current="0.2.0-rc.2")["status"], "available")
        self.assertEqual(self.check([stable], manifests, current="unknown")["status"], "installed-version-unknown")

    def test_mutable_missing_and_wrong_identity_fail_closed(self):
        release, manifest = fixture(immutable=False)
        self.assertEqual(self.check([release])["status"], "release-not-immutable")
        release["immutable"] = True
        for key, bad in [("repository", "panxuc/fcitx5-wetypex"), ("architecture", "aarch64"), ("commit", "main")]:
            changed = copy.deepcopy(manifest); changed[key] = bad
            with self.assertRaises(ValueError): self.check([release], {manifest["tag"]: changed})
        release["assets"] = []
        with self.assertRaises(ValueError): self.check([release])

    def test_runtime_changes_are_not_ordinary_updates(self):
        release, manifest = fixture()
        self.assertEqual(self.check([release], {manifest["tag"]: manifest}, runtime="other")["status"], "runtime-review-required")

    def test_network_failure_never_says_current(self):
        output = io.StringIO()
        with patch.dict(update["main"].__globals__, {"check_releases": lambda *args: (_ for _ in ()).throw(OSError("private URL"))}), contextlib.redirect_stdout(output):
            self.assertEqual(update["main"](["--check"]), 1)
        data = json.loads(output.getvalue())
        self.assertEqual(data["status"], "check-failed")
        self.assertNotIn("private URL", output.getvalue())

    def test_version_tag_restrictions(self):
        for tag in ("main", "v0.1.0-dev", "v0.1.0;echo", "v01.2.3", "v1.2.3-rc.0"):
            with self.assertRaises(ValueError): prepare["release_version"](tag)
        self.assertEqual(prepare["release_version"]("v0.1.0-rc.2"), "0.1.0-rc.2")

    def test_redirect_cannot_escape_release_hosts_or_tls(self):
        handler = update["SafeRedirect"]()
        request = urllib.request.Request("https://github.com/eddieqiu79/wetypex-omarchy/releases/download/v0.2.0/release-manifest.json")
        for destination in ("http://github.com/asset", "https://example.invalid/asset", "file:///tmp/asset"):
            with self.assertRaises(ValueError):
                handler.redirect_request(request, None, 302, "Found", {}, destination)

    def test_bundle_validates_bytes_provenance_and_allowlist(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "wetypex-omarchy-source.tar.gz").write_bytes(b"synthetic source")
            package = root / "wetypex-omarchy-1_0.2.0-1-x86_64.pkg.tar.zst"
            package.write_bytes(b"synthetic package")
            source = {"commit": "b" * 40, "project_version": "0.2.0", "version": "0.2.0", "epoch": 1,
                      "runtime_version": "2.2.3.657", "source_sha256": prepare["digest"](root / "wetypex-omarchy-source.tar.gz")}
            (root / "source-manifest.json").write_text(json.dumps(source))
            for name in ("PKGBUILD", "wetypex-omarchy.install", "build-packages.txt", "SHA256SUMS"):
                (root / name).write_text("synthetic")
            metadata = "pkgname = wetypex-omarchy\narch = x86_64\npkgver = 1:0.2.0-1\n"
            with patch.object(prepare["subprocess"], "check_output", return_value=metadata.replace("x86_64", "aarch64")):
                with self.assertRaises(ValueError): prepare["bundle"](root, "v0.2.0", "0.2.0", "b" * 40)
            with patch.object(prepare["subprocess"], "check_output", return_value=metadata):
                with self.assertRaises(ValueError): prepare["bundle"](root, "v0.2.0", "0.2.0", "c" * 40)
                (root / "wetypex-omarchy-source.tar.gz").write_bytes(b"tampered synthetic source")
                with self.assertRaises(ValueError): prepare["bundle"](root, "v0.2.0", "0.2.0", "b" * 40)
                (root / "wetypex-omarchy-source.tar.gz").write_bytes(b"synthetic source")
                (root / "private.key").write_text("synthetic forbidden artifact")
                with self.assertRaises(ValueError): prepare["bundle"](root, "v0.2.0", "0.2.0", "b" * 40)
                (root / "private.key").unlink()
                result = prepare["bundle"](root, "v0.2.0", "0.2.0", "b" * 40)
            self.assertEqual(result["package"]["sha256"], prepare["digest"](package))
            self.assertIn("release-manifest.json", (root / "SHA256SUMS").read_text())
            update["validate_manifest"](result, "v0.2.0")


if __name__ == "__main__": unittest.main()
