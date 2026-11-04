#ifndef GFX_H
#define GFX_H
#include "engine.h"

#define TILE 16
#define VIEW_W SCR_W
#define VIEW_H 168          /* 10.5 tiles, the HUD lives below */
#include "platform.h"

#include "palette.h"

void gfx_init(void);                              /* build tiles, palette */
void gfx_rect(int x, int y, int w, int h, int c);
void gfx_text(int x, int y, const char *s, int color, int scale);
int  gfx_text_w(const char *s, int scale);
void gfx_text_c(int y, const char *s, int color, int scale);   /* centred */
void gfx_num(int x, int y, int n, int digits, int color, int scale);
void gfx_clear(int c);
void gfx_clear_view(int c);                       /* only the game view, HUD stays */
extern int gfx_dirty;
/* draw a room with its top-left corner at pixel (ox,oy) relative to the view window,
   clipped to the view window. */
void gfx_room(const Room *r, int ox, int oy, int frame, int highlight_exit);
void gfx_player(int x, int y, int dx, int dy, int frame);   /* x,y = view pixel position */
void gfx_hud(const Game *g);
void gfx_sprite_tile(int ch, int x, int y);                  /* any tile at x,y (hud, legend) */
#endif
