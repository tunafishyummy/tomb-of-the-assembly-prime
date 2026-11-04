#include "engine.h"
#include "levels.h"

static void msg(Game *g, const char *m) { g->msg = m; g->msg_t = 150; }

void room_load_level(Room *r, int idx)
{
    int i, x, y;
    r->w = LEVEL_W; r->h = LEVEL_H;
    for (i = 0; i < LEVEL_W * LEVEL_H; i++) r->c[i] = (unsigned char)level_data[idx][i];
    r->sx = level_start[idx] % LEVEL_W; r->sy = level_start[idx] / LEVEL_W;
    r->ex = level_exit[idx] % LEVEL_W;  r->ey = level_exit[idx] / LEVEL_W;
    r->nflip = level_flip[idx] ? 1 : 0;
    r->flip[0] = level_flip[idx];
    (void)x; (void)y;
}

void game_enter_room(Game *g)
{
    g->orig = g->room;
    g->px = g->room.sx; g->py = g->room.sy;
    g->dx = g->dy = g->prog = g->to_exit = 0;
    g->keys = g->bombs = g->shield = 0;
    g->coin_rec = g->coins;
    g->msg = ""; g->msg_t = 0;
}

void game_restart(Game *g)
{
    g->room = g->orig;
    g->px = g->room.sx; g->py = g->room.sy;
    g->dx = g->dy = g->prog = g->to_exit = 0;
    g->keys = g->bombs = g->shield = 0;
    g->coins = g->coin_rec;
    g->msg = ""; g->msg_t = 0;
}

int game_dash(Game *g, int dx, int dy)
{
    if (game_dashing(g)) return 0;
    g->dx = dx; g->dy = dy; g->prog = 0; g->to_exit = 0;
    return 1;
}

static void open_gates(Room *r)
{
    int i;
    for (i = 0; i < r->w * r->h; i++) if (r->c[i] == T_GATE) r->c[i] = T_FLOOR;
}

static void flip_layout(Room *r)
{
    int i;
    for (i = 0; i < r->nflip; i++) {
        int f = r->flip[i];
        if (!f) continue;
        r->c[f] = (r->c[f] == T_TOGGLE) ? T_FLOOR : T_TOGGLE;
    }
}

/* Decide what happens when the dash tries to enter the next tile.
   returns 0 = dash stops, 1 = tile entered, 2 = dead. */
static int begin_step(Game *g, int *ev)
{
    Room *r = &g->room;
    int nx = g->px + g->dx, ny = g->py + g->dy;
    int idx, c;

    if (nx == r->ex && ny == r->ey) { g->to_exit = 1; return 1; }
    if (nx < 0 || ny < 0 || nx >= r->w || ny >= r->h) return 0;
    idx = ny * r->w + nx;
    c = r->c[idx];

    switch (c) {
    case T_FLOOR: return 1;
    case T_COIN:  g->coins++; *ev |= EV_COIN; r->c[idx] = T_FLOOR; return 1;
    case T_CRATE: {
        int bx = nx + g->dx, by = ny + g->dy, bc;
        if (bx < 0 || by < 0 || bx >= r->w || by >= r->h) return 0;
        bc = r->c[by * r->w + bx];
        if (bc != T_FLOOR && bc != T_TRAP && bc != T_PLATE) return 0;
        r->c[by * r->w + bx] = T_CRATE;
        if (bc == T_PLATE) open_gates(r);
        r->c[idx] = T_FLOOR;
        return 1;
    }
    case T_KEY:   g->keys++; *ev |= EV_ITEM; r->c[idx] = T_FLOOR; return 1;
    case T_BOMB:  g->bombs++; *ev |= EV_ITEM; r->c[idx] = T_FLOOR; return 1;
    case T_SHIELD:
        *ev |= EV_ITEM;
        if (g->shield) { g->coins += 5; msg(g, "SHIELD ALREADY ACTIVE +5 COINS"); }
        else g->shield = 1;
        r->c[idx] = T_FLOOR; return 1;
    case T_LEVER: flip_layout(r); r->c[idx] = T_FLOOR; *ev |= EV_ITEM; return 1;
    case T_DOOR:
        if (g->keys > 0) { g->keys--; r->c[idx] = T_FLOOR; *ev |= EV_ITEM; }
        return 0;                       /* opening a door always ends the dash */
    case T_PLATE: open_gates(r); r->c[idx] = T_FLOOR; *ev |= EV_ITEM; return 1;
    case T_TRAP:
        if (!g->shield) { msg(g, "TRAP! RESTARTING THE LEVEL"); return 2; }
        g->shield = 0; r->c[idx] = T_FLOOR; *ev |= EV_ITEM; return 1;
    default:      return 0;             /* walls, gates, unknown: solid */
    }
}

int game_tick(Game *g, int pixels)
{
    int ev = 0;
    while (pixels > 0 && game_dashing(g)) {
        int step;
        if (g->prog == 0) {
            int r = begin_step(g, &ev);
            if (r == 0) { g->dx = g->dy = 0; break; }
            if (r == 2) { g->dx = g->dy = 0; ev |= EV_DEAD; break; }
        }
        step = 16 - g->prog;
        if (step > pixels) step = pixels;
        g->prog += step; pixels -= step;
        if (g->prog >= 16) {
            g->px += g->dx; g->py += g->dy; g->prog = 0;
            if (g->to_exit) { g->dx = g->dy = 0; g->to_exit = 0; ev |= EV_CLEAR; break; }
        }
    }
    return ev;
}

int game_bomb(Game *g)
{
    static const int ox[4] = {0, 0, -1, 1}, oy[4] = {-1, 1, 0, 0};
    int i, n = 0;
    if (g->bombs <= 0) { msg(g, "NO BOMBS LEFT"); return 0; }
    for (i = 0; i < 4; i++) {
        int x = g->px + ox[i], y = g->py + oy[i];
        if (x < 0 || y < 0 || x >= g->room.w || y >= g->room.h) continue;
        if (CELL(&g->room, x, y) == T_BREAK) { CELL(&g->room, x, y) = T_FLOOR; n++; }
    }
    if (!n) { msg(g, "NOTHING TO BREAK"); return 0; }
    g->bombs--;
    return 1;
}
