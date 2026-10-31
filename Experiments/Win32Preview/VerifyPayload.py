"""Fail closed when the built setup source embeds bytes other than its EXE.

Build-only integrity check; not an installer execution or hardware QA.
"""
import argparse
import re
import struct
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('preview',type=Path)
parser.add_argument('setup',type=Path)
parser.add_argument('payload',type=Path)
args=parser.parse_args()

def check_pe(path):
    b=path.read_bytes()
    if len(b)<4096 or b[:2]!=b'MZ':raise SystemExit(f'Invalid PE: {path}')
    peoff=struct.unpack_from('<I',b,0x3c)[0]
    if peoff+26>=len(b) or b[peoff:peoff+4]!=b'PE\x00\x00':
        raise SystemExit(f'Invalid PE signature: {path}')
    machine=struct.unpack_from('<H',b,peoff+4)[0]
    magic=struct.unpack_from('<H',b,peoff+24)[0]
    if machine!=0x8664 or magic!=0x20b:raise SystemExit(f'Not x64 PE32+: {path}')
    return b

game=check_pe(args.preview)
setup=check_pe(args.setup)
contents=args.payload.read_text(encoding='utf-8')
if not re.fullmatch(r'[0-9,\s]+',contents):raise SystemExit('Invalid generated payload syntax')
nums=[int(s) for s in re.findall(r'\d+',contents)]
if any(x>255 for x in nums) or bytes(nums)!=game:
    raise SystemExit('Installer payload does not exactly equal preview EXE bytes')
print(f'PASS: x64 game ({len(game)} bytes), x64 setup ({len(setup)} bytes), exact embedded payload')
