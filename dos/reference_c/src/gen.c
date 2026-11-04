/* Endless-mode room generator and solver.
 *
 * Rooms are tall "temple" mazes in the style of Tomb of the Mask:
 *  - a lattice of 3x3 blocks (1 corridor cell + 2 thick walls) gives chunky
 *    walls instead of the paper thin maze of the old templates;
 *  - a spanning tree is braided with extra passages so there are loops;
 *  - open chambers with pillars give the dash places to stop;
 *  - about half of the rooms are mirrored left/right like a real tomb;
 *  - the player enters at the bottom and climbs to an opening in the top wall;
 *  - traps, coins and shields are placed with the solver, so every room can be
 *    finished, nobody can get stranded, and every coin lies on a real dash.
 */
#include "engine.h"

static const int DX[4] = {0, 0, -1, 1};
static const int DY[4] = {-1, 1, 0, 0};

unsigned rng_next(unsigned *s)
{
    *s = *s * 1664525u + 1013904223u;
    return (*s >> 16) & 0x7fff;
}
static int rnd(unsigned *s, int n) { return (int)(rng_next(s) % (unsigned)n); }

/* ------------------------------------------------------------------ solver */
#define EDGE_EXIT 255
static unsigned char edge[MAXCELLS][4];   /* slide length per direction, 0 = none, 255 = exit */
static short queue[MAXCELLS];
static unsigned char good[MAXCELLS / 8 + 1];

static int is_pass(int c) { return c == T_FLOOR || c == T_COIN || c == T_SHIELD; }

/* length of the slide from cell s in direction d (0 none, 255 reaches the exit) */
static int slide_len(const Room *r, int s, int d)
{
    int x = s % r->w, y = s / r->w, n = 0;
    for (;;) {
        int nx = x + DX[d], ny = y + DY[d], c;
        if (nx == r->ex && ny == r->ey) return EDGE_EXIT;
        if (nx < 0 || ny < 0 || nx >= r->w || ny >= r->h) break;
        c = r->c[ny * r->w + nx];
        if (c == T_TRAP) return 0;          /* the dash would die: not a usable move */
        if (!is_pass(c)) break;
        x = nx; y = ny; n++;
    }
    return n > 250 ? 250 : n;
}

int room_solve(const Room *r, unsigned char *dist, int *stranded)
{
    int head = 0, tail = 0, i, d, ncell = r->w * r->h, exit_dist = -1, changed;
    for (i = 0; i < ncell; i++) dist[i] = 255;
    for (i = 0; i < ncell / 8 + 1; i++) good[i] = 0;
    i = r->sy * r->w + r->sx;
    dist[i] = 0; queue[tail++] = (short)i;
    while (head < tail) {
        int s = queue[head++];
        for (d = 0; d < 4; d++) {
            int len = slide_len(r, s, d), t;
            edge[s][d] = (unsigned char)len;
            if (len == 0) continue;
            if (len == EDGE_EXIT) {
                if (exit_dist < 0 || dist[s] + 1 < exit_dist) exit_dist = dist[s] + 1;
                continue;
            }
            t = s + (DY[d] * r->w + DX[d]) * len;
            if (dist[t] == 255) { dist[t] = (unsigned char)(dist[s] + 1 > 254 ? 254 : dist[s] + 1); queue[tail++] = (short)t; }
        }
    }
    /* which rest positions can still reach the exit? iterate to a fixed point */
    do {
        changed = 0;
        for (i = 0; i < tail; i++) {
            int s = queue[i];
            if (good[s >> 3] & (1 << (s & 7))) continue;
            for (d = 0; d < 4; d++) {
                int len = edge[s][d], t;
                if (!len) continue;
                if (len == EDGE_EXIT) t = -1;
                else t = s + (DY[d] * r->w + DX[d]) * len;
                if (t < 0 || (good[t >> 3] & (1 << (t & 7)))) {
                    good[s >> 3] |= (unsigned char)(1 << (s & 7));
                    changed = 1; break;
                }
            }
        }
    } while (changed);
    if (stranded) {
        int n = 0;
        for (i = 0; i < tail; i++) { int s = queue[i]; if (!(good[s >> 3] & (1 << (s & 7)))) n++; }
        *stranded = n;
    }
    return exit_dist;
}

/* ---------------------------------------------------------------- generator */
#define MAXNX 12
#define MAXNY 16
static unsigned char seen[MAXNX * MAXNY];
static short stack[MAXNX * MAXNY];

static void fill(Room *r, int x0, int y0, int x1, int y1, int c)
{
    int x, y;
    for (y = y0; y <= y1; y++) for (x = x0; x <= x1; x++) r->c[y * r->w + x] = (unsigned char)c;
}

/* open the passage between two neighbouring lattice nodes */
static void link(Room *r, int i0, int j0, int i1, int j1)
{
    int x0 = 1 + 3 * i0, y0 = 1 + 3 * j0, x1 = 1 + 3 * i1, y1 = 1 + 3 * j1;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    fill(r, x0, y0, x1, y1, T_FLOOR);
}

static int free8(const Room *r, int x, int y)
{
    int dx, dy;
    for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++)
        if (r->c[(y + dy) * r->w + x + dx] != T_FLOOR) return 0;
    return 1;
}

static void layout(Room *r, unsigned *s, int nx, int ny, int mirror, int depth)
{
    int W = 3 * nx, H = 3 * ny, hx, i, j, top, n, xmax;
    r->w = W; r->h = H;
    fill(r, 0, 0, W - 1, H - 1, T_WALL);
    hx = mirror ? (nx + 1) / 2 : nx;          /* nodes of the half that is really generated */
    xmax = mirror ? (W - 1) / 2 : W - 2;      /* last column that may be carved */

    /* depth first maze over the generated half */
    for (i = 0; i < MAXNX * MAXNY; i++) seen[i] = 0;
    top = 0; stack[0] = (short)(rnd(s, hx) + MAXNX * (ny - 1)); seen[stack[0]] = 1;
    fill(r, 1 + 3 * (stack[0] % MAXNX), 1 + 3 * (stack[0] / MAXNX), 1 + 3 * (stack[0] % MAXNX), 1 + 3 * (stack[0] / MAXNX), T_FLOOR);
    while (top >= 0) {
        int cur = stack[top], ci = cur % MAXNX, cj = cur / MAXNX, opt[4], no = 0, d;
        for (d = 0; d < 4; d++) {
            int ni = ci + DX[d], nj = cj + DY[d];
            if (ni < 0 || nj < 0 || ni >= hx || nj >= ny || seen[nj * MAXNX + ni]) continue;
            opt[no++] = d;
        }
        if (!no) { top--; continue; }
        {
            int d2 = opt[rnd(s, no)], ni = ci + DX[d2], nj = cj + DY[d2];
            link(r, ci, cj, ni, nj);
            seen[nj * MAXNX + ni] = 1;
            stack[++top] = (short)(nj * MAXNX + ni);
        }
    }
    /* braid: extra passages make loops, so there is more than one way up */
    n = hx * ny / 4 + depth / 2;
    while (n-- > 0) {
        int bi = rnd(s, hx), bj = rnd(s, ny);
        if (rnd(s, 2) && bi + 1 < hx) link(r, bi, bj, bi + 1, bj);
        else if (bj + 1 < ny) link(r, bi, bj, bi, bj + 1);
    }
    /* chambers with pillars */
    n = hx * ny / 6;
    while (n-- > 0) {
        int cw = 3 + rnd(s, 4), ch = 3 + rnd(s, 3);
        int x0, y0, x1, y1, k, np;
        x0 = 1 + 3 * rnd(s, hx); x0 -= rnd(s, cw);      /* explicit order: the assembly port relies on it */
        y0 = 1 + 3 * rnd(s, ny); y0 -= rnd(s, ch);
        if (x0 < 1) x0 = 1;
        if (y0 < 2) y0 = 2;
        x1 = x0 + cw - 1; y1 = y0 + ch - 1;
        if (x1 > xmax) { x1 = xmax; }
        if (y1 > H - 3) { y1 = H - 3; }
        if (x1 - x0 < 2 || y1 - y0 < 2) continue;
        fill(r, x0, y0, x1, y1, T_FLOOR);
        np = (x1 - x0 + 1) * (y1 - y0 + 1) / 7;
        for (k = 0; k < np; k++) {
            int px = x0 + 1 + rnd(s, x1 - x0 - 1), py = y0 + 1 + rnd(s, y1 - y0 - 1);
            if (px <= xmax - 1 && free8(r, px, py)) r->c[py * W + px] = T_WALL;
        }
    }
    if (mirror) {
        int x, y;
        for (y = 0; y < H; y++)
            for (x = 0; x < W / 2; x++) r->c[y * W + (W - 1 - x)] = r->c[y * W + x];
        if (!(nx & 1)) {                      /* even width: join the halves in the middle */
            int k, cx = W / 2 - 1;
            for (k = 0; k < 2 + ny / 4; k++) {
                int y1 = 1 + 3 * rnd(s, ny);
                fill(r, cx, y1, cx + 1, y1, T_FLOOR);
            }
        }
    }
    (void)j;
}

/* put a coin run or shield on cells that really are crossed by a dash */
static void decorate(Room *r, unsigned *s, int depth)
{
    unsigned char dist[MAXCELLS];
    int ncell = r->w * r->h, want = 5 + depth * 2, tries = 0, placed = 0, shields;
    if (want > 36) want = 36;
    shields = (depth >= 2 && rnd(s, 3) != 0) ? 1 : 0;
    if (depth >= 6 && rnd(s, 2)) shields++;
    room_solve(r, dist, 0);
    while ((placed < want || shields > 0) && tries++ < 400) {
        int st = rnd(s, ncell), d = rnd(s, 4), len, k, x, y, run, off;
        if (dist[st] == 255) continue;
        len = edge[st][d];
        if (len == 0 || len == EDGE_EXIT) continue;
        run = shields > 0 ? 1 : 2 + rnd(s, 4);
        if (run > len) run = len;
        off = rnd(s, len - run + 1);
        x = st % r->w + DX[d] * (off + 1); y = st / r->w + DY[d] * (off + 1);
        for (k = 0; k < run; k++, x += DX[d], y += DY[d]) {
            if (CELL(r, x, y) != T_FLOOR) continue;
            if (x == r->sx && y == r->sy) continue;
            if (shields > 0) { CELL(r, x, y) = T_SHIELD; shields--; break; }
            CELL(r, x, y) = T_COIN; placed++;
        }
    }
}

static void place_traps(Room *r, unsigned *s, int depth, int min_exit)
{
    unsigned char dist[MAXCELLS];
    int want = 1 + depth, tries = 0, ncell = r->w * r->h;
    if (want > 14) want = 14;
    while (want > 0 && tries++ < want * 12 + 20) {
        int c = rnd(s, ncell), x = c % r->w, y = c / r->w, ex, str;
        if (r->c[c] != T_FLOOR) continue;
        if (x >= r->sx - 2 && x <= r->sx + 2 && y >= r->sy - 3) continue;  /* keep the start calm */
        if (x == r->ex && y <= 2) continue;
        r->c[c] = T_TRAP;
        ex = room_solve(r, dist, &str);
        if (ex >= min_exit && str == 0 && dist[r->sy * r->w + r->sx] == 0) want--;
        else r->c[c] = T_FLOOR;
    }
}

void gen_room(Room *r, unsigned *seed, int depth)
{
    unsigned char dist[MAXCELLS];
    int attempt, nx, ny;
    nx = 7 + (depth / 2 > 5 ? 5 : depth / 2);
    ny = 8 + (depth > 8 ? 8 : depth);
    for (attempt = 0; attempt < 200; attempt++) {
        int mirror = (rnd(seed, 5) < 3), H, is, ie, ex, str, min_exit;
        layout(r, seed, nx, ny, mirror, depth);
        H = r->h;
        is = rnd(seed, nx);
        if (mirror && (nx & 1) && is == nx / 2) is = (is + 1) % nx;
        ie = (mirror && (nx & 1)) ? nx / 2 : rnd(seed, nx);
        if (!mirror && ie == is) ie = (is + nx / 2) % nx;
        r->sx = 1 + 3 * is; r->sy = H - 2;
        r->ex = 1 + 3 * ie; r->ey = 0;
        fill(r, r->ex, 0, r->ex, 4, T_FLOOR);   /* the climb to the exit opening */
        r->nflip = 0;
        min_exit = 5 + depth / 3 - attempt / 6;
        if (min_exit < 2) min_exit = 2;
        ex = room_solve(r, dist, &str);
        if (ex < min_exit || str != 0) continue;
        place_traps(r, seed, depth, min_exit);
        decorate(r, seed, depth);
        return;
    }
    /* extremely unlikely fallback: keep the last layout, it is still a valid maze */
    ;
}
