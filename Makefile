# Automatlab -- compilation native (raylib installée) et web (emscripten)
#
# Natif : raylib >= 4.2 (testé avec raylib 5.5 Homebrew sur macOS).
#   make            -> ./main
#   make RAYLIB_INC=... RAYLIB_LIB=...   pour un raylib installé ailleurs
# Web : emcc dans le PATH et une libraylib.a compilée pour PLATFORM_WEB
#   make web RAYLIB_WEB_SRC=/chemin/vers/raylib/src
#   (cd $(RAYLIB_WEB_SRC) && make PLATFORM=PLATFORM_WEB  produit libraylib.a)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  BREW_PREFIX ?= $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
  RAYLIB_INC ?= $(BREW_PREFIX)/include
  RAYLIB_LIB ?= $(BREW_PREFIX)/lib
  SYSLIBS = -framework OpenGL -framework Cocoa -framework IOKit
else
  RAYLIB_INC ?= /usr/local/include
  RAYLIB_LIB ?= /usr/local/lib
  SYSLIBS = -lGL -lpthread -ldl -lrt -lX11
endif

CC      ?= cc
CFLAGS  ?= -g -Wall
SAN     ?= -fsanitize=address

RAYLIB_WEB_SRC ?= $(HOME)/dev/raylib/src
WEBDIR  = web

all: main

main: main.c raygui.h
	$(CC) $(CFLAGS) $(SAN) -I$(RAYLIB_INC) -o $@ main.c -L$(RAYLIB_LIB) -lraylib -lm $(SYSLIBS)

run: main
	./main

# Tests de la construction de Thompson (comparaison avec regex.h POSIX)
tests/test_thompson: tests/test_thompson.c main.c
	$(CC) -g -I. -I$(RAYLIB_INC) -o $@ tests/test_thompson.c -L$(RAYLIB_LIB) -lraylib -lm $(SYSLIBS)

test: tests/test_thompson
	./tests/test_thompson '(a|b)*abb' '(a|b)*abb' 'a*b*' 'a*b*' '(ab|ba)*' '(ab|ba)*' \
	  'e' '' 'a*' 'a*' 'ab(a|b)' 'ab(a|b)' '(a*b)*' '(a*b)*' 'b(a|e)b*' 'b(a)?b*'

# Version web : produit web/index.html, web/index.js, web/index.wasm
web: $(WEBDIR)/index.html

$(WEBDIR)/index.html: main.c raygui.h emscripten/minshell.html
	mkdir -p $(WEBDIR)
	emcc -DPLATFORM_WEB -Os -Wall -I$(RAYLIB_WEB_SRC) -o $@ main.c $(RAYLIB_WEB_SRC)/libraylib.a \
	  --shell-file emscripten/minshell.html -s USE_GLFW=3 -s WASM=1 -s ASSERTIONS=1

runserver: web
	cd $(WEBDIR) && python3 -m http.server 1234

clean:
	rm -rf main main.dSYM tests/test_thompson tests/test_thompson.dSYM $(WEBDIR)

.PHONY: all run test web runserver clean
