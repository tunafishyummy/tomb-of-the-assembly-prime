#include "gfx.h"
#include "assets.h"

/* ---- clipped blits into the 320x200 back buffer -------------------------- */
int gfx_dirty;                                       /* set by every drawing call, cleared when presented */
static int clip_x0, clip_y0, clip_x1, clip_y1;       /* active clip window [x0,x1) [y0,y1) */

static void clip_all(void) { clip_x0 = 0; clip_y0 = 0; clip_x1 = SCR_W; clip_y1 = SCR_H; }
static void clip_view(void) { clip_x0 = 0; clip_y0 = 0; clip_x1 = VIEW_W; clip_y1 = VIEW_H; }

void gfx_clear(int c) { gfx_dirty = 1; plat_fill(0, 64000u, c); }
void gfx_clear_view(int c) { gfx_dirty = 1; plat_fill(0, (unsigned)VIEW_H * SCR_W, c); }

void gfx_rect(int x, int y, int w, int h, int c)
{
    int yy;
    gfx_dirty = 1;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (w <= 0) return;
    for (yy = y; yy < y + h; yy++)
        if (yy >= 0 && yy < SCR_H) plat_fill((unsigned)(yy * SCR_W + x), (unsigned)w, c);
}

static void blit(const unsigned char *t, int x, int y, int keyed)
{
    int row, x0 = 0, x1 = 16;
    gfx_dirty = 1;
    if (x < clip_x0) x0 = clip_x0 - x;
    if (x + 16 > clip_x1) x1 = clip_x1 - x;
    if (x0 >= x1) return;
    for (row = 0; row < 16; row++) {
        int yy = y + row;
        const unsigned char *s = t + row * 16;
        if (yy < clip_y0 || yy >= clip_y1) continue;
        if (!keyed) {
            plat_copy((unsigned)(yy * SCR_W + x + x0), s + x0, (unsigned)(x1 - x0));
        } else {
            int i = x0;
            while (i < x1) {
                int j;
                if (s[i] == 255) { i++; continue; }
                j = i;
                while (j < x1 && s[j] != 255) j++;
                plat_copy((unsigned)(yy * SCR_W + x + i), s + i, (unsigned)(j - i));
                i = j;
            }
        }
    }
}

/* ---- init ---------------------------------------------------------------- */
void gfx_init(void) { plat_init(&pal6[0][0], P_COUNT); clip_all(); }

/* ---- text (5x7 font in a 6x8 cell) -------------------------------------- */
int gfx_text_w(const char *s, int scale) { int n = 0; while (*s++) n++; return n * 6 * scale; }

static void glyph(int x, int y, int ch, int color, int scale)
{
    int r, c;
    const unsigned char *g;
    gfx_dirty = 1;
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    if (ch < 32 || ch > 95) ch = 32;
    g = font5x7[ch - 32];
    for (r = 0; r < 7; r++) {
        c = 0;
        while (c < 5) {
            if (g[r] & (1 << (4 - c))) {
                int e = c, k;
                while (e < 5 && (g[r] & (1 << (4 - e)))) e++;
                for (k = 0; k < scale; k++) {
                    int yy = y + r * scale + k;
                    if (yy >= 0 && yy < SCR_H && x + c * scale >= 0 && x + e * scale <= SCR_W)
                        plat_fill((unsigned)(yy * SCR_W + x + c * scale), (unsigned)((e - c) * scale), color);
                }
                c = e;
            } else c++;
        }
    }
}

void gfx_text(int x, int y, const char *s, int color, int scale)
{
    for (; *s; s++, x += 6 * scale) glyph(x, y, *s, color, scale);
}

void gfx_text_c(int y, const char *s, int color, int scale)
{
    gfx_text((SCR_W - gfx_text_w(s, scale)) / 2, y, s, color, scale);
}

void gfx_num(int x, int y, int n, int digits, int color, int scale)
{
    char b[8]; int i;
    if (digits > 7) digits = 7;
    for (i = digits - 1; i >= 0; i--) { b[i] = (char)('0' + n % 10); n /= 10; }
    b[digits] = 0;
    gfx_text(x, y, b, color, scale);
}

/* ---- tiles ---------------------------------------------------------------- */
static int solid(const Room *r, int x, int y)
{
    int c;
    if (x < 0 || y < 0 || x >= r->w || y >= r->h) return 1;
    c = r->c[y * r->w + x];
    return c == T_WALL || c == T_TRAP || c == T_BREAK || c == T_TOGGLE;
}

static int open_mask(const Room *r, int x, int y)
{
    return (!solid(r, x, y - 1) ? 1 : 0) | (!solid(r, x, y + 1) ? 2 : 0) |
           (!solid(r, x - 1, y) ? 4 : 0) | (!solid(r, x + 1, y) ? 8 : 0);
}

/* lethal wall: red block, saw teeth on every open side */
static unsigned char trap_tile[256];
static void build_trap(int mask)
{
    int i, x, d;
    for (i = 0; i < 256; i++) trap_tile[i] = P_TRAP;
    for (i = 0; i < 16; i++) {                       /* bright rim */
        if (mask & 1) trap_tile[i] = P_TRAP_SP;
        if (mask & 2) trap_tile[240 + i] = P_TRAP_SP;
        if (mask & 4) trap_tile[i * 16] = P_TRAP_SP;
        if (mask & 8) trap_tile[i * 16 + 15] = P_TRAP_SP;
    }
    for (x = 0; x < 16; x++) {
        int t = x & 7, h = t < 4 ? t + 1 : 8 - t;   /* tooth height 1..4 */
        for (d = 0; d < 4; d++) {
            int lit = d >= 4 - h;                    /* tip points to the open side */
            int c = lit ? P_TRAP_HI : P_FLOOR;
            if (mask & 1) trap_tile[d * 16 + x] = (unsigned char)c;
            if (mask & 2) trap_tile[(15 - d) * 16 + x] = (unsigned char)c;
            if (mask & 4) trap_tile[x * 16 + d] = (unsigned char)c;
            if (mask & 8) trap_tile[x * 16 + 15 - d] = (unsigned char)c;
        }
    }
    trap_tile[7 * 16 + 7] = P_RED_HI; trap_tile[7 * 16 + 8] = P_RED_HI;
    trap_tile[8 * 16 + 7] = P_RED_HI; trap_tile[8 * 16 + 8] = P_RED_HI;
}

static const unsigned char *tile_for(const Room *r, int x, int y, int frame)
{
    int c = r->c[y * r->w + x];
    switch (c) {
    case T_WALL:   return tile_data[TI_WALL0 + open_mask(r, x, y)];
    case T_TRAP:   build_trap(open_mask(r, x, y)); return trap_tile;
    case T_BREAK:  return tile_data[TI_BREAK];
    case T_TOGGLE: return tile_data[TI_TOGGLE];
    case T_GATE:   return tile_data[TI_GATE];
    case T_DOOR:   return tile_data[TI_DOOR];
    case T_PLATE:  return tile_data[TI_PLATE];
    case T_COIN:   return tile_data[((frame / 10) + x + y) & 1 ? TI_COIN1 : TI_COIN0];
    case T_KEY:    return tile_data[TI_KEY];
    case T_LEVER:  return tile_data[TI_LEVER];
    case T_BOMB:   return tile_data[TI_BOMB];
    case T_SHIELD: return tile_data[TI_SHIELD];
    case T_CRATE:  return tile_data[TI_CRATE];
    default:       return tile_data[TI_FLOOR];
    }
}

void gfx_sprite_tile(int ch, int x, int y)
{
    clip_all();
    switch (ch) {
    case T_COIN:   blit(tile_data[TI_COIN0], x, y, 0); break;
    case T_KEY:    blit(tile_data[TI_KEY], x, y, 0); break;
    case T_BOMB:   blit(tile_data[TI_BOMB], x, y, 0); break;
    case T_SHIELD: blit(tile_data[TI_SHIELD], x, y, 0); break;
    case T_CRATE:  blit(tile_data[TI_CRATE], x, y, 0); break;
    case T_DOOR:   blit(tile_data[TI_DOOR], x, y, 0); break;
    case T_PLATE:  blit(tile_data[TI_PLATE], x, y, 0); break;
    case T_GATE:   blit(tile_data[TI_GATE], x, y, 0); break;
    case T_LEVER:  blit(tile_data[TI_LEVER], x, y, 0); break;
    case T_BREAK:  blit(tile_data[TI_BREAK], x, y, 0); break;
    case T_TOGGLE: blit(tile_data[TI_TOGGLE], x, y, 0); break;
    case T_WALL:   blit(tile_data[TI_WALL0], x, y, 0); break;
    case T_TRAP:   build_trap(15); blit(trap_tile, x, y, 0); break;
    case 'z':      blit(tile_data[TI_FLOOR], x, y, 0); blit(player_data, x, y, 1); break;
    default:       blit(tile_data[TI_FLOOR], x, y, 0); break;
    }
}

void gfx_room(const Room *r, int ox, int oy, int frame, int highlight_exit)
{
    int tx0 = (0 - ox) / TILE, ty0 = (0 - oy) / TILE, tx1, ty1, tx, ty;
    if (ox > 0) tx0 = 0;
    if (oy > 0) ty0 = 0;
    if (ox < 0 && (0 - ox) % TILE == 0) { /* exact */ }
    tx1 = (VIEW_W - ox + TILE - 1) / TILE; ty1 = (VIEW_H - oy + TILE - 1) / TILE;
    if (tx1 > r->w) tx1 = r->w;
    if (ty1 > r->h) ty1 = r->h;
    clip_view();
    for (ty = ty0; ty < ty1; ty++)
        for (tx = tx0; tx < tx1; tx++) {
            int sx = ox + tx * TILE, sy = oy + ty * TILE;
            if (sx + TILE <= 0 || sy + TILE <= 0 || sx >= VIEW_W || sy >= VIEW_H) continue;
            if (highlight_exit && tx == r->ex && ty == r->ey)
                blit(tile_data[((frame / 14) & 1) ? TI_EXIT1 : TI_EXIT0], sx, sy, 0);
            else
                blit(tile_for(r, tx, ty, frame), sx, sy, 0);
        }
    clip_all();
}

void gfx_player(int x, int y, int dx, int dy, int frame)
{
    (void)dx; (void)dy; (void)frame;
    clip_view();
    blit(player_data, x, y, 1);
    clip_all();
}

void gfx_hud(const Game *g)
{
    char lab[16]; int i, n;
    gfx_rect(0, VIEW_H, SCR_W, SCR_H - VIEW_H, P_HUD);
    gfx_rect(0, VIEW_H, SCR_W, 1, P_WALL_HI);
    gfx_sprite_tile(T_COIN, 4, VIEW_H + 3);
    gfx_num(22, VIEW_H + 6, g->coins, 3, P_YELLOW, 2);
    gfx_sprite_tile(T_KEY, 70, VIEW_H + 3);
    gfx_num(88, VIEW_H + 6, g->keys, 1, P_SILVER, 2);
    gfx_sprite_tile(T_BOMB, 112, VIEW_H + 3);
    gfx_num(130, VIEW_H + 6, g->bombs, 1, P_RED_HI, 2);
    if (g->shield) gfx_sprite_tile(T_SHIELD, 154, VIEW_H + 3);
    else gfx_text(160, VIEW_H + 6, "-", P_GREY, 2);
    n = 0;
    if (g->mode == MODE_ENDLESS) { lab[n++] = 'R'; lab[n++] = 'O'; lab[n++] = 'O'; lab[n++] = 'M'; lab[n++] = ' '; }
    else { lab[n++] = 'L'; lab[n++] = 'V'; lab[n++] = 'L'; lab[n++] = ' '; }
    {
        int v = g->mode == MODE_ENDLESS ? g->depth : g->level + 1, d = v >= 100 ? 3 : v >= 10 ? 2 : 1;
        for (i = d - 1; i >= 0; i--) { lab[n + i] = (char)('0' + v % 10); v /= 10; }
        n += d;
    }
    lab[n] = 0;
    gfx_text(SCR_W - 4 - gfx_text_w(lab, 2), VIEW_H + 6, lab, P_WALL_HI, 2);
    if (g->msg_t > 0) gfx_text(4, VIEW_H + 22, g->msg, P_WHITE, 1);
    else gfx_text(4, VIEW_H + 22, "WASD/ARROWS DASH  B BOMB  R RESTART  H HELP  M MENU", P_GREY, 1);
}
