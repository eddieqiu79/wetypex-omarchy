#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate an exact release tag and construct an adapter-only release manifest."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = "eddieqiu79/wetypex-omarchy"


def release_version(tag):
    if not re.fullmatch(r"v(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-rc\.[1-9]\d*)?", tag):
        raise ValueError("Use vMAJOR.MINOR.PATCH or vMAJOR.MINOR.PATCH-rc.N; development snapshots are not releases")
    return tag[1:]


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def validate_tag(tag):
    version = release_version(tag)
    head = git("rev-parse", "HEAD")
    if git("rev-parse", "refs/tags/" + tag + "^{commit}") != head:
        raise ValueError("Checkout must be exactly the release tag")
    if (ROOT / "VERSION").read_text().strip() != version:
        raise ValueError("Tag and VERSION differ")
    if git("status", "--porcelain", "--untracked-files=normal"):
        raise ValueError("Release checkout must be clean")
    return version, head


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def bundle(directory, tag, version, commit):
    source = json.loads((directory / "source-manifest.json").read_text())
    expected = version.replace("-rc.", "rc")
    if (source.get("commit"), source.get("project_version"), source.get("version"), source.get("epoch")) != (commit, version, expected, 1):
        raise ValueError("Build provenance does not match release tag/version/epoch")
    packages = list(directory.glob("*.pkg.tar.zst"))
    if len(packages) != 1:
        raise ValueError("Expected exactly one adapter package")
    package = packages[0]
    if not re.fullmatch(r"wetypex-omarchy-[A-Za-z0-9._+-]+-x86_64\.pkg\.tar\.zst", package.name):
        raise ValueError("Unexpected package filename")
    metadata = subprocess.check_output(["bsdtar", "-xOf", str(package), ".PKGINFO"], text=True)
    info = dict(line.split(" = ", 1) for line in metadata.splitlines() if " = " in line)
    if info.get("pkgname") != "wetypex-omarchy" or info.get("arch") != "x86_64" or info.get("pkgver") != "1:" + expected + "-1":
        raise ValueError("Package metadata does not match expected adapter/version/architecture")
    source_archive = directory / "wetypex-omarchy-source.tar.gz"
    if digest(source_archive) != source.get("source_sha256"):
        raise ValueError("Source archive checksum mismatch")
    required = {"source-manifest.json", "wetypex-omarchy-source.tar.gz", "PKGBUILD",
                "wetypex-omarchy.install", "build-packages.txt", "SHA256SUMS", package.name}
    if {p.name for p in directory.iterdir()} != required:
        raise ValueError("Release directory has unexpected or missing assets")
    manifest = {
        "schema": 1, "repository": REPOSITORY, "tag": tag,
        "version": version, "commit": commit,
        "channel": "preview" if "-rc." in version else "stable",
        "architecture": "x86_64", "runtime_version": source["runtime_version"],
        "package": {"name": package.name, "sha256": digest(package),
                    "size": package.stat().st_size, "pacman_version": info["pkgver"]},
        "source": {"name": source_archive.name, "sha256": source["source_sha256"]},
        "installation": "manual-pacman-U", "restart": "explicit-after-active-work-finishes",
    }
    (directory / "release-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    assets = sorted(p for p in directory.iterdir() if p.name != "SHA256SUMS")
    (directory / "SHA256SUMS").write_text("".join(digest(p) + "  " + p.name + "\n" for p in assets))
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--assets", type=Path)
    args = parser.parse_args()
    try:
        version, commit = validate_tag(args.tag)
        result = bundle(args.assets, args.tag, version, commit) if args.assets else {
            "version": version, "commit": commit, "prerelease": "-rc." in version}
        print(json.dumps(result))
    except (ValueError, OSError, subprocess.SubprocessError, KeyError) as error:
        parser.exit(1, "Release validation failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
