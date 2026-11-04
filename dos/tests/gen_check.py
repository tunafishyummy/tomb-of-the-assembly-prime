#!/usr/bin/env python3
"""Compare the assembly room generator with the C reference: same seed + depth must give identical rooms.
usage: gen_check.py [seed depth]...   (default: a list of cases)"""
import subprocess, sys, re, struct
cases = [(int(sys.argv[i]), int(sys.argv[i+1])) for i in range(1, len(sys.argv)-1, 2)] or \
        [(1005,1),(77,2),(4242,3),(9,5),(31337,8),(555,12),(2024,20),(8,33),(99999,40)]
bad = 0
for seed, depth in cases:
    subprocess.run(['as','--32','-march=i386','-I','src','--defsym','SELFTEST=1','--defsym','TSEED=%d'%seed,'--defsym','TDEPTH=%d'%depth,'src/tomb.s','-o','/tmp/st.o'],check=True)
    subprocess.run(['ld','-m','elf_i386','-T','src/tomb.ld','-Map=/tmp/st.map','-o','/tmp/ST.COM','/tmp/st.o'],check=True)
    m = open('/tmp/st.map').read()
    bss = int(re.search(r'0x([0-9a-f]+)\s+__bss_start', m).group(1), 16)
    nm = subprocess.run(['nm','/tmp/st.o'],capture_output=True,text=True).stdout
    room = bss + int(re.search(r'^([0-9a-f]+) b room$', nm, re.M).group(1), 16)
    seedsym = bss + int(re.search(r'^([0-9a-f]+) b seed$', nm, re.M).group(1), 16)
    r = subprocess.run(['tests/x86run','/tmp/ST.COM','-n','5','-x','/tmp/st.mem'],capture_output=True,text=True,timeout=300)
    if 'FAULT' in r.stderr: print(seed, depth, r.stderr[-200:]); bad += 1; continue
    mem = open('/tmp/st.mem','rb').read()
    w,h,sx,sy,ex,ey = struct.unpack_from('<6H', mem, room)
    cells = mem[room+22: room+22+w*h]
    aseed = struct.unpack_from('<I', mem, seedsym)[0]
    out = subprocess.run(['tests/gen_dump',str(seed),str(depth)],capture_output=True,text=True).stdout.split('\n')
    cw,ch,csx,csy,cex,cey,cseed = map(int,out[0].split())
    ccells = bytes.fromhex(out[1])
    ok = (w,h,sx,sy,ex,ey)==(cw,ch,csx,csy,cex,cey) and cells==ccells and aseed==cseed
    print('seed %6d depth %2d: %s (%dx%d)' % (seed, depth, 'identical' if ok else 'DIFFERENT asm=%s c=%s seed asm=%d c=%d' % ((w,h,sx,sy,ex,ey),(cw,ch,csx,csy,cex,cey),aseed,cseed), w, h))
    bad += not ok
sys.exit(1 if bad else 0)
