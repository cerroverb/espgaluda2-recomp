#!/usr/bin/env python3
"""Pairs futari-nx's named Direct3D functions (XDK 2.0.9328) with Espgaluda II's by layout.

Original code (no existing tool covers it): both games link D3D9 2.0.9328.16384 (xex static-library header), so
the D3D objects should sit in the same order with the same function sizes. For each futari-nx entry this lists the
Espgaluda II function starts at futari address + delta, for the deltas that pair many entries. A pairing is only a
candidate: each one is then confirmed by instructions (see WORKLOG.md).

  python3 tools/d3d_layout_align.py <futari_d3d_functions.txt> <generated/default>
"""
import collections, glob, os, re, sys

def starts(gen):
    s = set()
    for p in glob.glob(os.path.join(gen, '*_recomp.*.cpp')):
        for m in re.finditer(r'^(?:DEFINE_REX_FUNC|PPC_FUNC_IMPL)\((?:__imp__)?sub_([0-9A-F]{8})\)', open(p, errors='replace').read(), re.M):
            s.add(int(m.group(1), 16))
    return s

names = []
for line in open(sys.argv[1]):
    f = line.split('#')[0].split()
    if len(f) >= 2:
        names.append((f[0], int(f[1], 16)))
S = starts(sys.argv[2])
cnt = collections.Counter()
for n, a in names:
    for s in S:
        d = s - a
        if 0 <= d < 0x200000:
            cnt[d] += 1
for d, c in cnt.most_common(8):
    print('delta %06X pairs %d' % (d, c))

# Local runs: consecutive futari entries (by address) whose spacing is reproduced exactly by two Espgaluda starts.
print()
srt = sorted(names, key=lambda x: x[1])
S_sorted = sorted(S)
per = collections.defaultdict(set)
for i in range(len(srt) - 1):
    (n1, a1), (n2, a2) = srt[i], srt[i + 1]
    for s in S_sorted:
        if s + (a2 - a1) in S and 0x82090000 <= s < 0x82400000:
            d = s - a1
            per[n1].add(d); per[n2].add(d)
for n, a in srt:
    ds = sorted(per.get(n, ()))
    print('%-40s %08X %s' % (n, a, ' '.join('%06X' % d for d in ds[:12]) + (' ...' if len(ds) > 12 else '')))
