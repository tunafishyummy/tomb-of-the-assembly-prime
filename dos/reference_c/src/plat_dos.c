/* DOS platform: VGA mode 13h, BIOS keyboard, vertical retrace sync.
   Built with gcc -m16 as a flat .COM file. All hardware access is inline asm. */
#include "platform.h"
#include "types16.h"

static unsigned short backseg;      /* segment of the 64000 byte back buffer */

static unsigned short get_cs(void) { unsigned short s; __asm__ volatile("movw %%cs,%0" : "=r"(s)); return s; }

void plat_fill(unsigned off, unsigned n, int c)
{
    __asm__ volatile("pushw %%es\n\tmovw %w3,%%es\n\tcld\n\trep stosb\n\tpopw %%es"
                     : "+D"(off), "+c"(n) : "a"(c), "r"(backseg) : "memory", "cc");
}

void plat_copy(unsigned off, const unsigned char *src, unsigned n)
{
    __asm__ volatile("pushw %%es\n\tmovw %w3,%%es\n\tcld\n\trep movsb\n\tpopw %%es"
                     : "+D"(off), "+c"(n), "+S"(src) : "r"(backseg) : "memory", "cc");
}

void plat_present(void)
{
    __asm__ volatile(
        "pushw %%ds\n\tpushw %%es\n\t"
        "movw %w0,%%ds\n\tmovw $0xA000,%%ax\n\tmovw %%ax,%%es\n\t"
        "xorl %%esi,%%esi\n\txorl %%edi,%%edi\n\tmovl $16000,%%ecx\n\tcld\n\t"
        "rep movsl\n\t"
        "popw %%es\n\tpopw %%ds"
        : : "r"(backseg) : "eax", "ecx", "esi", "edi", "memory", "cc");
}

void plat_wait_frame(void)
{
    unsigned char v;
    do { __asm__ volatile("inb %%dx,%%al" : "=a"(v) : "d"(0x3DA)); } while (v & 8);   /* leave retrace */
    do { __asm__ volatile("inb %%dx,%%al" : "=a"(v) : "d"(0x3DA)); } while (!(v & 8)); /* wait for next */
}

void plat_init(const unsigned char *pal6, int n)
{
    unsigned top;
    unsigned short cs = get_cs();
    backseg = (unsigned short)(cs + 0x1000u);          /* the 64K right behind this program */
    { unsigned short v; __asm__ volatile("movw 2,%0" : "=r"(v)); top = v; }   /* PSP:0002 = first segment past our memory block */
    if (top < (unsigned)backseg + 0xFA0u) {
        __asm__ volatile("movw $0x0900,%%ax\n\tint $0x21" : : "d"("NOT ENOUGH MEMORY\r\n$") : "eax", "memory");
        __asm__ volatile("movw $0x4C01,%%ax\n\tint $0x21" : : : "eax");
    }
    __asm__ volatile("movw $0x0013,%%ax\n\tint $0x10" : : : "eax", "memory");               /* mode 13h */
    __asm__ volatile("movw $0x1012,%%ax\n\txorl %%ebx,%%ebx\n\tint $0x10"                    /* set DAC block */
                     : : "c"(n), "d"(pal6) : "eax", "ebx", "memory");
    plat_fill(0, 64000u, 0);
}

void plat_quit(void)
{
    __asm__ volatile("movw $0x0003,%%ax\n\tint $0x10" : : : "eax", "memory");
}

unsigned plat_ticks(void)
{
    unsigned hi, lo;
    __asm__ volatile("xorl %%eax,%%eax\n\tint $0x1A" : "=c"(hi), "=d"(lo) : : "eax", "memory");
    return ((hi & 0xFFFFu) << 16) | (lo & 0xFFFFu);
}

static int key_hit(void)
{
    int r;
    __asm__ volatile("movw $0x0100,%%ax\n\tint $0x16\n\tsetnz %%al\n\tandl $0xFF,%%eax" : "=a"(r) : : "memory", "cc");
    return r;
}

static int key_read(void)
{
    int ax;
    __asm__ volatile("xorl %%eax,%%eax\n\tint $0x16\n\tandl $0xFFFF,%%eax" : "=a"(ax) : : "memory", "cc");
    {
        int al = ax & 0xFF, ah = (ax >> 8) & 0xFF;
        if (al == 0 || al == 0xE0) {
            switch (ah) {
            case 0x48: return K_UP;
            case 0x50: return K_DOWN;
            case 0x4B: return K_LEFT;
            case 0x4D: return K_RIGHT;
            default:   return K_NONE;
            }
        }
        return al;
    }
}

int plat_key(void)
{
    if (!key_hit()) return K_NONE;
    return key_read();
}

void plat_flush_keys(void) { while (key_hit()) (void)key_read(); }

/* gcc may emit calls to these even with -ffreestanding */
void *memcpy(void *d, const void *s, unsigned n)
{
    void *r = d;
    __asm__ volatile("cld\n\trep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory", "cc");
    return r;
}
void *memset(void *d, int c, unsigned n)
{
    void *r = d;
    __asm__ volatile("cld\n\trep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory", "cc");
    return r;
}
