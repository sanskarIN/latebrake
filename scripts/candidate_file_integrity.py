"""Read-only, closed-world integrity check for an unverified Win64 candidate.

This independently rechecks every file in a schema-2 UAT candidate
manifest during private manual QA preparation. SHA256 matching does NOT
authenticate Unreal provenance, executable behavior, or playable status.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import stat

MANIFEST_NAME = "Latebrake-candidate-manifest.json"
MAX_MANIFEST_BYTES = 32 * 1024 * 1024
MAX_ENTRIES = 200_000
REPARSE_POINT = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)


def reject(message: str) -> None:
    raise ValueError(message)


def _linked(path: Path) -> bool:
    info = path.lstat()
    return stat.S_ISLNK(info.st_mode) or bool(
        getattr(info, "st_file_attributes", 0) & REPARSE_POINT
    )


def require_unlinked_path(path: Path) -> None:
    """Check all *supplied* ancestors before .resolve() hides a symlink."""
    if not path.is_absolute():
        reject(f"Candidate path must be absolute: {path}")
    for node in (path, *path.parents):
        if _linked(node):
            reject(f"Candidate path traverses a symlink or reparse point: {node}")


def _unique_properties(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        if key in result:
            reject(f"Duplicate candidate manifest property: {key}")
        result[key] = value
    return result


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _hash(value: object) -> bool:
    return isinstance(value, str) and bool(re.fullmatch(r"[a-fA-F0-9]{64}", value))


def _safe_relative(value: object) -> bool:
    if not isinstance(value, str) or not value:
        return False
    if value.startswith("/") or "\\" in value or ":" in value or "\x00" in value:
        return False
    parts = value.split("/")
    return all(part not in ("", ".", "..") for part in parts)


def verify_candidate_archive(root: Path, manifest_path: Path) -> tuple[int, int]:
    """Rehash all packaged files, reject absent/extra/linked archive entries.

    The manifest is intentionally excluded from its own file list. This is
    an integrity comparison, not an authentic package/installed-game test.
    """
    require_unlinked_path(root)
    require_unlinked_path(manifest_path)
    if not root.is_dir() or not manifest_path.is_file():
        reject("Candidate root and its manifest must exist as regular paths.")
    if manifest_path.parent != root or manifest_path.name != MANIFEST_NAME:
        reject("Candidate manifest must be directly within the archive root.")
    size = manifest_path.stat().st_size
    if size <= 0 or size > MAX_MANIFEST_BYTES:
        reject("Candidate manifest is empty or too large.")
    metadata = json.loads(
        manifest_path.read_text(encoding="utf-8-sig"),
        object_pairs_hook=_unique_properties,
    )
    if not isinstance(metadata, dict):
        reject("Candidate manifest must be a JSON object.")
    if (type(metadata.get("schema_version")) is not int or
            metadata["schema_version"] != 2 or
            metadata.get("project") != "LateBrake" or
            metadata.get("platform") != "Win64" or
            metadata.get("status") != "UNVERIFIED_CANDIDATE_NOT_PLAYTESTED" or
            metadata.get("source_tree_tracked_changes") is not False or
            not isinstance(metadata.get("source_commit"), str) or
            not re.fullmatch(r"[a-fA-F0-9]{40}", metadata["source_commit"]) or
            not isinstance(metadata.get("unreal_engine_version"), str) or
            not re.fullmatch(r"5\.8(?:\..*)?", metadata["unreal_engine_version"]) or
            not _hash(metadata.get("starter_map_sha256")) or
            not _hash(metadata.get("unreal_engine_build_sha256"))):
        reject("Candidate schema, platform, provenance or engine metadata is invalid.")

    records = metadata.get("files")
    if not isinstance(records, list) or not 0 < len(records) <= MAX_ENTRIES:
        reject("Candidate manifest must list a bounded, nonempty set of files.")
    expected: dict[str, tuple[str, int, str]] = {}
    for item in records:
        if not isinstance(item, dict) or set(item) != {"path", "bytes", "sha256"}:
            reject("Malformed candidate archive file record.")
        name, byte_count, digest = item["path"], item["bytes"], item["sha256"]
        if (not _safe_relative(name) or
                name.casefold() == MANIFEST_NAME.casefold() or
                type(byte_count) is not int or byte_count <= 0 or
                not _hash(digest)):
            reject(f"Invalid file path, size or hash in candidate manifest: {name!r}")
        folded = name.casefold()
        if folded in expected:
            reject(f"Duplicate or case-colliding candidate file path: {name!r}")
        expected[folded] = (name, byte_count, digest.lower())

    found = set()
    total_bytes = 0
    pending = [root]
    while pending:
        directory = pending.pop()
        # Do not follow any symlink or Windows directory junctions.
        with os.scandir(directory) as entries:
            for entry in entries:
                current = Path(entry.path)
                info = current.lstat()
                if _linked(current):
                    reject(f"Linked archive entry is forbidden: {current}")
                if stat.S_ISDIR(info.st_mode):
                    pending.append(current)
                elif stat.S_ISREG(info.st_mode):
                    if current == manifest_path:
                        continue
                    relative = current.relative_to(root).as_posix()
                    folded = relative.casefold()
                    if folded not in expected or folded in found:
                        reject(f"Unexpected or duplicate candidate file: {relative}")
                    expected_name, expected_bytes, expected_digest = expected[folded]
                    if relative != expected_name:
                        reject(f"Candidate file case differs from its manifest: {relative}")
                    if info.st_size != expected_bytes or _sha256(current) != expected_digest:
                        reject(f"Candidate file length or SHA256 mismatch: {relative}")
                    found.add(folded)
                    total_bytes += info.st_size
                else:
                    reject(f"Non-file/non-directory archive entry: {current}")

    if found != set(expected):
        missing = sorted(set(expected) - found)
        reject(f"Candidate archive is missing listed files: {missing[:3]}")
    return len(found), total_bytes
