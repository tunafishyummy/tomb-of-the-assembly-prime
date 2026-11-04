# Tomb of the Assembly - DOS edition (pure x86 assembly)
#
#   make          assemble + link  ->  TOMB.COM
#   make test     compare against the reference build (needs python3 and gcc)
#   make clean
#
# Tools needed: GNU as + ld (package "binutils", 32-bit x86 support).  On Windows use WSL (see HOW_TO_RUN.md).
AS = as --32 -march=i386 -I src
SRC = src/tomb.s src/defs.inc src/plat.inc src/gfx.inc src/engine.inc src/gen.inc src/gen_vars.inc src/ui.inc src/assets.inc src/levels.inc

all: TOMB.COM

build/tomb.o: $(SRC)
	@mkdir -p build
	$(AS) src/tomb.s -o build/tomb.o

TOMB.COM: build/tomb.o src/tomb.ld
	ld -m elf_i386 -T src/tomb.ld -o $@ build/tomb.o
	@echo "TOMB.COM = $$(stat -c %s $@) bytes"

# data tables are generated once and committed; rerun these only if you change sprites/font/levels
assets:
	python3 tools/gen_assets_asm.py src/assets.inc
levels:
	python3 tools/extract_levels_asm.py ../Simulador/tomb_of_mask.asm src/levels.inc reference_c/src/levels.h

tests/x86run: tests/x86run.c
	gcc -O2 -Wno-unused -o $@ $<
tests/gen_dump: tests/gen_dump.c
	gcc -O1 -Wno-unused -o $@ $< reference_c/src/gen.c reference_c/src/engine.c

test: TOMB.COM tests/x86run tests/gen_dump
	python3 tests/gen_check.py
	python3 tests/compare.py reference_c/TOMB_C.COM TOMB.COM tests/x86run 8 14 30 -k 5:enter -k 20:d
	python3 tests/fuzz_compare.py reference_c/TOMB_C.COM TOMB.COM tests/x86run 4 3000
	python3 tests/endless_play.py reference_c/TOMB_C.COM TOMB.COM tests/x86run 3

clean:
	rm -rf build TOMB.COM tests/x86run tests/gen_dump
