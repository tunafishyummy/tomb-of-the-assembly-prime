/* Tomb of the Assembly - DOS edition: game rules (no drawing, no DOS calls) */
#ifndef ENGINE_H
#define ENGINE_H

#define MAXW 40
#define MAXH 52
#define MAXCELLS (MAXW * MAXH)
#define NUM_LEVELS 5

/* tile characters, identical to the ICMC assembly version */
#define T_FLOOR  ' '
#define T_WALL   '}'
#define T_TRAP   '{'
#define T_COIN   '*'
#define T_CRATE  '#'
#define T_KEY    '$'
#define T_DOOR   '='
#define T_PLATE  '@'
#define T_GATE   '%'
#define T_LEVER  '<'
#define T_BOMB   '&'
#define T_SHIELD '^'
#define T_BREAK  '|'
#define T_TOGGLE '!'

typedef struct {
    int w, h;
    int sx, sy;            /* player start */
    int ex, ey;            /* exit cell (opening in the outer wall) */
    int nflip;
    int flip[4];           /* cells toggled by the lever */
    unsigned char c[MAXCELLS];
} Room;

enum { MODE_NORMAL = 1, MODE_ENDLESS = 2 };
enum { EV_NONE = 0, EV_CLEAR = 1, EV_DEAD = 2, EV_COIN = 4, EV_ITEM = 8 };

typedef struct {
    Room room, orig;       /* orig = pristine copy for restarts */
    int px, py;            /* player tile */
    int dx, dy;            /* dash direction, 0,0 = idle */
    int prog;              /* pixels travelled towards the next tile (0..15) */
    int to_exit;           /* the step in progress enters the exit */
    int coins, coin_rec, keys, bombs, shield;
    int mode, level, depth;
    const char *msg;
    int msg_t;
} Game;

#define CELL(r, x, y) ((r)->c[(y) * (r)->w + (x)])

void room_load_level(Room *r, int idx);
void game_enter_room(Game *g);          /* resets per-room counters, drops player at start */
void game_restart(Game *g);
int  game_dash(Game *g, int dx, int dy);/* 1 if a dash started */
int  game_tick(Game *g, int pixels);    /* advance the dash, returns EV_ flags */
int  game_bomb(Game *g);
static inline int game_dashing(const Game *g) { return g->dx || g->dy; }

/* generator and solver (gen.c) */
void gen_room(Room *r, unsigned *seed, int depth);
/* BFS over rest positions. dist[] gets move counts (-1 = unreachable).
   returns exit distance or -1. *stranded = number of reachable rest positions
   from which the exit can not be reached any more. */
int  room_solve(const Room *r, unsigned char *dist, int *stranded);
unsigned rng_next(unsigned *s);

#endif
