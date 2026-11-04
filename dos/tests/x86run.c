/* Minimal 386 real-mode interpreter, just enough to run TOMB.COM headless:
   executes the real .COM, emulates BIOS INT 10h/16h/1Ah, DOS INT 21h and VGA port 3DA.
   Usage: x86run TOMB.COM [-n maxframes] [-k frame:key]... [-d frame]... [-o prefix]
   keys: enter esc up down left right tab or a single character */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static uint8_t mem[1 << 20];
static uint32_t R[8];                 /* eax ecx edx ebx esp ebp esi edi */
static uint16_t S[6];                 /* es cs ss ds fs gs */
static uint32_t ip;
static int CF, ZF, SF, OF, PF, DF;
enum { AX, CX, DX, BX, SP, BP, SI, DI };
enum { ES, CS, SS, DS };
static long icount, frames;
static unsigned char pal[256][3];
static int mode13;

static void die(const char *m) { fprintf(stderr, "EMU FAULT: %s at %04x:%04x (instr %ld, frame %ld)\n", m, S[CS], ip, icount, frames); exit(2); }
static uint32_t lin(int seg, uint32_t off) { uint32_t a = ((uint32_t)S[seg] << 4) + off; if (a >= sizeof mem) die("linear address out of range"); return a; }
static uint8_t  rd8(int seg, uint32_t off)  { return mem[lin(seg, off)]; }
static uint16_t rd16(int seg, uint32_t off) { return rd8(seg, off) | (rd8(seg, off + 1) << 8); }
static uint32_t rd32(int seg, uint32_t off) { return rd16(seg, off) | ((uint32_t)rd16(seg, off + 2) << 16); }
static void wr8(int seg, uint32_t off, uint8_t v)  { mem[lin(seg, off)] = v; }
static void wr16(int seg, uint32_t off, uint16_t v) { wr8(seg, off, v); wr8(seg, off + 1, v >> 8); }
static void wr32(int seg, uint32_t off, uint32_t v) { wr16(seg, off, v); wr16(seg, off + 2, v >> 16); }
static uint32_t rdsz(int seg, uint32_t off, int sz) { return sz == 1 ? rd8(seg, off) : sz == 2 ? rd16(seg, off) : rd32(seg, off); }
static void wrsz(int seg, uint32_t off, uint32_t v, int sz) { if (sz == 1) wr8(seg, off, v); else if (sz == 2) wr16(seg, off, v); else wr32(seg, off, v); }

static uint8_t f8(void)  { uint8_t v = rd8(CS, ip); ip = (ip + 1) & 0xFFFF; return v; }
static uint16_t f16(void){ uint16_t v = f8(); v |= f8() << 8; return v; }
static uint32_t f32(void){ uint32_t v = f16(); v |= (uint32_t)f16() << 16; return v; }
static uint32_t fimm(int sz) { return sz == 1 ? f8() : sz == 2 ? f16() : f32(); }

/* ---- prefixes / state of the instruction being decoded */
static int osz, asz, segov;
static void push(uint32_t v, int sz) { if (sz == 4) { R[SP] -= 4; wr32(SS, R[SP] & 0xFFFF, v); } else { R[SP] = (R[SP] & ~0xFFFFu) | ((R[SP] - 2) & 0xFFFF); wr16(SS, R[SP] & 0xFFFF, v); } }
static uint32_t pop(int sz) { uint32_t v; if (sz == 4) { v = rd32(SS, R[SP] & 0xFFFF); R[SP] += 4; } else { v = rd16(SS, R[SP] & 0xFFFF); R[SP] = (R[SP] & ~0xFFFFu) | ((R[SP] + 2) & 0xFFFF); } return v; }
/* gcc -m16 code keeps ESP inside the segment: if its upper half is non-zero the real CPU would fault */
static void check_esp(void) { if (R[SP] >> 16) die("ESP upper half not zero"); }

static uint32_t getreg(int i, int sz) { if (sz == 1) return (i < 4) ? (R[i] & 0xFF) : ((R[i - 4] >> 8) & 0xFF); if (sz == 2) return R[i] & 0xFFFF; return R[i]; }
static void setreg(int i, uint32_t v, int sz) { if (sz == 1) { if (i < 4) R[i] = (R[i] & ~0xFFu) | (v & 0xFF); else R[i - 4] = (R[i - 4] & ~0xFF00u) | ((v & 0xFF) << 8); } else if (sz == 2) R[i] = (R[i] & ~0xFFFFu) | (v & 0xFFFF); else R[i] = v; }

typedef struct { int is_reg; int reg; int seg; uint32_t off; } Opnd;
static int mod_, reg_, rm_;
static Opnd modrm(void)
{
    Opnd o; uint8_t m = f8(); mod_ = m >> 6; reg_ = (m >> 3) & 7; rm_ = m & 7;
    o.is_reg = 0; o.reg = 0; o.seg = DS; o.off = 0;
    if (mod_ == 3) { o.is_reg = 1; o.reg = rm_; return o; }
    if (asz == 2) {
        uint32_t d = 0;
        switch (rm_) {
        case 0: o.off = R[BX] + R[SI]; break; case 1: o.off = R[BX] + R[DI]; break;
        case 2: o.off = R[BP] + R[SI]; o.seg = SS; break; case 3: o.off = R[BP] + R[DI]; o.seg = SS; break;
        case 4: o.off = R[SI]; break; case 5: o.off = R[DI]; break;
        case 6: if (mod_ == 0) o.off = 0; else { o.off = R[BP]; o.seg = SS; } break;
        case 7: o.off = R[BX]; break;
        }
        if (mod_ == 0 && rm_ == 6) d = f16(); else if (mod_ == 1) d = (uint32_t)(int32_t)(int8_t)f8(); else if (mod_ == 2) d = f16();
        o.off = (o.off + d) & 0xFFFF;
    } else {
        uint32_t base = 0, idx = 0, d = 0; int ss = 0, sib = -1;
        if (rm_ == 4) sib = f8();
        if (sib >= 0) {
            int b = sib & 7, i = (sib >> 3) & 7; ss = sib >> 6;
            if (i != 4) idx = R[i] << ss;
            if (b == 5 && mod_ == 0) base = 0; else { base = R[b]; if (b == 4 || b == 5) o.seg = SS; }
            if (b == 5 && mod_ == 0) d = f32();
        } else if (rm_ == 5 && mod_ == 0) { d = f32(); }
        else { base = R[rm_]; if (rm_ == 5) o.seg = SS; }
        if (mod_ == 1) d = (uint32_t)(int32_t)(int8_t)f8(); else if (mod_ == 2) d = f32();
        o.off = base + idx + d;
    }
    if (segov >= 0) o.seg = segov;
    return o;
}
static void chk(Opnd o) { if (!o.is_reg && o.off > 0xFFFF) die("memory access with 32-bit offset above 64K (would #GP in real mode)"); }
static uint32_t rdop(Opnd o, int sz) { chk(o); return o.is_reg ? getreg(o.reg, sz) : rdsz(o.seg, o.off, sz); }
static void wrop(Opnd o, uint32_t v, int sz) { chk(o); if (o.is_reg) setreg(o.reg, v, sz); else wrsz(o.seg, o.off, v, sz); }

static uint32_t mask(int sz) { return sz == 1 ? 0xFF : sz == 2 ? 0xFFFF : 0xFFFFFFFFu; }
static uint32_t sbit(int sz) { return sz == 1 ? 0x80 : sz == 2 ? 0x8000 : 0x80000000u; }
static void szp(uint32_t r, int sz) { r &= mask(sz); ZF = r == 0; SF = (r & sbit(sz)) != 0; { uint8_t b = r & 0xFF; b ^= b >> 4; b ^= b >> 2; b ^= b >> 1; PF = !(b & 1); } }
static uint32_t alu(int op, uint32_t a, uint32_t b, int sz)
{
    uint64_t r; uint32_t m = mask(sz), c = CF;
    a &= m; b &= m;
    switch (op) {
    case 0: case 2: r = (uint64_t)a + b + (op == 2 ? c : 0); CF = (r >> (sz * 8)) & 1; OF = ((~(a ^ b) & (a ^ (uint32_t)r)) & sbit(sz)) != 0; break;
    case 5: case 7: case 3: r = (uint64_t)a - b - (op == 3 ? c : 0); CF = (op == 3) ? ((uint64_t)a < (uint64_t)b + c) : (a < b); OF = (((a ^ b) & (a ^ (uint32_t)r)) & sbit(sz)) != 0; break;
    case 1: r = a | b; CF = OF = 0; break;
    case 4: r = a & b; CF = OF = 0; break;
    case 6: r = a ^ b; CF = OF = 0; break;
    default: die("alu op"); r = 0;
    }
    szp((uint32_t)r, sz);
    return (uint32_t)r & m;
}
static int cond(int cc)
{
    int r = 0;
    switch (cc >> 1) {
    case 0: r = OF; break; case 1: r = CF; break; case 2: r = ZF; break; case 3: r = CF || ZF; break;
    case 4: r = SF; break; case 5: r = PF; break; case 6: r = SF != OF; break; case 7: r = ZF || (SF != OF); break;
    }
    return (cc & 1) ? !r : r;
}
static int32_t sx(uint32_t v, int sz) { return sz == 1 ? (int32_t)(int8_t)v : sz == 2 ? (int32_t)(int16_t)v : (int32_t)v; }

/* ---- host environment -------------------------------------------------- */
typedef struct { long frame; int key; } Key;
static Key keys[512]; static int nkeys, kpos;
static long dumpf[1024]; static int ndump; static const char *prefix = "/tmp/emu";
static long maxframes = 2000; static int exited; static const char *memdump;
static int vs_toggle;

static void dump(long f)
{
    char nm[256]; FILE *fp; int i; fprintf(stderr, "frame %ld at instruction %ld\n", f, icount); sprintf(nm, "%s_%ld.ppm", prefix, f); fp = fopen(nm, "wb");
    fprintf(fp, "P6\n320 200\n255\n");
    for (i = 0; i < 64000; i++) { uint8_t c = mem[0xA0000 + i]; fputc(pal[c][0] * 4, fp); fputc(pal[c][1] * 4, fp); fputc(pal[c][2] * 4, fp); }
    fclose(fp);
}
static int key_ready(void) { return kpos < nkeys && keys[kpos].frame <= frames; }
static int in_port(int port)
{
    if (port == 0x3DA) {
        int v;
        vs_toggle++;
        v = (vs_toggle & 1) ? 8 : 0;
        if (v == 8 && !(vs_toggle & 2)) { int i; frames++; for (i = 0; i < ndump; i++) if (dumpf[i] == frames) dump(frames); if (frames > maxframes) { fprintf(stderr, "reached frame limit %ld\n", maxframes); if (memdump) { FILE *o = fopen(memdump, "wb"); fwrite(mem + 0x10000, 1, 0x10000, o); fclose(o); } exit(0); } }
        return v;
    }
    die("unexpected IN port"); return 0;
}
static void do_int(int n)
{
    int ah = (R[AX] >> 8) & 0xFF;
    if (n == 0x10) {
        if ((R[AX] & 0xFFFF) == 0x0013) mode13 = 1;
        else if ((R[AX] & 0xFFFF) == 0x0003) mode13 = 0;
        else if ((R[AX] & 0xFFFF) == 0x1012) { int i, first = R[BX] & 0xFFFF, cnt = R[CX] & 0xFFFF; for (i = 0; i < cnt * 3; i++) pal[first + i / 3][i % 3] = rd8(ES, (R[DX] & 0xFFFF) + i); }
        else die("unhandled INT 10h");
    } else if (n == 0x16) {
        if (ah == 1) { ZF = !key_ready(); if (key_ready()) R[AX] = (R[AX] & ~0xFFFFu) | (keys[kpos].key & 0xFFFF); }
        else if (ah == 0) { if (!key_ready()) die("INT 16h AH=0 with empty buffer (would block forever)"); R[AX] = (R[AX] & ~0xFFFFu) | (keys[kpos++].key & 0xFFFF); }
        else die("unhandled INT 16h");
    } else if (n == 0x1A) {
        R[CX] = (R[CX] & ~0xFFFFu) | 0x0012; R[DX] = (R[DX] & ~0xFFFFu) | ((0x3456 + frames) & 0xFFFF);
    } else if (n == 0x21) {
        if (ah == 0x4C) { fprintf(stderr, "program exited with code %u after %ld frames, %ld instructions\n", (unsigned)(R[AX] & 0xFF), frames, icount); exited = 1; }
        else if (ah == 0x02) { fputc(R[DX] & 0xFF, stderr); }
        else if (ah == 0x09) { uint32_t o = R[DX] & 0xFFFF; fprintf(stderr, "DOS print: "); while (rd8(DS, o) != '$') fputc(rd8(DS, o++), stderr); fputc('\n', stderr); }
        else die("unhandled INT 21h");
    } else die("unhandled interrupt");
}

static void strop(int op, int sz)
{   /* A4/A5 movs, AA/AB stos; address size is 16 bit here (CX,SI,DI) */
    int step = (DF ? -1 : 1) * sz;
    if (asz != 2) die("32-bit address size string op");
    for (;;) {
        uint32_t cnt = R[CX] & 0xFFFF;
        if (op == 0xA4 || op == 0xA5) { wrsz(ES, R[DI] & 0xFFFF, rdsz(segov >= 0 ? segov : DS, R[SI] & 0xFFFF, sz), sz); R[SI] = (R[SI] & ~0xFFFFu) | ((R[SI] + step) & 0xFFFF); }
        else wrsz(ES, R[DI] & 0xFFFF, R[AX], sz);
        R[DI] = (R[DI] & ~0xFFFFu) | ((R[DI] + step) & 0xFFFF);
        R[CX] = (R[CX] & ~0xFFFFu) | ((cnt - 1) & 0xFFFF);
        if (cnt - 1 == 0) break;
    }
}

static void step(void)
{
    int rep = 0, op, sz, rm_done = 0;
    Opnd o;
    osz = 2; asz = 2; segov = -1;
    for (;;) {
        op = f8();
        if (op == 0x66) osz = 4; else if (op == 0x67) asz = 4;
        else if (op == 0x26) segov = ES; else if (op == 0x2E) segov = CS; else if (op == 0x36) segov = SS; else if (op == 0x3E) segov = DS;
        else if (op == 0xF3 || op == 0xF2) rep = 1; else if (op == 0xF0) {}
        else break;
    }
    (void)rm_done;
    if (op < 0x40 && (op & 7) < 6) {                       /* ALU group */
        int a = op >> 3, form = op & 7; sz = (form & 1) ? osz : 1;
        if (form < 4) { o = modrm(); if (form < 2) { uint32_t r = alu(a, rdop(o, sz), getreg(reg_, sz), sz); if (a != 7) wrop(o, r, sz); } else { uint32_t r = alu(a, getreg(reg_, sz), rdop(o, sz), sz); if (a != 7) setreg(reg_, r, sz); } }
        else { uint32_t imm = fimm(sz), r = alu(a, getreg(AX, sz), imm, sz); if (a != 7) setreg(AX, r, sz); }
        return;
    }
    if (op >= 0x40 && op <= 0x4F) { int r = op & 7; uint32_t v = getreg(r, osz), c = CF; v = alu((op < 0x48) ? 0 : 5, v, 1, osz); CF = c; setreg(r, v, osz); return; }
    if (op >= 0x50 && op <= 0x57) { push(getreg(op & 7, osz), osz); return; }
    if (op >= 0x58 && op <= 0x5F) { setreg(op & 7, pop(osz), osz); return; }
    if (op == 0x06 || op == 0x0E || op == 0x16 || op == 0x1E) { push(S[(op >> 3) & 3 ? ((op >> 3) & 3) : ES], 2); return; }
    if (op == 0x07 || op == 0x17 || op == 0x1F) { S[(op >> 3) & 3] = pop(2); return; }
    if (op == 0x68) { push(fimm(osz), osz); return; }
    if (op == 0x6A) { push((uint32_t)(int32_t)(int8_t)f8(), osz); return; }
    if (op == 0x69 || op == 0x6B) {
        int32_t b, a; int64_t p; o = modrm(); b = op == 0x6B ? (int32_t)(int8_t)f8() : sx(fimm(osz), osz); a = sx(rdop(o, osz), osz);
        p = (int64_t)a * b; setreg(reg_, (uint32_t)p, osz); CF = OF = (p != sx((uint32_t)p, osz)); return;
    }
    if (op >= 0x70 && op <= 0x7F) { int8_t d = (int8_t)f8(); if (cond(op & 15)) ip = (ip + d) & 0xFFFF; return; }
    if (op == 0x80 || op == 0x81 || op == 0x83) {
        uint32_t imm; sz = (op == 0x80) ? 1 : osz; o = modrm();
        imm = (op == 0x81) ? fimm(osz) : (op == 0x80 ? f8() : (uint32_t)(int32_t)(int8_t)f8());
        { uint32_t r = alu(reg_, rdop(o, sz), imm, sz); if (reg_ != 7) wrop(o, r, sz); } return;
    }
    if (op == 0x84 || op == 0x85) { sz = op == 0x84 ? 1 : osz; o = modrm(); alu(4, rdop(o, sz), getreg(reg_, sz), sz); return; }
    if (op == 0x86 || op == 0x87) { uint32_t t; sz = op == 0x86 ? 1 : osz; o = modrm(); t = rdop(o, sz); wrop(o, getreg(reg_, sz), sz); setreg(reg_, t, sz); return; }
    if (op >= 0x88 && op <= 0x8B) { sz = (op & 1) ? osz : 1; o = modrm(); if (op < 0x8A) wrop(o, getreg(reg_, sz), sz); else setreg(reg_, rdop(o, sz), sz); return; }
    if (op == 0x8C) { o = modrm(); wrop(o, S[reg_], 2); return; }
    if (op == 0x8E) { o = modrm(); S[reg_] = rdop(o, 2); return; }
    if (op == 0x8D) { o = modrm(); setreg(reg_, o.off, osz); return; }
    if (op == 0x8F) { o = modrm(); wrop(o, pop(osz), osz); return; }
    if (op == 0x90) return;
    if (op > 0x90 && op <= 0x97) { uint32_t t = getreg(op & 7, osz); setreg(op & 7, getreg(AX, osz), osz); setreg(AX, t, osz); return; }
    if (op == 0x98) { if (osz == 4) R[AX] = (uint32_t)(int32_t)(int16_t)R[AX]; else R[AX] = (R[AX] & ~0xFFFFu) | ((uint32_t)(int16_t)(int8_t)R[AX] & 0xFFFF); return; }
    if (op == 0x99) { if (osz == 4) R[DX] = ((int32_t)R[AX] < 0) ? 0xFFFFFFFFu : 0; else R[DX] = (R[DX] & ~0xFFFFu) | (((int16_t)R[AX] < 0) ? 0xFFFF : 0); return; }
    if (op == 0xA0 || op == 0xA1) { uint32_t off = asz == 2 ? f16() : f32(); sz = op == 0xA0 ? 1 : osz; setreg(AX, rdsz(segov >= 0 ? segov : DS, off, sz), sz); return; }
    if (op == 0xA2 || op == 0xA3) { uint32_t off = asz == 2 ? f16() : f32(); sz = op == 0xA2 ? 1 : osz; wrsz(segov >= 0 ? segov : DS, off, getreg(AX, sz), sz); return; }
    if (op == 0xA4 || op == 0xA5 || op == 0xAA || op == 0xAB) { sz = (op & 1) ? osz : 1; if (rep) strop(op, sz); else { uint32_t c = R[CX]; R[CX] = 1; strop(op, sz); R[CX] = c; } return; }
    if (op == 0xA8 || op == 0xA9) { sz = op == 0xA8 ? 1 : osz; alu(4, getreg(AX, sz), fimm(sz), sz); return; }
    if (op >= 0xB0 && op <= 0xB7) { setreg(op & 7, f8(), 1); return; }
    if (op >= 0xB8 && op <= 0xBF) { setreg(op & 7, fimm(osz), osz); return; }
    if (op == 0xC0 || op == 0xC1 || op == 0xD0 || op == 0xD1 || op == 0xD2 || op == 0xD3) {
        uint32_t v, cnt; sz = (op & 1) ? osz : 1; o = modrm(); v = rdop(o, sz);
        cnt = (op <= 0xC1) ? f8() : (op <= 0xD1 ? 1 : (R[CX] & 0xFF)); cnt &= 31;
        if (cnt) {
            uint32_t m = mask(sz), r = v; int i;
            switch (reg_) {
            case 4: case 6: for (i = 0; i < (int)cnt; i++) { CF = (r & sbit(sz)) != 0; r = (r << 1) & m; } szp(r, sz); OF = ((r & sbit(sz)) != 0) != CF; break;
            case 5: for (i = 0; i < (int)cnt; i++) { CF = r & 1; r >>= 1; } szp(r, sz); OF = (v & sbit(sz)) != 0; break;
            case 7: { int32_t sv = sx(v, sz); for (i = 0; i < (int)cnt; i++) { CF = sv & 1; sv >>= 1; } r = (uint32_t)sv & m; szp(r, sz); OF = 0; break; }
            case 0: for (i = 0; i < (int)cnt; i++) { CF = (r & sbit(sz)) != 0; r = ((r << 1) | CF) & m; } break;
            case 1: for (i = 0; i < (int)cnt; i++) { CF = r & 1; r = (r >> 1) | (CF ? sbit(sz) : 0); } break;
            default: die("shift/rotate kind");
            }
            wrop(o, r, sz);
        }
        return;
    }
    if (op == 0xC3) { ip = pop(osz) & 0xFFFF; return; }
    if (op == 0xC6 || op == 0xC7) { sz = op == 0xC6 ? 1 : osz; o = modrm(); wrop(o, fimm(sz), sz); return; }
    if (op == 0xCD) { do_int(f8()); return; }
    if (op == 0xE8) { int32_t d = osz == 4 ? (int32_t)f32() : (int16_t)f16(); push(ip, osz); ip = (ip + d) & 0xFFFF; return; }
    if (op == 0xE9) { int32_t d = osz == 4 ? (int32_t)f32() : (int16_t)f16(); ip = (ip + d) & 0xFFFF; return; }
    if (op == 0xE2) { int8_t d = (int8_t)f8(); uint32_t c; if (asz == 2) { c = ((R[CX] - 1) & 0xFFFF); R[CX] = (R[CX] & ~0xFFFFu) | c; } else { c = --R[CX]; } if (c != 0) ip = (ip + d) & 0xFFFF; return; }
    if (op == 0xEB) { int8_t d = (int8_t)f8(); ip = (ip + d) & 0xFFFF; return; }
    if (op == 0xEC) { setreg(AX, in_port(R[DX] & 0xFFFF), 1); return; }
    if (op == 0xFC) { DF = 0; return; }
    if (op == 0xF6 || op == 0xF7) {
        sz = op == 0xF6 ? 1 : osz; o = modrm();
        switch (reg_) {
        case 0: alu(4, rdop(o, sz), fimm(sz), sz); break;
        case 2: wrop(o, ~rdop(o, sz), sz); break;
        case 3: { uint32_t v = rdop(o, sz); wrop(o, alu(5, 0, v, sz), sz); CF = v != 0; break; }
        case 4: { uint64_t p = (uint64_t)getreg(AX, sz) * rdop(o, sz);
                  if (sz == 4) { R[AX] = (uint32_t)p; R[DX] = (uint32_t)(p >> 32); CF = OF = R[DX] != 0; }
                  else if (sz == 2) { R[AX] = (R[AX] & ~0xFFFFu) | (p & 0xFFFF); R[DX] = (R[DX] & ~0xFFFFu) | ((p >> 16) & 0xFFFF); CF = OF = (p >> 16) != 0; }
                  else { R[AX] = (R[AX] & ~0xFFFFu) | (p & 0xFFFF); CF = OF = (p >> 8) != 0; } break; }
        case 5: { int64_t p = (int64_t)sx(getreg(AX, sz), sz) * sx(rdop(o, sz), sz);
                  if (sz == 4) { R[AX] = (uint32_t)p; R[DX] = (uint32_t)((uint64_t)p >> 32); CF = OF = p != (int32_t)p; }
                  else if (sz == 2) { R[AX] = (R[AX] & ~0xFFFFu) | (p & 0xFFFF); R[DX] = (R[DX] & ~0xFFFFu) | (((uint64_t)p >> 16) & 0xFFFF); CF = OF = p != (int16_t)p; }
                  else { R[AX] = (R[AX] & ~0xFFFFu) | (p & 0xFFFF); CF = OF = p != (int8_t)p; } break; }
        case 6: { uint64_t n; uint32_t d = rdop(o, sz), q, r;
                  if (!d) die("divide by zero");
                  if (sz == 4) n = ((uint64_t)R[DX] << 32) | R[AX]; else if (sz == 2) n = ((uint64_t)(R[DX] & 0xFFFF) << 16) | (R[AX] & 0xFFFF); else n = R[AX] & 0xFFFF;
                  if (n / d > mask(sz)) die("div overflow");
                  q = (uint32_t)(n / d); r = (uint32_t)(n % d);
                  if (sz == 4) { R[AX] = q; R[DX] = r; } else if (sz == 2) { setreg(AX, q, 2); setreg(DX, r, 2); } else { R[AX] = (R[AX] & ~0xFFFFu) | (q & 0xFF) | ((r & 0xFF) << 8); } break; }
        case 7: { int64_t n; int32_t d = sx(rdop(o, sz), sz); int64_t q, r;
                  if (!d) die("divide by zero");
                  if (sz == 4) n = (int64_t)(((uint64_t)R[DX] << 32) | R[AX]); else if (sz == 2) n = (int32_t)(((uint32_t)(R[DX] & 0xFFFF) << 16) | (R[AX] & 0xFFFF)); else n = (int16_t)(R[AX] & 0xFFFF);
                  q = n / d; r = n % d;
                  if (q > (int64_t)(sbit(sz) - 1) || q < -(int64_t)sbit(sz)) die("idiv overflow");
                  if (sz == 4) { R[AX] = (uint32_t)q; R[DX] = (uint32_t)r; } else if (sz == 2) { setreg(AX, (uint32_t)q, 2); setreg(DX, (uint32_t)r, 2); } else { R[AX] = (R[AX] & ~0xFFFFu) | ((uint32_t)q & 0xFF) | (((uint32_t)r & 0xFF) << 8); } break; }
        default: die("F6/F7 group");
        }
        return;
    }
    if (op == 0xFE || op == 0xFF) {
        sz = op == 0xFE ? 1 : osz; o = modrm();
        switch (reg_) {
        case 0: case 1: { uint32_t c = CF, v = alu(reg_ ? 5 : 0, rdop(o, sz), 1, sz); CF = c; wrop(o, v, sz); break; }
        case 2: { uint32_t t = rdop(o, osz); push(ip, osz); ip = t & 0xFFFF; break; }
        case 4: ip = rdop(o, osz) & 0xFFFF; break;
        case 6: push(rdop(o, osz), osz); break;
        default: die("FF group");
        }
        return;
    }
    if (op == 0x0F) {
        int op2 = f8();
        if (op2 >= 0x80 && op2 <= 0x8F) { int32_t d = osz == 4 ? (int32_t)f32() : (int16_t)f16(); if (cond(op2 & 15)) ip = (ip + d) & 0xFFFF; return; }
        if (op2 >= 0x90 && op2 <= 0x9F) { o = modrm(); wrop(o, cond(op2 & 15), 1); return; }
        if (op2 == 0xAF) { int64_t p; o = modrm(); p = (int64_t)sx(getreg(reg_, osz), osz) * sx(rdop(o, osz), osz); setreg(reg_, (uint32_t)p, osz); CF = OF = p != sx((uint32_t)p, osz); return; }
        if (op2 == 0xB6 || op2 == 0xB7) { o = modrm(); setreg(reg_, rdop(o, op2 == 0xB6 ? 1 : 2), osz); return; }
        if (op2 == 0xBE || op2 == 0xBF) { o = modrm(); setreg(reg_, (uint32_t)sx(rdop(o, op2 == 0xBE ? 1 : 2), op2 == 0xBE ? 1 : 2), osz); return; }
        if (op2 == 0xA3 || op2 == 0xBA) {
            int32_t bit; o = modrm();
            if (op2 == 0xBA) { bit = f8(); if (reg_ != 4) die("0F BA group"); } else bit = (int32_t)getreg(reg_, osz);
            if (!o.is_reg) { if (op2 == 0xA3) o.off += (uint32_t)((bit >> (osz == 4 ? 5 : 4)) * osz); bit &= (osz * 8 - 1); }
            else bit &= (osz * 8 - 1);
            CF = (rdop(o, osz) >> bit) & 1; return;
        }
        die("unimplemented 0F opcode");
    }
    { char m[64]; sprintf(m, "unimplemented opcode %02X", op); die(m); }
}

int main(int argc, char **argv)
{
    FILE *f; long n; int i;
    if (argc < 2) { fprintf(stderr, "usage\n"); return 1; }
    for (i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "-n")) maxframes = atol(argv[++i]);
        else if (!strcmp(argv[i], "-d")) dumpf[ndump++] = atol(argv[++i]);
        else if (!strcmp(argv[i], "-o")) prefix = argv[++i];
        else if (!strcmp(argv[i], "-x")) memdump = argv[++i];
        else if (!strcmp(argv[i], "-k")) {
            char *c = strchr(argv[++i], ':'); long fr = atol(argv[i]); const char *k = c + 1; int code;
            if (!strcmp(k, "enter")) code = 0x1C0D; else if (!strcmp(k, "esc")) code = 0x011B; else if (!strcmp(k, "tab")) code = 0x0F09;
            else if (!strcmp(k, "up")) code = 0x4800; else if (!strcmp(k, "down")) code = 0x5000;
            else if (!strcmp(k, "left")) code = 0x4B00; else if (!strcmp(k, "right")) code = 0x4D00; else code = k[0];
            keys[nkeys].frame = fr; keys[nkeys++].key = code;
        }
    }
    f = fopen(argv[1], "rb"); if (!f) { perror("open"); return 1; }
    n = fread(mem + 0x10000 + 0x100, 1, 0xFE00, f); fclose(f);
    S[CS] = S[DS] = S[ES] = S[SS] = 0x1000; ip = 0x100; R[SP] = 0xFFFE;
    mem[0x10000 + 2] = 0x00; mem[0x10000 + 3] = 0xA0;         /* PSP top of memory = A000 */
    /* poison the area the program must not rely on being zero */
    for (i = 0x10000 + 0x100 + (int)n; i < 0x20000; i++) mem[i] = 0xA5;
    for (i = 0x20000; i < 0x2FA00; i++) mem[i] = 0x5A;
    R[AX] = 0x1234; R[BX] = 0x00FF0000; R[CX] = 0x7777FFFF; R[DX] = 0x0; R[SI] = 0xDEAD0000; R[DI] = 0xBEEF0000;   /* junk high halves */
    while (!exited) { step(); icount++; if ((R[SP] >> 16) != 0) die("ESP upper half became non zero"); }
    if (memdump) { FILE *o = fopen(memdump, "wb"); fwrite(mem + 0x10000, 1, 0x10000, o); fclose(o); }
    return 0;
}
