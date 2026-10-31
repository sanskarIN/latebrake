"""Embed exact compiled standalone EXE in the local-only setup application.
Run only on outputs you built yourself. No downloads or remote API calls.
"""
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("game", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
raw = args.game.read_bytes()
if len(raw) < 4096 or raw[:2] != b"MZ":
    raise SystemExit("Expected a real compiled Windows PE executable")
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(",\n".join(",".join(str(b) for b in raw[i:i + 40]) for i in range(0, len(raw), 40)) + "\n", encoding="utf-8")
print(f"Embedded {len(raw)} bytes from {args.game} into {args.output}")
