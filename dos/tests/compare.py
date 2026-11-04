#!/usr/bin/env python3
"""Run the C reference COM and the assembly COM with the same key script and compare the frames.
usage: compare.py REF.COM ASM.COM EMU frames... [-- extra emulator args like -k 5:enter]"""
import subprocess, sys
ref, asm, emu = sys.argv[1:4]
rest = sys.argv[4:]
frames = [a for a in rest if a.isdigit()]
extra = [a for a in rest if not a.isdigit()]
extra2 = []
i = 0
while i < len(extra):
    extra2.append(extra[i]); i += 1
def run(com, prefix):
    args = [emu, com, '-o', prefix, '-n', str(max(map(int, frames)) + 2)]
    for f in frames: args += ['-d', f]
    args += extra2
    r = subprocess.run(args, capture_output=True, text=True, timeout=900)
    if 'FAULT' in r.stderr: print(com, r.stderr[-300:]); sys.exit(2)
def load(path):
    d = open(path, 'rb').read(); return d[d.index(b'255\n') + 4:]
run(ref, '/tmp/cmp_ref'); run(asm, '/tmp/cmp_asm')
bad = 0
for f in frames:
    a = load('/tmp/cmp_ref_%s.ppm' % f); b = load('/tmp/cmp_asm_%s.ppm' % f)
    diff = sum(1 for x, y in zip(a, b) if x != y) // 3
    print('frame %5s: %s' % (f, 'identical' if diff == 0 else '%d pixels differ' % diff))
    bad += diff != 0
sys.exit(1 if bad else 0)
