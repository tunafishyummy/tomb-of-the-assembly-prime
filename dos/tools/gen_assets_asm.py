#!/usr/bin/env python3
"""Generates src/assets.h (palette, tiles, font) and assets_preview.png.
Pixel art is authored as text so it can be tweaked without a paint program."""
import sys
try:
    from PIL import Image
except ImportError:
    Image = None   # the preview PNG is optional

PAL = [  # name, 6 bit rgb
 ('BLACK',(0,0,0)),('FLOOR',(2,2,9)),('FLOOR2',(6,6,16)),('WALL',(7,9,40)),('WALL_HI',(28,34,63)),
 ('WALL_LO',(3,4,26)),('TRAP',(40,4,4)),('TRAP_HI',(63,42,42)),('TRAP_SP',(22,2,2)),
 ('YELLOW',(63,56,6)),('YELLOW_HI',(63,63,40)),('YELLOW_LO',(44,34,0)),('WHITE',(63,63,63)),
 ('GREY',(34,34,38)),('GREY_LO',(18,18,22)),('SILVER',(50,52,58)),('PURPLE',(36,10,44)),
 ('PURPLE_HI',(54,30,63)),('TEAL',(4,40,40)),('TEAL_HI',(30,63,60)),('GATE',(10,20,63)),
 ('GATE_HI',(40,50,63)),('OLIVE',(36,42,10)),('OLIVE_HI',(54,58,24)),('RED',(58,8,8)),
 ('RED_HI',(63,36,30)),('AQUA',(8,52,63)),('AQUA_HI',(40,63,63)),('BROWN',(40,20,6)),
 ('BROWN_HI',(54,34,14)),('BROWN_LO',(22,10,2)),('NAVY',(8,8,30)),('NAVY_HI',(22,22,52)),
 ('GREEN',(6,50,12)),('GREEN_HI',(40,63,40)),('HUD',(5,5,14)),('EYE',(4,4,4)),
]
IDX = {n: i for i, (n, _) in enumerate(PAL)}
KEY = 255
LEG = {'y':'YELLOW','Y':'YELLOW_HI','o':'YELLOW_LO','w':'WHITE','k':'BLACK','g':'GREY','G':'GREY_LO',
 's':'SILVER','p':'PURPLE','P':'PURPLE_HI','t':'TEAL','T':'TEAL_HI','b':'GATE','B':'GATE_HI',
 'l':'OLIVE','L':'OLIVE_HI','r':'RED','R':'RED_HI','a':'AQUA','A':'AQUA_HI','n':'BROWN','N':'BROWN_HI',
 'm':'BROWN_LO','v':'NAVY','V':'NAVY_HI','h':'GREEN','H':'GREEN_HI','f':'FLOOR2','e':'EYE'}

def sprite(rows):
    assert len(rows) == 16, rows
    out = []
    for r in rows:
        assert len(r) == 16, (r, len(r))
        out += [KEY if ch == '.' else IDX[LEG[ch]] for ch in r]
    return out

def on_floor(spr):
    f = floor()
    return [f[i] if spr[i] == KEY else spr[i] for i in range(256)]

def floor():
    t = [IDX['FLOOR']] * 256
    for (x, y) in ((7, 7), (8, 7), (7, 8), (8, 8)):
        t[y*16+x] = IDX['FLOOR2']
    return t

SPR = {}
SPR['coin0'] = [
"................","......oooo......","....ooyyyyoo....","...oyyYYyyyyo...","..oyyYyyyyyyyo..","..oyYyyyyyyyyo..",
".oyyYyyooyyyyyo.",".oyyyyyoyyyyyyo.",".oyyyyyoyyyyyyo.",".oyyyyyooyyyyyo.","..oyyyyyyyyyyo..","..oyyyyyyyyyyo..",
"...oyyyyyyyyo...","....ooyyyyoo....","......oooo......","................"]
SPR['coin1'] = [
"................","......oooo......","......oyYo......",".....oyYyyo.....",".....oyYyyo.....",".....oyYyyo.....",
".....oyYyyo.....",".....oyYyyo.....",".....oyYyyo.....",".....oyYyyo.....",".....oyYyyo.....",".....oyYyyo.....",
".....oyYyyo.....","......oyyo......","......oooo......","................"]
SPR['key'] = [
"................","....ssssss......","...sGGGGGGs.....","...sG....Gs.....","...sG....Gs.....","...sGGGGGGs.....",
"....ssssss......",".....sGs........",".....sGs........",".....sGsss......",".....sGGGs......",".....sGs........",
".....sGsss......",".....sGGGs......",".....sss........","................"]
SPR['door'] = [
"pppppppppppppppp","pPPPPPPPPPPPPPPp","pPppppppppppppPp","pPppppppppppppPp","pPppppkkkkpppppp","pPpppkkkkkkppppp",
"pPpppkkkkkkppppp","pPppppkkkkpppppp","pPpppppkkpppppPp","pPpppppkkpppppPp","pPpppppkkpppppPp","pPppppppppppppPp",
"pPppppppppppppPp","pPPPPPPPPPPPPPPp","pppppppppppppppp","pppppppppppppppp"]
SPR['plate'] = [
"................","................","................","................","..TTTTTTTTTTTT..",".TttttttttttttT.",
".TttttttttttttT.",".TttttttttttttT.",".TttttttttttttT.",".TttttttttttttT.",".TttttttttttttT.",".TttttttttttttT.",
"..tttttttttttt..","................","................","................"]
SPR['gate'] = [
"BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.",
"BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.",
"BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb.","BbbBbbBbbBbbBbb."]
SPR['lever'] = [
"................","................","..........LL....",".........LLl....","........LLl.....",".......LLl......",
"......LLl.......",".....LLl........",".....Ll.........","....gggggg......","...gGGGGGGg.....","...gGGGGGGg.....",
"...gggggggg.....","................","................","................"]
SPR['bomb'] = [
"................","..........yy....",".........y.y....","........sy......",".....kkkks......","...kkkkkkkkk....",
"..kkwwkkkkkkk...","..kkwkkkkkkkk...","..kkkkkkkkkkk...","..kkkkkkkkkkk...","..kkkkkkkkkkk...","...kkkkkkkkk....",
"....kkkkkkk.....",".....kkkkk......","................","................"]
SPR['shield'] = [
"................","...aaaaaaaaaa...","..aAAAAAAAAAAa..","..aAaaaaaaaaAa..","..aAaaaAAaaaAa..","..aAaaAAAAaaAa..",
"..aAaaaAAaaaAa..","..aAaaaAAaaaAa..","..aAaaaaaaaaAa..","...aAaaaaaaAa...","....aAaaaaAa....",".....aAaaAa.....",
"......aAAa......",".......aa.......","................","................"]
SPR['crate'] = [
"nNNNNNNNNNNNNNNn","NnnnnnnnnnnnnnnN","NnNmmmmmmmmmmNnN","NnmNnnnnnnnnNmnN","NnmnNnnnnnnNnmnN","NnmnnNnnnnNnnmnN",
"NnmnnnNnnNnnnmnN","NnmnnnnNNnnnnmnN","NnmnnnnNNnnnnmnN","NnmnnnNnnNnnnmnN","NnmnnNnnnnNnnmnN","NnmnNnnnnnnNnmnN",
"NnmNnnnnnnnnNmnN","NnNmmmmmmmmmmNnN","NnnnnnnnnnnnnnnN","nNNNNNNNNNNNNNNn"]
SPR['break'] = [
"GGGGGGGGGGGGGGGG","gggggggGgggggggG","gggggggGgggggggG","gggggggGgggggggG","GGGGGGGGGGGGGGGG","gggGggggggggGggg",
"gggGggggggggGggg","gggGggggggggGggg","GGGGGGGGGGGGGGGG","gggggggGgggggggG","gggggggGgggggggG","gggggggGgggggggG",
"GGGGGGGGGGGGGGGG","gggGggggggggGggg","gggGggggggggGggg","GGGGGGGGGGGGGGGG"]
SPR['toggle'] = [
"VVVVVVVVVVVVVVVV","VvvvvvvvvvvvvvvV","VvVvvvVvvvVvvvvV","VvvVvvvVvvvVvvvV","VvvvVvvvVvvvVvvV","VvvvvVvvvVvvvVvV",
"VvvvvvVvvvVvvvvV","VvVvvvvVvvvVvvvV","VvvVvvvvVvvvVvvV","VvvvVvvvvVvvvVvV","VvvvvVvvvvVvvvvV","VvvvvvVvvvvVvvvV",
"VvVvvvvVvvvvVvvV","VvvVvvvvVvvvvVvV","VvvvvvvvvvvvvvvV","VVVVVVVVVVVVVVVV"]
SPR['exit0'] = [
"................","................","....hhhhhhhh....","...hHHHHHHHHh...","..hHh......hHh..",".hHh........hHh.",
".hH..........Hh.",".hH....hh....Hh.",".hH....hh....Hh.",".hH..........Hh.",".hHh........hHh.","..hHh......hHh..",
"...hHHHHHHHHh...","....hhhhhhhh....","................","................"]
SPR['exit1'] = [
"................","....hhhhhhhh....","...hHHHHHHHHh...","..hHhhhhhhhhHh..",".hHh........hHh.",".hH...hhhh...Hh.",
".hH..hHHHHh..Hh.",".hH..hH..Hh..Hh.",".hH..hH..Hh..Hh.",".hH..hHHHHh..Hh.",".hH...hhhh...Hh.",".hHh........hHh.",
"..hHhhhhhhhhHh..","...hHHHHHHHHh...","....hhhhhhhh....","................"]
SPR['player'] = [
"................","....yyyyyyyy....","..yyYYyyyyyyyy..",".yyYYyyyyyyyyyo.",".yyYyyyyyyyyyyo.",".yyyyyyyyyyyyyo.",
".yyykkkyykkkyyo.",".yyykekyykekyyo.",".yyykkkyykkkyyo.",".yyyyyyyyyyyyyo.",".yyyykkkkkkyyyo.",".yyyyykkkkyyyoo.",
"..yyyyyyyyyyoo..","...yyyyyyyyoo...","....ooyyyyoo....","................"]

# ---- walls (auto tiled): bit0 up open, bit1 down open, bit2 left open, bit3 right open
def wall(mask):
    t = [IDX['WALL']] * 256
    def px(x, y, c): t[y*16+x] = IDX[c]
    for i in range(16):
        if mask & 1:
            px(i, 0, 'WALL_HI'); px(i, 1, 'WALL_HI'); px(i, 2, 'WALL_LO')
        if mask & 2:
            px(i, 15, 'WALL_HI'); px(i, 14, 'WALL_HI'); px(i, 13, 'WALL_LO')
        if mask & 4:
            px(0, i, 'WALL_HI'); px(1, i, 'WALL_HI'); px(2, i, 'WALL_LO')
        if mask & 8:
            px(15, i, 'WALL_HI'); px(14, i, 'WALL_HI'); px(13, i, 'WALL_LO')
    # texture: a few darker rivets so big wall masses are not flat
    for (x, y) in ((5, 5), (10, 10)):
        if 3 <= x <= 12 and 3 <= y <= 12: px(x, y, 'WALL_LO')
    # rounded outer corners
    if (mask & 1) and (mask & 4): px(0, 0, 'FLOOR'); px(1, 0, 'WALL_HI'); px(0, 1, 'WALL_HI')
    if (mask & 1) and (mask & 8): px(15, 0, 'FLOOR')
    if (mask & 2) and (mask & 4): px(0, 15, 'FLOOR')
    if (mask & 2) and (mask & 8): px(15, 15, 'FLOOR')
    return t

TILES = []   # (name, pixels)
TILES.append(('FLOOR', floor()))
for m in range(16): TILES.append(('WALL%d' % m, wall(m)))
for n in ('toggle', 'break', 'gate', 'door', 'plate', 'coin0', 'coin1', 'key', 'lever', 'bomb', 'shield', 'crate', 'exit0', 'exit1'):
    TILES.append((n.upper(), on_floor(sprite(SPR[n]))))
# gate/plate/lever look better with transparent background on floor: handled by on_floor
PLAYER = sprite(SPR['player'])

# ---- 5x7 font, each glyph = 7 rows of 5 pixels
FONT_SRC = """
A: .###. #...# #...# ##### #...# #...# #...#
B: ####. #...# #...# ####. #...# #...# ####.
C: .###. #...# #.... #.... #.... #...# .###.
D: ####. #...# #...# #...# #...# #...# ####.
E: ##### #.... #.... ####. #.... #.... #####
F: ##### #.... #.... ####. #.... #.... #....
G: .###. #...# #.... #.### #...# #...# .###.
H: #...# #...# #...# ##### #...# #...# #...#
I: ###.. .#... .#... .#... .#... .#... ###..
J: ..### ...#. ...#. ...#. ...#. #..#. .##..
K: #...# #..#. #.#.. ##... #.#.. #..#. #...#
L: #.... #.... #.... #.... #.... #.... #####
M: #...# ##.## #.#.# #.#.# #...# #...# #...#
N: #...# ##..# #.#.# #..## #...# #...# #...#
O: .###. #...# #...# #...# #...# #...# .###.
P: ####. #...# #...# ####. #.... #.... #....
Q: .###. #...# #...# #...# #.#.# #..#. .##.#
R: ####. #...# #...# ####. #.#.. #..#. #...#
S: .#### #.... #.... .###. ....# ....# ####.
T: ##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#..
U: #...# #...# #...# #...# #...# #...# .###.
V: #...# #...# #...# #...# #...# .#.#. ..#..
W: #...# #...# #...# #.#.# #.#.# ##.## #...#
X: #...# #...# .#.#. ..#.. .#.#. #...# #...#
Y: #...# #...# .#.#. ..#.. ..#.. ..#.. ..#..
Z: ##### ....# ...#. ..#.. .#... #.... #####
0: .###. #...# #..## #.#.# ##..# #...# .###.
1: ..#.. .##.. ..#.. ..#.. ..#.. ..#.. .###.
2: .###. #...# ....# ...#. ..#.. .#... #####
3: .###. #...# ....# ..##. ....# #...# .###.
4: ...#. ..##. .#.#. #..#. ##### ...#. ...#.
5: ##### #.... ####. ....# ....# #...# .###.
6: ..##. .#... #.... ####. #...# #...# .###.
7: ##### ....# ...#. ..#.. .#... .#... .#...
8: .###. #...# #...# .###. #...# #...# .###.
9: .###. #...# #...# .#### ....# ...#. .##..
' ': ..... ..... ..... ..... ..... ..... .....
.: ..... ..... ..... ..... ..... .##.. .##..
,: ..... ..... ..... ..... .##.. ..#.. .#...
:: ..... .##.. .##.. ..... .##.. .##.. .....
!: ..#.. ..#.. ..#.. ..#.. ..#.. ..... ..#..
?: .###. #...# ....# ...#. ..#.. ..... ..#..
-: ..... ..... ..... ##### ..... ..... .....
+: ..... ..#.. ..#.. ##### ..#.. ..#.. .....
/: ....# ....# ...#. ..#.. .#... #.... #....
': ..#.. ..#.. .#... ..... ..... ..... .....
>: #.... .#... ..#.. ...#. ..#.. .#... #....
<: ....# ...#. ..#.. .#... ..#.. ...#. ....#
*: ..... #.#.# .###. ##### .###. #.#.# .....
^: ..#.. .#.#. #...# ..... ..... ..... .....
=: ..... ..... ##### ..... ##### ..... .....
(: ...#. ..#.. .#... .#... .#... ..#.. ...#.
): .#... ..#.. ...#. ...#. ...#. ..#.. .#...
&: .##.. #..#. #.#.. .#... #.#.# #..#. .##.#
_: ..... ..... ..... ..... ..... ..... #####
%: ##..# ##..# ...#. ..#.. .#... #..## #..##
#: .#.#. .#.#. ##### .#.#. ##### .#.#. .#.#.
"""
FONT = {}
for line in FONT_SRC.strip().split('\n'):
    k, rest = line.split(': ', 1)
    k = k.strip("'") if k.startswith("'") and len(k) == 3 else k
    ch = ' ' if k == "' '" else k
    if k == "':": ch = "'"
    rows = rest.split()
    assert len(rows) == 7 and all(len(r) == 5 for r in rows), line
    FONT[ch] = rows

def font_bytes():
    out = []
    for code in range(32, 96):           # ' ' .. '_' (upper case only)
        ch = chr(code)
        rows = FONT.get(ch, ['.....'] * 7)
        out.append([sum(1 << (4 - i) for i, c in enumerate(r) if c == '#') for r in rows])
    return out

def main():
    h = ['# generated by tools/gen_assets_asm.py - do not edit', '# palette indices']
    for i, (n, _) in enumerate(PAL): h.append('    .equ P_%s, %d' % (n, i))
    h.append('    .equ P_COUNT, %d' % len(PAL))
    h.append('# tile indices')
    for i, (n, _) in enumerate(TILES): h.append('    .equ TI_%s, %d' % (n, i))
    h.append('    .equ TI_COUNT, %d' % len(TILES))
    h.append('    .text')
    h.append('# VGA DAC values (6 bit R,G,B), P_COUNT entries')
    h.append('palette:')
    for n, rgb in PAL: h.append('    .byte %d,%d,%d        # %s' % (rgb + (n,)))
    h.append('# 16x16 tiles, one palette index per pixel (255 = transparent), TI_COUNT tiles of 256 bytes')
    h.append('tile_data:')
    for n, px in TILES:
        h.append('    # TI_%s' % n)
        for r in range(16): h.append('    .byte ' + ','.join(map(str, px[r*16:(r+1)*16])))
    h.append('player_data:')
    for r in range(16): h.append('    .byte ' + ','.join(map(str, PLAYER[r*16:(r+1)*16])))
    h.append('# 5x7 font, characters 32..95, 7 row bytes each (bit 4 = leftmost pixel)')
    h.append('font5x7:')
    for r in font_bytes(): h.append('    .byte ' + ','.join(map(str, r)))
    open(sys.argv[1], 'w').write('\n'.join(h) + '\n')
main()
