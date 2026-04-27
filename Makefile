PRAIA_INCLUDE := $(shell praia --include-path)
UNAME_S := $(shell uname -s)
CXXFLAGS = -std=c++17 -D_XOPEN_SOURCE=600 -D_DARWIN_C_SOURCE -DNCURSES_WIDECHAR=1 -Wno-deprecated-declarations -shared -fPIC

ifeq ($(UNAME_S),Darwin)
  EXT = .dylib
  LDFLAGS = -undefined dynamic_lookup
  NCURSES_PREFIX := $(shell brew --prefix ncurses 2>/dev/null)
  ifneq ($(NCURSES_PREFIX),)
    NCURSES_CFLAGS = -I$(NCURSES_PREFIX)/include/ncursesw -I$(NCURSES_PREFIX)/include
    NCURSES_LDFLAGS = -L$(NCURSES_PREFIX)/lib -lncursesw
  else
    NCURSES_CFLAGS =
    NCURSES_LDFLAGS = -lncurses
  endif
else
  EXT = .so
  LDFLAGS =
  NCURSES_CFLAGS = $(shell pkg-config --cflags ncursesw 2>/dev/null)
  NCURSES_LDFLAGS = $(shell pkg-config --libs ncursesw 2>/dev/null || echo -lncursesw)
endif

all:
	g++ $(CXXFLAGS) -I$(PRAIA_INCLUDE) $(NCURSES_CFLAGS) $(LDFLAGS) $(NCURSES_LDFLAGS) -o plugins/curses$(EXT) plugins/curses.cpp

clean:
	rm -f plugins/curses.dylib plugins/curses.so

.PHONY: all clean
