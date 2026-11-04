/* Tiny platform layer. dos build: plat_dos.c (VGA mode 13h, BIOS keyboard).
   host build: plat_host.c (memory framebuffer for automated tests). */
#ifndef PLATFORM_H
#define PLATFORM_H

#define SCR_W 320
#define SCR_H 200

#define K_NONE  0
#define K_UP    1001
#define K_DOWN  1002
#define K_LEFT  1003
#define K_RIGHT 1004
#define K_ENTER 13
#define K_ESC   27
#define K_TAB   9

void     plat_init(const unsigned char *pal6, int ncolors);  /* 6 bit VGA DAC triples */
void     plat_quit(void);
void     plat_wait_frame(void);        /* wait for vertical retrace (70 Hz) */
void     plat_present(void);           /* copy the back buffer to the screen */
int      plat_key(void);               /* next key or K_NONE, never blocks */
void     plat_flush_keys(void);
unsigned plat_ticks(void);             /* BIOS timer ticks, used to seed the rng */

/* back buffer access */
void plat_fill(unsigned off, unsigned n, int color);
void plat_copy(unsigned off, const unsigned char *src, unsigned n);

#endif
