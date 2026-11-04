/* prints a generated room as: w h sx sy ex ey then the w*h cell bytes in hex (reference for the assembly generator) */
#include <stdio.h>
#include <stdlib.h>
#include "../reference_c/src/engine.h"
int main(int argc, char **argv)
{
    static Room r; unsigned seed = (unsigned)strtoul(argv[1], 0, 10); int depth = atoi(argv[2]), i;
    gen_room(&r, &seed, depth);
    printf("%d %d %d %d %d %d %u\n", r.w, r.h, r.sx, r.sy, r.ex, r.ey, seed);
    for (i = 0; i < r.w * r.h; i++) printf("%02x", r.c[i]);
    printf("\n");
    return 0;
}
