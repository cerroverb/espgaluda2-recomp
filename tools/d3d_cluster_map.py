#!/usr/bin/env python3
"""Candidate Espgaluda II addresses for futari-nx's named D3D functions, by shared layout delta.

Original code (see WORKLOG.md "Original code"). Both games link D3D9 2.0.9328.16384; functions of one library
object keep their relative layout. For each futari entry A and delta D with A+D a function start here, the
score is how many other futari entries within +-WINDOW also land on function starts with the same D.

  python3 tools/d3d_cluster_map.py <futari_d3d_functions.txt> <generated/default> [WINDOW_HEX]
"""
import glob, os, re, sys
S = set()
for p in glob.glob(os.path.join(sys.argv[2], '*_recomp.*.cpp')):
    for m in re.finditer(r'^DEFINE_REX_FUNC\(sub_([0-9A-F]{8})\)', open(p, errors='replace').read(), re.M):
        S.add(int(m.group(1), 16))
MINRUN = int(sys.argv[4]) if len(sys.argv) > 4 else 5
W = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x3000
names = []
for line in open(sys.argv[1]):
    f = line.split('#')[0].split()
    if len(f) >= 2:
        names.append((f[0], int(f[1], 16)))
names.sort(key=lambda x: x[1])
for n, a in names:
    best = []
    for s in S:
        d = s - a
        if not (0xC0000 <= d <= 0xD8000):
            continue
        sc = [m for m, b in names if b != a and abs(b - a) <= W and (b + d) in S]
        if sc:
            best.append((len(sc), d, s, sc))
    best.sort(reverse=True)
    out = '  '.join('%08X(d%05X,%d)' % (s, d, c) for c, d, s, _ in best[:3])
    print('%-38s %08X  %s' % (n, a, out))

# Runs: maximal sequences of consecutive futari entries that all land on function starts with one delta.
print('\nruns (>=3 consecutive entries at one delta):')
runs = []
for d in sorted({s - a for s in S for _, a in names if 0xC0000 <= s - a <= 0xD8000}):
    cur = []
    for n, a in names + [('', 0)]:
        if a and (a + d) in S:
            cur.append(n)
        else:
            if len(cur) >= MINRUN:
                runs.append((d, list(cur)))
            cur = []
for d, r in runs:
    print('  d%05X  %s' % (d, ' '.join(r)))
