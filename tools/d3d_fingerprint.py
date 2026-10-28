#!/usr/bin/env python3
"""Per-function fingerprints of the generated code, for confirming Direct3D function addresses.

Original code (no existing tool reads ReXGlue's output this way): for each sub_X in generated/default it reads the
disassembly comments the code generator writes above every instruction and reports the instruction count, the
memory displacements used (e.g. device offsets such as 12872 for SetVertexShader's shader field, from futari-nx's
docs/futari-notes.md and tools/renut_futari_layout.py) and the direct callees.

  python3 tools/d3d_fingerprint.py <generated/default> ADDR...          one function's fingerprint
  python3 tools/d3d_fingerprint.py <generated/default> --disp N [--lo A --hi B]   functions using displacement N
  python3 tools/d3d_fingerprint.py <generated/default> --callers ADDR   direct callers
"""
import argparse, glob, os, re, collections

FUNC = re.compile(r'^(?:DEFINE_REX_FUNC|PPC_FUNC_IMPL)\((?:__imp__)?sub_([0-9A-F]{8})\)')
INSN = re.compile(r'^\s*// (\w[\w.+-]*)\s*(.*)$')
DISP = re.compile(r'(-?\d+)\((r\d+)\)')

def load(gen):
    funcs = {}
    cur = None
    for p in sorted(glob.glob(os.path.join(gen, '*_recomp.*.cpp'))):
        for line in open(p, errors='replace'):
            m = FUNC.match(line)
            if m:
                cur = funcs.setdefault(int(m.group(1), 16), [])
                continue
            m = INSN.match(line)
            if m and cur is not None:
                cur.append((m.group(1), m.group(2)))
    return funcs

def fp(insns):
    disps = collections.Counter()
    calls = []
    for op, args in insns:
        for d, r in DISP.findall(args):
            if r != 'r1':
                disps[int(d)] += 1
        if op == 'bl':
            calls.append(args.strip())
    return disps, calls

ap = argparse.ArgumentParser()
ap.add_argument('gen'); ap.add_argument('addrs', nargs='*')
ap.add_argument('--disp', type=int, action='append'); ap.add_argument('--callers')
ap.add_argument('--lo', default='82000000'); ap.add_argument('--hi', default='83000000')
ap.add_argument('--asm', action='store_true')
a = ap.parse_args()
F = load(a.gen)
lo, hi = int(a.lo, 16), int(a.hi, 16)
if a.disp:
    for addr in sorted(F):
        if lo <= addr < hi:
            d, c = fp(F[addr])
            if all(x in d for x in a.disp):
                print('sub_%08X  %4d insns  calls %d' % (addr, len(F[addr]), len(c)))
if a.callers:
    t = '0x' + a.callers.lower()
    for addr in sorted(F):
        if any(op == 'bl' and args.strip() == t for op, args in F[addr]):
            print('sub_%08X' % addr)
for s in a.addrs:
    addr = int(s, 16)
    d, c = fp(F[addr])
    print('sub_%08X  %d insns' % (addr, len(F[addr])))
    print('  disps:', ' '.join('%d' % k for k in sorted(d)))
    print('  calls:', ' '.join(c))
    if a.asm:
        for op, args in F[addr]:
            print('   ', op, args)
