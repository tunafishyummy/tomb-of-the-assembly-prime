#!/usr/bin/env python3
"""Play several endless rooms with a solver, then compare every sampled frame of the C build and the asm build.
usage: endless_play.py REF.COM ASM.COM EMU ROOMS"""
import subprocess, sys, re, struct, collections
ref, asm, emu, nrooms = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
subprocess.run(['as','--32','-march=i386','-I','src','src/tomb.s','-o','/tmp/pl.o'],check=True)
subprocess.run(['ld','-m','elf_i386','-T','src/tomb.ld','-Map=/tmp/pl.map','-o','/tmp/PL.COM','/tmp/pl.o'],check=True)
m = open('/tmp/pl.map').read(); nm = subprocess.run(['nm','/tmp/pl.o'],capture_output=True,text=True).stdout
bss = int(re.search(r'0x([0-9a-f]+)\s+__bss_start', m).group(1), 16)
sym = lambda n: bss + int(re.search(r'^([0-9a-f]+) b %s$' % n, nm, re.M).group(1), 16)
ROOM, DEPTH = sym('room'), sym('g_depth')
DX = {'w':(0,-1),'s':(0,1),'a':(-1,0),'d':(1,0)}
def solve(w,h,sx,sy,ex,ey,cells):
    def slide(x,y,k):
        dx,dy = DX[k]; n=0
        while True:
            nx,ny=x+dx,y+dy
            if (nx,ny)==(ex,ey): return 'exit',x,y
            if not(0<=nx<w and 0<=ny<h): break
            c=cells[ny*w+nx]
            if c==ord('{'): return None
            if c not in (32,42,94): break
            x,y=nx,ny; n+=1
        return ('go',x,y) if n else None
    start=(sx,sy); prev={start:None}; q=collections.deque([start])
    while q:
        cur=q.popleft()
        for k in 'wasd':
            r=slide(cur[0],cur[1],k)
            if not r: continue
            if r[0]=='exit':
                path=[k]; p=cur
                while prev[p]: path.append(prev[p][1]); p=prev[p][0]
                return ''.join(reversed(path))
            nxt=(r[1],r[2])
            if nxt not in prev: prev[nxt]=(cur,k); q.append(nxt)
    return None
keys = [(5,'down'),(7,'enter')]
t = 40
def run(com, n, prefix, frames=(), dump=None):
    a=[emu,com,'-n',str(n),'-o',prefix]
    for f in frames: a+=['-d',str(f)]
    if dump: a+=['-x',dump]
    for f,k in keys: a+=['-k','%d:%s'%(f,k)]
    r=subprocess.run(a,capture_output=True,text=True,timeout=1500)
    if 'FAULT' in r.stderr: print(r.stderr[-300:]); sys.exit(2)
for room in range(nrooms):
    run('/tmp/PL.COM', t, '/tmp/pl', dump='/tmp/pl.mem')
    mem=open('/tmp/pl.mem','rb').read()
    w,h,sx,sy,ex,ey=struct.unpack_from('<6H',mem,ROOM); cells=mem[ROOM+22:ROOM+22+w*h]
    depth=struct.unpack_from('<H',mem,DEPTH)[0]
    path=solve(w,h,sx,sy,ex,ey,cells)
    print('room %d: depth %d, %dx%d, solution %d dashes' % (room+1, depth, w, h, len(path)))
    for k in path:
        keys.append((t, k)); t += 80
    t += 330
frames=list(range(8, t, 13))
run(ref, t, '/tmp/ep_ref', frames); run(asm, t, '/tmp/ep_asm', frames)
import os
def load(p):
    d=open(p,'rb').read(); return d[d.index(b'255\n')+4:]
bad=0; n=0
for f in frames:
    a,b='/tmp/ep_ref_%d.ppm'%f,'/tmp/ep_asm_%d.ppm'%f
    if not os.path.exists(a) or not os.path.exists(b): continue
    n+=1
    if load(a)!=load(b): bad+=1; print('frame',f,'DIFFERENT')
print('%d frames compared over %d endless rooms, %d different' % (n, nrooms, bad))
sys.exit(1 if bad else 0)
