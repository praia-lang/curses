# curses

Terminal UI library for [Praia](https://praia.sh), wrapping [ncurses](https://invisible-island.net/ncurses/) via a native C++ plugin. Ships with pre-built native plugins for macOS and Linux.

## Install

```sh
sand install github.com/praia-lang/curses
```

Pre-built binaries are included for macOS (ARM), Linux (x86_64), and Linux (ARM).

## Usage

```praia
use "curses"

curses.initscr()
curses.addstr(0, 0, "Hello, TUI!")
curses.refresh()
curses.getch()
curses.endwin()
```

## API

### Initialization

| Function | Description |
|----------|-------------|
| `initscr()` | Enter curses mode (raw input, hidden cursor, colors) |
| `endwin()` | Restore terminal to normal |

### Screen

| Function | Description |
|----------|-------------|
| `rows()` | Terminal height |
| `cols()` | Terminal width |
| `clear()` | Clear screen |
| `erase()` | Erase screen without repaint |
| `clrtoeol()` | Clear to end of line |
| `refresh()` | Flush changes to terminal |
| `refresh(win)` | Flush a window |

### Output

| Function | Description |
|----------|-------------|
| `addstr("text")` | Write at cursor |
| `addstr(row, col, "text")` | Write at position |
| `addstr(win, row, col, "text")` | Write in a window |
| `addwstr(row, col, "█▓▒░")` | Wide-char/Unicode output (requires ncursesw) |
| `move(row, col)` | Move cursor |

### Input

| Function | Description |
|----------|-------------|
| `getch()` | Blocking key read, returns key name |
| `nodelay(true)` | Non-blocking mode (getch returns nil if no key) |
| `timeout(ms)` | getch waits up to N milliseconds |

Key names: `"a"`-`"z"`, `"UP"`, `"DOWN"`, `"LEFT"`, `"RIGHT"`, `"ENTER"`, `"BACKSPACE"`, `"TAB"`, `"ESCAPE"`, `"SPACE"`, `"HOME"`, `"END"`, `"PGUP"`, `"PGDN"`, `"DELETE"`, `"INSERT"`, `"F1"`-`"F12"`, `"CTRL+A"`-`"CTRL+Z"`, `"MOUSE"`, `"RESIZE"`

### Attributes

| Function | Description |
|----------|-------------|
| `attron("bold")` | Enable attribute (bold, dim, underline, reverse, blink, standout) |
| `attroff("bold")` | Disable attribute |
| `attreset()` | Reset all attributes |
| `attron(win, "bold")` | Window-specific attribute |

### Colors

| Function | Description |
|----------|-------------|
| `color("red")` | Set foreground (red, green, yellow, blue, magenta, cyan, white, black) |
| `color(win, "red")` | Set color on a window |
| `colorpair(id, fg, bg)` | Define a custom color pair |
| `usepair(id)` | Activate a color pair |
| `hasColors()` | True if terminal supports colors |

### Mouse

```praia
curses.mouseOn()

let key = curses.getch()
if (key == "MOUSE") {
    let ev = curses.getmouse()
    print(ev.x, ev.y, ev.button)
}
```

| Function | Description |
|----------|-------------|
| `mouseOn()` | Enable mouse events |
| `mouseOff()` | Disable mouse events |
| `getmouse()` | Returns `{x, y, button, bstate}` after a MOUSE event |

Button values: `LEFT_CLICK`, `LEFT_PRESS`, `LEFT_RELEASE`, `LEFT_DOUBLE`, `MIDDLE_CLICK`, `RIGHT_CLICK`, `SCROLL_UP`, `SCROLL_DOWN`

### Windows

| Function | Description |
|----------|-------------|
| `newwin(h, w, row, col)` | Create a sub-window |
| `box(win)` | Draw border around window |
| `delwin(win)` | Destroy a window |
| `hline(row, col, len)` | Horizontal line |
| `vline(row, col, len)` | Vertical line |

### Misc

| Function | Description |
|----------|-------------|
| `cursor(bool)` | Show or hide terminal cursor |
| `napms(ms)` | Sleep N milliseconds |
| `beep()` | Terminal bell |
| `flash()` | Visual bell |

## Examples

```sh
praia examples/demo.praia    # keyboard navigation, windows, colors
praia examples/mouse.praia   # mouse painting with scroll color selection
```

## Building from source

If you need to rebuild the native plugin (e.g. for a different architecture):

```sh
cd ext_grains/curses
make
```

Requires ncurses headers and a C++17 compiler. For wide-character/Unicode support, install ncursesw:

```sh
# macOS
brew install ncurses

# Ubuntu / Debian
sudo apt install libncursesw5-dev

# Fedora / RHEL
sudo dnf install ncurses-devel
```

The Makefile auto-detects ncursesw and falls back to standard ncurses if unavailable.

## Structure

```
curses/
  grain.yaml
  main.praia               # Praia wrapper, loads native plugin
  plugins/
    curses.cpp             # C++ source
    curses.dylib           # macOS ARM pre-built
    curses-linux-x86_64.so # Linux x86_64 pre-built
    curses-linux-arm64.so  # Linux ARM pre-built
```
