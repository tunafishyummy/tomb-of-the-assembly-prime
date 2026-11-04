/* Tomb of the Assembly - DOS edition: game flow, menus, camera, transitions */
#include "gfx.h"

static Game g;
static Room next_room;
static unsigned seed = 44257u;
static int speed_idx = 1;
static const int speed_px[3] = {6, 10, 16};
static const char *const speed_name[3] = {"SLOW  ", "NORMAL", "FAST  "};
static int frame;
static int cam_x, cam_y;
static int lava_y, lava_clock;

/* ------------------------------------------------------------------ helpers */
static int is_up(int k)    { return k == K_UP || k == 'w' || k == 'W'; }
static int is_down(int k)  { return k == K_DOWN || k == 's' || k == 'S'; }
static int is_left(int k)  { return k == K_LEFT || k == 'a' || k == 'A'; }
static int is_right(int k) { return k == K_RIGHT || k == 'd' || k == 'D'; }

/* One frame boundary: wait for the vertical retrace, then flip whatever was drawn since the
   last frame (nothing drawn = nothing copied). Presenting right at the retrace avoids tearing. */
static void tick(void)
{
    plat_wait_frame();
    if (gfx_dirty) { plat_present(); gfx_dirty = 0; }
    frame++;
}

static int last_valid;                          /* 0 = next draw_world repaints everything */
static void invalidate(void) { last_valid = 0; }

static int wait_key(void)
{
    int k;
    plat_flush_keys();
    for (;;) {
        tick();
        k = plat_key();
        if (k != K_NONE) { seed = seed * 31u + (unsigned)k + plat_ticks(); return k; }
    }
}

static int clamp_cam(int p, int map_px, int view_px)
{
    int max = map_px - view_px;
    if (max <= 0) return max / 2;           /* small map: centre it */
    if (p < 0) return 0;
    if (p > max) return max;
    return p;
}

static void cam_target(const Game *gm, int *tx, int *ty)
{
    int px = gm->px * TILE + gm->dx * gm->prog + TILE / 2;
    int py = gm->py * TILE + gm->dy * gm->prog + TILE / 2;
    *tx = clamp_cam(px - VIEW_W / 2, gm->room.w * TILE, VIEW_W);
    *ty = clamp_cam(py - VIEW_H / 2, gm->room.h * TILE, VIEW_H);
}

static int ease(int cur, int target)
{
    int d = target - cur;
    if (d == 0) return cur;
    if (d > -4 && d < 4) return target;
    return cur + d / 3;
}

static void draw_world(const Game *gm)
{
    static int lcx, lcy, lpx, lpy, la, lb, lhud; static const char *lmsg;
    int px = gm->px * TILE + gm->dx * gm->prog, py = gm->py * TILE + gm->dy * gm->prog;
    int a = (frame / 10) & 1, b = (frame / 14) & 1;
    int hud = gm->coins | (gm->keys << 10) | (gm->bombs << 14) | (gm->shield << 18) | ((gm->msg_t > 0) << 19);
    if (!last_valid || cam_x != lcx || cam_y != lcy || px != lpx || py != lpy || a != la || b != lb) {
        gfx_clear_view(P_BLACK);
        gfx_room(&gm->room, -cam_x, -cam_y, frame, 1);
        if (gm->mode == MODE_ENDLESS && lava_y >= 0 && lava_y < gm->room.h)
            gfx_rect(0, lava_y * TILE - cam_y, SCR_W, 6, P_RED);
        gfx_player(px - cam_x, py - cam_y, gm->dx, gm->dy, frame);
        lcx = cam_x; lcy = cam_y; lpx = px; lpy = py; la = a; lb = b;
    }
    if (!last_valid || hud != lhud || gm->msg != lmsg) {
        gfx_hud(gm);
        lhud = hud; lmsg = gm->msg;
    }
    last_valid = 1;
}

/* ------------------------------------------------------------------- screens */
static void title_bg(void)
{
    int x, y;
    gfx_clear(P_BLACK);
    for (y = 0; y < 2; y++)
        for (x = 0; x < SCR_W / TILE; x++) {
            gfx_sprite_tile(T_WALL, x * TILE, y * TILE - 8);
        }
    for (x = 0; x < SCR_W / TILE; x++) gfx_sprite_tile(T_WALL, x * TILE, SCR_H - 24);
    for (x = 0; x < SCR_W / TILE; x++) gfx_sprite_tile(T_WALL, x * TILE, SCR_H - 8);
}

static int menu_screen(void)       /* 0 normal, 1 endless, 2 settings, -1 quit */
{
    static const char *const items[3] = {"NORMAL MODE", "ENDLESS MODE", "SETTINGS"};
    int sel = 0, k, redraw = 1;
    plat_flush_keys();
    for (;;) {
        int i;
        if (redraw) {
        redraw = 0;
        title_bg();
        gfx_text_c(30, "TOMB OF", P_YELLOW, 3);
        gfx_text_c(56, "THE MASK", P_YELLOW, 3);
        gfx_text_c(86, "DOS EDITION - GROUP 1 FINAL PROJECT", P_WALL_HI, 1);
        for (i = 0; i < 3; i++) {
            int c = i == sel ? P_WHITE : P_GREY;
            gfx_text_c(102 + i * 18, items[i], c, 2);
            if (i == sel) {
                gfx_text((SCR_W - gfx_text_w(items[i], 2)) / 2 - 16, 102 + i * 18, ">", P_YELLOW, 2);
            }
        }
        gfx_text_c(162, "ARROWS/WASD MOVE   ENTER SELECT   ESC QUIT", P_GREY, 1);
        gfx_sprite_tile('z', 24, 96);
        }
        tick();
        k = plat_key();
        if (k == K_NONE) continue;
        redraw = 1;
        seed = seed * 31u + (unsigned)k + plat_ticks();
        if (is_up(k)) sel = (sel + 2) % 3;
        else if (is_down(k)) sel = (sel + 1) % 3;
        else if (k == K_ENTER) return sel;
        else if (k == K_ESC) return -1;
    }
}

static void settings_screen(void)
{
    int sel = 0, k, redraw = 1;
    const char *note = "";
    plat_flush_keys();
    for (;;) {
        if (redraw) {
        redraw = 0;
        title_bg();
        gfx_text_c(20, "SETTINGS", P_YELLOW, 3);
        gfx_text(40, 70, sel == 0 ? "> DASH SPEED" : "  DASH SPEED", sel == 0 ? P_WHITE : P_GREY, 2);
        gfx_text(230, 70, speed_name[speed_idx], P_YELLOW, 1);
        gfx_text(40, 90, sel == 1 ? "> SOUND FX" : "  SOUND FX", sel == 1 ? P_WHITE : P_GREY, 2);
        gfx_text(230, 94, "RESERVED", P_GREY_LO, 1);
        gfx_text(40, 110, sel == 2 ? "> GAME MUSIC" : "  GAME MUSIC", sel == 2 ? P_WHITE : P_GREY, 2);
        gfx_text(230, 114, "RESERVED", P_GREY_LO, 1);
        gfx_text(40, 130, sel == 3 ? "> BACK" : "  BACK", sel == 3 ? P_WHITE : P_GREY, 2);
        gfx_text_c(160, note, P_AQUA, 1);
        gfx_text_c(162, "UP/DOWN MOVE  LEFT/RIGHT CHANGE  ENTER BACK", P_GREY, 1);
        }
        tick();
        k = plat_key();
        if (k == K_NONE) continue;
        redraw = 1;
        note = "";
        if (is_up(k)) sel = (sel + 3) % 4;
        else if (is_down(k)) sel = (sel + 1) % 4;
        else if (k == K_ESC) return;
        else if (is_left(k) && sel == 0) speed_idx = (speed_idx + 2) % 3;
        else if (is_right(k) && sel == 0) speed_idx = (speed_idx + 1) % 3;
        else if (k == K_ENTER) {
            if (sel == 0) speed_idx = (speed_idx + 1) % 3;
            else if (sel == 3) return;
            else note = "SOUND IS RESERVED FOR A FUTURE UPDATE";
        }
    }
}

static void help_screen(void)
{
    static const struct { char ch; const char *name; } rows[] = {
        {'z', "PLAYER"}, {T_COIN, "COIN"}, {T_TRAP, "TRAP - DEADLY"}, {T_WALL, "WALL"},
        {T_CRATE, "CRATE - PUSH IT"}, {T_KEY, "KEY"}, {T_DOOR, "DOOR - NEEDS KEY"}, {T_PLATE, "PLATE - OPENS GATES"},
        {T_GATE, "GATE"}, {T_LEVER, "LEVER - TOGGLES WALLS"}, {T_BOMB, "BOMB (B)"}, {T_SHIELD, "SHIELD"},
        {T_BREAK, "WEAK WALL - BOMB IT"}, {T_TOGGLE, "LEVER WALL"}
    };
    int i;
    gfx_clear(P_BLACK);
    gfx_text_c(4, "ICON LEGEND", P_YELLOW, 2);
    for (i = 0; i < 14; i++) {
        int col = i / 7, row = i % 7, x = 6 + col * 160, y = 24 + row * 22;
        gfx_sprite_tile(rows[i].ch, x, y);
        gfx_text(x + 22, y + 4, rows[i].name, P_WHITE, 1);
    }
    gfx_text_c(186, "ANY KEY TO RETURN", P_GREY, 1);
    plat_present();
    (void)wait_key();
}

static void end_screen(const char *title, int rooms, int coins)
{
    char b[8]; int n, i;
    gfx_clear(P_BLACK);
    gfx_text_c(40, title, P_YELLOW, 3);
    if (rooms >= 0) {
        gfx_text(80, 90, "ROOMS CLEARED:", P_WHITE, 1);
        gfx_num(190, 90, rooms, 3, P_YELLOW, 1);
    }
    gfx_text(80, 104, "COINS:", P_WHITE, 1);
    n = coins; for (i = 2; i >= 0; i--) { b[i] = (char)('0' + n % 10); n /= 10; } b[3] = 0;
    gfx_text(190, 104, b, P_YELLOW, 1);
    gfx_text_c(150, "PRESS ENTER", P_GREY, 1);
    for (;;) { if (wait_key() == K_ENTER) return; }
}

/* ------------------------------------------------------------- transitions */
static void slide_transition(Game *gm, const Room *nr, int dirx, int diry)
{
    /* the old room slides out while the new one slides in (pure blits, no
       per-cell redraws: this is what made the simulator version so slow) */
    int p, total = (dirx ? VIEW_W : VIEW_H), step = 8;
    int ncx, ncy;
    int nsx = nr->sx * TILE, nsy = nr->sy * TILE;
    ncx = clamp_cam(nsx + TILE / 2 - VIEW_W / 2, nr->w * TILE, VIEW_W);
    ncy = clamp_cam(nsy + TILE / 2 - VIEW_H / 2, nr->h * TILE, VIEW_H);
    for (p = step; p <= total + step - 1; p += step) {
        int q = p > total ? total : p;
        int oox = -cam_x - dirx * q, ooy = -cam_y - diry * q;
        int nox = -ncx + dirx * (total - q), noy = -ncy + diry * (total - q);
        gfx_clear(P_BLACK);
        gfx_room(&gm->room, oox, ooy, frame, 0);
        gfx_player(gm->px * TILE + oox, gm->py * TILE + ooy, 0, 0, frame);
        gfx_room(nr, nox, noy, frame, 1);
        gfx_player(nsx + nox, nsy + noy, 0, 0, frame);
        gfx_hud(gm);
        tick();
    }
    cam_x = ncx; cam_y = ncy;
}

static void pause_frames(int n)
{
    while (n-- > 0) {
        draw_world(&g);
        tick();
        if (g.msg_t > 0) g.msg_t--;
    }
}

/* ------------------------------------------------------------------- play */
static void setup_room_camera(void)
{
    cam_target(&g, &cam_x, &cam_y);
}

/* returns 0 = back to menu */
static void play(void)
{
    int k;
    plat_flush_keys();
    setup_room_camera(); invalidate();
    for (;;) {
        int ev = 0, tx, ty;
        if (!game_dashing(&g)) {
            /* idle: read at most one command, ignore stacked key repeats */
            k = plat_key();
            while (plat_key() != K_NONE) { }
            if (k != K_NONE) {
                seed = seed * 31u + (unsigned)k + plat_ticks();
                if (is_up(k)) game_dash(&g, 0, -1);
                else if (is_down(k)) game_dash(&g, 0, 1);
                else if (is_left(k)) game_dash(&g, -1, 0);
                else if (is_right(k)) game_dash(&g, 1, 0);
                else if (k == 'b' || k == 'B') { game_bomb(&g); invalidate(); }
                else if (k == 'r' || k == 'R' || k == '`') { game_restart(&g); invalidate(); }
                else if (k == 'h' || k == 'H' || k == K_TAB) { help_screen(); plat_flush_keys(); invalidate(); }
                else if (k == 'm' || k == 'M' || k == K_ESC) return;
            }
        }
        if (g.mode == MODE_ENDLESS) {
            if (++lava_clock >= 70) { lava_clock = 0; lava_y--; invalidate(); }
        }
        if (game_dashing(&g)) ev = game_tick(&g, speed_px[speed_idx]);
        if (g.mode == MODE_ENDLESS && lava_y >= 0 && lava_y < g.room.h && g.py >= lava_y)
            ev |= EV_DEAD;
        if (g.msg_t > 0) g.msg_t--;

        cam_target(&g, &tx, &ty);
        cam_x = ease(cam_x, tx); cam_y = ease(cam_y, ty);
        draw_world(&g);
        tick();

        if (ev & EV_DEAD) {
            g.msg = "STAGE FAILED"; g.msg_t = 60; invalidate();
            pause_frames(60);
            if (g.mode == MODE_ENDLESS) { end_screen("GAME OVER", g.depth - 1, g.coins); return; }
            game_restart(&g); setup_room_camera(); invalidate();
            plat_flush_keys();
        }
        if (ev & EV_CLEAR) {
            int dirx = 0, diry = -1;
            g.msg = "LEVEL CLEAR!"; g.msg_t = 100;
            draw_world(&g); tick();
            if (g.room.ex == 0) { dirx = -1; diry = 0; }
            else if (g.room.ex == g.room.w - 1) { dirx = 1; diry = 0; }
            if (g.mode == MODE_NORMAL) {
                g.level++;
                if (g.level >= NUM_LEVELS) { pause_frames(30); end_screen("YOU WIN!", -1, g.coins); return; }
                gen_room(&next_room, &seed, g.level + 1);
            } else {
                g.depth++;
                gen_room(&next_room, &seed, g.depth);   /* hidden inside the pause */
            }
            pause_frames(25);
            slide_transition(&g, &next_room, dirx, diry);
            g.room = next_room;
            if (g.mode == MODE_ENDLESS) { lava_y = g.room.h + 4; lava_clock = 0; }
            game_enter_room(&g);
            setup_room_camera(); invalidate();
            plat_flush_keys();
        }
    }
}

static void start_game(int mode)
{
    g.mode = mode; g.coins = 0; g.level = 0; g.depth = 1;
    /* Keep normal stages in the same chunky tomb-maze style as endless mode. */
    gen_room(&g.room, &seed, 1);
    lava_y = g.room.h + 4; lava_clock = 0;
    game_enter_room(&g);
    play();
}

int game_main(void)
{
    gfx_init();
    for (;;) {
        int m = menu_screen();
        if (m < 0) break;
        if (m == 0) start_game(MODE_NORMAL);
        else if (m == 1) start_game(MODE_ENDLESS);
        else settings_screen();
    }
    plat_quit();
    return 0;
}
