#!/usr/bin/env python3
"""Random key scripts through both builds; frames must stay identical.  usage: fuzz_compare.py REF ASM EMU seed frames"""
import random, subprocess, sys
ref, asm, emu, seed, nframes = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
rnd = random.Random(seed)
keys = ['w','a','s','d','up','down','left','right','enter','r','b','h','m','tab','x','enter','enter','w','d','s','a']
args = []
f = 4
while f < nframes - 60:
    k = rnd.choice(keys)
    # menu navigation more likely to need enter early
    args += ['-k', '%d:%s' % (f, k)]
    f += rnd.choice([3, 8, 20, 45, 70, 110])
frames = list(range(10, nframes, 23))
cmd = lambda com, pre: [emu, com, '-o', pre, '-n', str(nframes)] + sum((['-d', str(x)] for x in frames), []) + args
for com, pre in ((ref, '/tmp/fz_ref'), (asm, '/tmp/fz_asm')):
    r = subprocess.run(cmd(com, pre), capture_output=True, text=True, timeout=1500)
    if 'FAULT' in r.stderr: print(com, r.stderr[-200:]); sys.exit(2)
def load(p):
    d = open(p, 'rb').read(); return d[d.index(b'255\n') + 4:]
bad = 0
import os
n = 0
for x in frames:
    pa, pb = '/tmp/fz_ref_%d.ppm' % x, '/tmp/fz_asm_%d.ppm' % x
    if not (os.path.exists(pa) and os.path.exists(pb)):
        if os.path.exists(pa) != os.path.exists(pb): print('frame', x, 'exists in only one build'); bad += 1
        continue
    n += 1
    if load(pa) != load(pb):
        print('frame', x, 'DIFFERENT'); bad += 1
print('seed %d: %d frames compared, %d different' % (seed, n, bad))
sys.exit(1 if bad else 0)
