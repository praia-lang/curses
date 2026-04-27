#include "praia_plugin.h"
#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <locale.h>
#include <string>
#include <cstring>
#include <unordered_map>

static std::unordered_map<int, WINDOW*> g_windows;
static int g_nextWinId = 1;
static bool g_initialized = false;

static int registerWindow(WINDOW* win) {
    int id = g_nextWinId++;
    g_windows[id] = win;
    return id;
}

static WINDOW* getWindow(const Value& v) {
    if (v.isNil()) return stdscr;
    int id = static_cast<int>(v.asNumber());
    auto it = g_windows.find(id);
    if (it == g_windows.end()) throw RuntimeError("Invalid window handle", 0);
    return it->second;
}

extern "C" void praia_register(PraiaMap* m) {

    // ── Initialization ──

    m->entries[Value("initscr")] = Value(makeNative("curses.initscr", 0,
        [](const std::vector<Value>&) -> Value {
            setlocale(LC_ALL, "");
            initscr();
            g_initialized = true;
            raw();
            keypad(stdscr, TRUE);
            noecho();
            curs_set(0);
            if (has_colors()) {
                start_color();
                use_default_colors();
                init_pair(1, COLOR_RED, -1);
                init_pair(2, COLOR_GREEN, -1);
                init_pair(3, COLOR_YELLOW, -1);
                init_pair(4, COLOR_BLUE, -1);
                init_pair(5, COLOR_MAGENTA, -1);
                init_pair(6, COLOR_CYAN, -1);
                init_pair(7, COLOR_WHITE, -1);
                init_pair(8, COLOR_BLACK, -1);
            }
            return Value();
        }));

    m->entries[Value("endwin")] = Value(makeNative("curses.endwin", 0,
        [](const std::vector<Value>&) -> Value {
            if (g_initialized) {
                for (auto& [id, win] : g_windows)
                    if (win != stdscr) delwin(win);
                g_windows.clear();
                endwin();
                g_initialized = false;
            }
            return Value();
        }));

    // ── Screen info ──

    m->entries[Value("rows")] = Value(makeNative("curses.rows", 0,
        [](const std::vector<Value>&) -> Value {
            return Value(static_cast<int64_t>(LINES));
        }));

    m->entries[Value("cols")] = Value(makeNative("curses.cols", 0,
        [](const std::vector<Value>&) -> Value {
            return Value(static_cast<int64_t>(COLS));
        }));

    // ── Output ──

    m->entries[Value("move")] = Value(makeNative("curses.move", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() >= 3) wmove(getWindow(args[0]), static_cast<int>(args[1].asNumber()), static_cast<int>(args[2].asNumber()));
            else wmove(stdscr, static_cast<int>(args[0].asNumber()), static_cast<int>(args[1].asNumber()));
            return Value();
        }));

    m->entries[Value("addstr")] = Value(makeNative("curses.addstr", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() == 1) {
                addstr(args[0].asString().c_str());
            } else if (args.size() == 2) {
                waddstr(getWindow(args[0]), args[1].asString().c_str());
            } else if (args.size() == 3) {
                mvaddstr(static_cast<int>(args[0].asNumber()),
                         static_cast<int>(args[1].asNumber()),
                         args[2].asString().c_str());
            } else if (args.size() == 4) {
                mvwaddstr(getWindow(args[0]),
                          static_cast<int>(args[1].asNumber()),
                          static_cast<int>(args[2].asNumber()),
                          args[3].asString().c_str());
            }
            return Value();
        }));

    // Wide-string addstr — converts UTF-8 to wchar_t for Unicode output
    m->entries[Value("addwstr")] = Value(makeNative("curses.addwstr", -1,
        [](const std::vector<Value>& args) -> Value {
#ifdef NCURSES_WIDECHAR
            if (args.size() == 1) {
                // addwstr(str)
                auto& s = args[0].asString();
                std::mbstate_t state{};
                const char* src = s.c_str();
                size_t wlen = mbsrtowcs(nullptr, &src, 0, &state) + 1;
                std::vector<wchar_t> ws(wlen);
                src = s.c_str();
                std::memset(&state, 0, sizeof(state));
                mbsrtowcs(ws.data(), &src, wlen, &state);
                ::addwstr(ws.data());
            } else if (args.size() == 2) {
                // addwstr(win, str)
                WINDOW* win = getWindow(args[0]);
                auto& s = args[1].asString();
                std::mbstate_t state{};
                const char* src = s.c_str();
                size_t wlen = mbsrtowcs(nullptr, &src, 0, &state) + 1;
                std::vector<wchar_t> ws(wlen);
                src = s.c_str();
                std::memset(&state, 0, sizeof(state));
                mbsrtowcs(ws.data(), &src, wlen, &state);
                waddwstr(win, ws.data());
            } else if (args.size() == 3) {
                // addwstr(row, col, str)
                int r = static_cast<int>(args[0].asNumber());
                int c = static_cast<int>(args[1].asNumber());
                auto& s = args[2].asString();
                std::mbstate_t state{};
                const char* src = s.c_str();
                size_t wlen = mbsrtowcs(nullptr, &src, 0, &state) + 1;
                std::vector<wchar_t> ws(wlen);
                src = s.c_str();
                std::memset(&state, 0, sizeof(state));
                mbsrtowcs(ws.data(), &src, wlen, &state);
                mvaddwstr(r, c, ws.data());
            } else if (args.size() == 4) {
                // addwstr(win, row, col, str)
                WINDOW* win = getWindow(args[0]);
                int r = static_cast<int>(args[1].asNumber());
                int c = static_cast<int>(args[2].asNumber());
                auto& s = args[3].asString();
                std::mbstate_t state{};
                const char* src = s.c_str();
                size_t wlen = mbsrtowcs(nullptr, &src, 0, &state) + 1;
                std::vector<wchar_t> ws(wlen);
                src = s.c_str();
                std::memset(&state, 0, sizeof(state));
                mbsrtowcs(ws.data(), &src, wlen, &state);
                mvwaddwstr(win, r, c, ws.data());
            }
#else
            // Fallback: use regular addstr (won't render multi-byte chars correctly)
            if (args.size() == 1) addstr(args[0].asString().c_str());
            else if (args.size() == 2) waddstr(getWindow(args[0]), args[1].asString().c_str());
            else if (args.size() == 3) mvaddstr(static_cast<int>(args[0].asNumber()), static_cast<int>(args[1].asNumber()), args[2].asString().c_str());
            else if (args.size() == 4) mvwaddstr(getWindow(args[0]), static_cast<int>(args[1].asNumber()), static_cast<int>(args[2].asNumber()), args[3].asString().c_str());
#endif
            return Value();
        }));

    // Read character at position (for save/restore under cursor)
    m->entries[Value("inch")] = Value(makeNative("curses.inch", -1,
        [](const std::vector<Value>& args) -> Value {
            chtype ch;
            if (args.size() == 0) {
                ch = ::inch();
            } else if (args.size() == 1) {
                ch = winch(getWindow(args[0]));
            } else if (args.size() == 2) {
                ch = mvinch(static_cast<int>(args[0].asNumber()),
                            static_cast<int>(args[1].asNumber()));
            } else {
                ch = mvwinch(getWindow(args[0]),
                             static_cast<int>(args[1].asNumber()),
                             static_cast<int>(args[2].asNumber()));
            }
            return Value(static_cast<int64_t>(ch));
        }));

    // Write chtype (character + attributes) at position
    m->entries[Value("addch")] = Value(makeNative("curses.addch", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() == 1) {
                addch(static_cast<chtype>(args[0].asInt()));
            } else if (args.size() == 2) {
                waddch(getWindow(args[0]), static_cast<chtype>(args[1].asInt()));
            } else if (args.size() == 3) {
                mvaddch(static_cast<int>(args[0].asNumber()),
                        static_cast<int>(args[1].asNumber()),
                        static_cast<chtype>(args[2].asInt()));
            } else if (args.size() == 4) {
                mvwaddch(getWindow(args[0]),
                         static_cast<int>(args[1].asNumber()),
                         static_cast<int>(args[2].asNumber()),
                         static_cast<chtype>(args[3].asInt()));
            }
            return Value();
        }));

    m->entries[Value("refresh")] = Value(makeNative("curses.refresh", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.empty()) refresh(); else wrefresh(getWindow(args[0]));
            return Value();
        }));

    m->entries[Value("clear")] = Value(makeNative("curses.clear", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.empty()) clear(); else wclear(getWindow(args[0]));
            return Value();
        }));

    m->entries[Value("erase")] = Value(makeNative("curses.erase", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.empty()) erase(); else werase(getWindow(args[0]));
            return Value();
        }));

    m->entries[Value("clrtoeol")] = Value(makeNative("curses.clrtoeol", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.empty()) clrtoeol(); else wclrtoeol(getWindow(args[0]));
            return Value();
        }));

    // ── Input ──

    m->entries[Value("getch")] = Value(makeNative("curses.getch", -1,
        [](const std::vector<Value>& args) -> Value {
            WINDOW* win = args.empty() ? stdscr : getWindow(args[0]);
            int ch = wgetch(win);
            if (ch == ERR) return Value();
            switch (ch) {
                case KEY_UP:        return Value(std::string("UP"));
                case KEY_DOWN:      return Value(std::string("DOWN"));
                case KEY_LEFT:      return Value(std::string("LEFT"));
                case KEY_RIGHT:     return Value(std::string("RIGHT"));
                case KEY_HOME:      return Value(std::string("HOME"));
                case KEY_END:       return Value(std::string("END"));
                case KEY_PPAGE:     return Value(std::string("PGUP"));
                case KEY_NPAGE:     return Value(std::string("PGDN"));
                case KEY_IC:        return Value(std::string("INSERT"));
                case KEY_DC:        return Value(std::string("DELETE"));
                case KEY_BACKSPACE: return Value(std::string("BACKSPACE"));
                case KEY_ENTER: case 10: case 13: return Value(std::string("ENTER"));
                case 9:             return Value(std::string("TAB"));
                case 27:            return Value(std::string("ESCAPE"));
                case 32:            return Value(std::string("SPACE"));
                case KEY_RESIZE:    return Value(std::string("RESIZE"));
                case KEY_MOUSE:     return Value(std::string("MOUSE"));
                case KEY_F(1):  return Value(std::string("F1"));
                case KEY_F(2):  return Value(std::string("F2"));
                case KEY_F(3):  return Value(std::string("F3"));
                case KEY_F(4):  return Value(std::string("F4"));
                case KEY_F(5):  return Value(std::string("F5"));
                case KEY_F(6):  return Value(std::string("F6"));
                case KEY_F(7):  return Value(std::string("F7"));
                case KEY_F(8):  return Value(std::string("F8"));
                case KEY_F(9):  return Value(std::string("F9"));
                case KEY_F(10): return Value(std::string("F10"));
                case KEY_F(11): return Value(std::string("F11"));
                case KEY_F(12): return Value(std::string("F12"));
                case 127: case 8: return Value(std::string("BACKSPACE"));
                default:
                    if (ch >= 1 && ch <= 26)
                        return Value(std::string("CTRL+") + (char)('A' + ch - 1));
                    if (ch >= 32 && ch < 127)
                        return Value(std::string(1, static_cast<char>(ch)));
                    return Value(static_cast<int64_t>(ch));
            }
        }));

    m->entries[Value("nodelay")] = Value(makeNative("curses.nodelay", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() == 1) nodelay(stdscr, args[0].isTruthy() ? TRUE : FALSE);
            else nodelay(getWindow(args[0]), args[1].isTruthy() ? TRUE : FALSE);
            return Value();
        }));

    m->entries[Value("timeout")] = Value(makeNative("curses.timeout", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() == 1) wtimeout(stdscr, static_cast<int>(args[0].asNumber()));
            else wtimeout(getWindow(args[0]), static_cast<int>(args[1].asNumber()));
            return Value();
        }));

    // ── Attributes (window-aware) ──

    m->entries[Value("attron")] = Value(makeNative("curses.attron", -1,
        [](const std::vector<Value>& args) -> Value {
            WINDOW* win = stdscr;
            std::string s;
            if (args.size() == 1) { s = args[0].asString(); }
            else { win = getWindow(args[0]); s = args[1].asString(); }
            if (s == "bold") wattron(win, A_BOLD);
            else if (s == "dim") wattron(win, A_DIM);
            else if (s == "underline") wattron(win, A_UNDERLINE);
            else if (s == "reverse") wattron(win, A_REVERSE);
            else if (s == "blink") wattron(win, A_BLINK);
            else if (s == "standout") wattron(win, A_STANDOUT);
            return Value();
        }));

    m->entries[Value("attroff")] = Value(makeNative("curses.attroff", -1,
        [](const std::vector<Value>& args) -> Value {
            WINDOW* win = stdscr;
            std::string s;
            if (args.size() == 1) { s = args[0].asString(); }
            else { win = getWindow(args[0]); s = args[1].asString(); }
            if (s == "bold") wattroff(win, A_BOLD);
            else if (s == "dim") wattroff(win, A_DIM);
            else if (s == "underline") wattroff(win, A_UNDERLINE);
            else if (s == "reverse") wattroff(win, A_REVERSE);
            else if (s == "blink") wattroff(win, A_BLINK);
            else if (s == "standout") wattroff(win, A_STANDOUT);
            return Value();
        }));

    m->entries[Value("attreset")] = Value(makeNative("curses.attreset", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.empty()) attrset(A_NORMAL);
            else wattrset(getWindow(args[0]), A_NORMAL);
            return Value();
        }));

    // ── Colors (window-aware) ──

    m->entries[Value("color")] = Value(makeNative("curses.color", -1,
        [](const std::vector<Value>& args) -> Value {
            WINDOW* win = stdscr;
            std::string s;
            if (args.size() == 1) { s = args[0].asString(); }
            else { win = getWindow(args[0]); s = args[1].asString(); }
            int pair = 0;
            if (s == "red") pair = 1; else if (s == "green") pair = 2;
            else if (s == "yellow") pair = 3; else if (s == "blue") pair = 4;
            else if (s == "magenta") pair = 5; else if (s == "cyan") pair = 6;
            else if (s == "white") pair = 7; else if (s == "black") pair = 8;
            if (pair > 0) wattron(win, COLOR_PAIR(pair));
            else wattroff(win, A_COLOR);
            return Value();
        }));

    m->entries[Value("colorpair")] = Value(makeNative("curses.colorpair", 3,
        [](const std::vector<Value>& args) -> Value {
            init_pair(static_cast<int>(args[0].asNumber()),
                      static_cast<int>(args[1].asNumber()),
                      static_cast<int>(args[2].asNumber()));
            return Value();
        }));

    m->entries[Value("usepair")] = Value(makeNative("curses.usepair", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() == 1) attron(COLOR_PAIR(static_cast<int>(args[0].asNumber())));
            else wattron(getWindow(args[0]), COLOR_PAIR(static_cast<int>(args[1].asNumber())));
            return Value();
        }));

    // ── Cursor ──

    m->entries[Value("cursor")] = Value(makeNative("curses.cursor", 1,
        [](const std::vector<Value>& args) -> Value {
            curs_set(args[0].isTruthy() ? 1 : 0);
            return Value();
        }));

    // ── Drawing ──

    m->entries[Value("box")] = Value(makeNative("curses.box", -1,
        [](const std::vector<Value>& args) -> Value {
            WINDOW* win = args.empty() ? stdscr : getWindow(args[0]);
            ::box(win, 0, 0);
            return Value();
        }));

    m->entries[Value("hline")] = Value(makeNative("curses.hline", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() == 1) hline(ACS_HLINE, static_cast<int>(args[0].asNumber()));
            else if (args.size() == 3) mvhline(static_cast<int>(args[0].asNumber()), static_cast<int>(args[1].asNumber()), ACS_HLINE, static_cast<int>(args[2].asNumber()));
            return Value();
        }));

    m->entries[Value("vline")] = Value(makeNative("curses.vline", -1,
        [](const std::vector<Value>& args) -> Value {
            if (args.size() == 1) vline(ACS_VLINE, static_cast<int>(args[0].asNumber()));
            else if (args.size() == 3) mvvline(static_cast<int>(args[0].asNumber()), static_cast<int>(args[1].asNumber()), ACS_VLINE, static_cast<int>(args[2].asNumber()));
            return Value();
        }));

    // ── Windows ──

    m->entries[Value("newwin")] = Value(makeNative("curses.newwin", 4,
        [](const std::vector<Value>& args) -> Value {
            WINDOW* win = ::newwin(
                static_cast<int>(args[0].asNumber()), static_cast<int>(args[1].asNumber()),
                static_cast<int>(args[2].asNumber()), static_cast<int>(args[3].asNumber()));
            if (!win) throw RuntimeError("Failed to create window", 0);
            keypad(win, TRUE);
            return Value(static_cast<int64_t>(registerWindow(win)));
        }));

    m->entries[Value("delwin")] = Value(makeNative("curses.delwin", 1,
        [](const std::vector<Value>& args) -> Value {
            int id = static_cast<int>(args[0].asNumber());
            auto it = g_windows.find(id);
            if (it != g_windows.end()) { delwin(it->second); g_windows.erase(it); }
            return Value();
        }));

    // ── Misc ──

    m->entries[Value("napms")] = Value(makeNative("curses.napms", 1,
        [](const std::vector<Value>& args) -> Value { napms(static_cast<int>(args[0].asNumber())); return Value(); }));

    m->entries[Value("beep")] = Value(makeNative("curses.beep", 0,
        [](const std::vector<Value>&) -> Value { ::beep(); return Value(); }));

    m->entries[Value("flash")] = Value(makeNative("curses.flash", 0,
        [](const std::vector<Value>&) -> Value { ::flash(); return Value(); }));

    m->entries[Value("hasColors")] = Value(makeNative("curses.hasColors", 0,
        [](const std::vector<Value>&) -> Value { return Value(has_colors() != 0); }));

    // Color constants
    m->entries[Value("COLOR_BLACK")]   = Value(static_cast<int64_t>(COLOR_BLACK));
    m->entries[Value("COLOR_RED")]     = Value(static_cast<int64_t>(COLOR_RED));
    m->entries[Value("COLOR_GREEN")]   = Value(static_cast<int64_t>(COLOR_GREEN));
    m->entries[Value("COLOR_YELLOW")]  = Value(static_cast<int64_t>(COLOR_YELLOW));
    m->entries[Value("COLOR_BLUE")]    = Value(static_cast<int64_t>(COLOR_BLUE));
    m->entries[Value("COLOR_MAGENTA")] = Value(static_cast<int64_t>(COLOR_MAGENTA));
    m->entries[Value("COLOR_CYAN")]    = Value(static_cast<int64_t>(COLOR_CYAN));
    m->entries[Value("COLOR_WHITE")]   = Value(static_cast<int64_t>(COLOR_WHITE));

    // ── Mouse ──

    m->entries[Value("mouseOn")] = Value(makeNative("curses.mouseOn", 0,
        [](const std::vector<Value>&) -> Value {
            mouseinterval(0); // no click delay
            mousemask(ALL_MOUSE_EVENTS, nullptr);
            return Value();
        }));

    m->entries[Value("mouseOff")] = Value(makeNative("curses.mouseOff", 0,
        [](const std::vector<Value>&) -> Value {
            mousemask(0, nullptr);
            return Value();
        }));

    m->entries[Value("getmouse")] = Value(makeNative("curses.getmouse", 0,
        [](const std::vector<Value>&) -> Value {
            MEVENT event;
            if (::getmouse(&event) != OK) return Value();
            auto result = gcNew<PraiaMap>();
            result->entries[Value("x")] = Value(static_cast<int64_t>(event.x));
            result->entries[Value("y")] = Value(static_cast<int64_t>(event.y));

            // Decode button state
            std::string button;
            mmask_t bs = event.bstate;
            if (bs & BUTTON1_PRESSED)             button = "LEFT_PRESS";
            else if (bs & BUTTON1_RELEASED)       button = "LEFT_RELEASE";
            else if (bs & BUTTON1_CLICKED)        button = "LEFT_CLICK";
            else if (bs & BUTTON1_DOUBLE_CLICKED) button = "LEFT_DOUBLE";
            else if (bs & BUTTON2_PRESSED)        button = "MIDDLE_PRESS";
            else if (bs & BUTTON2_RELEASED)       button = "MIDDLE_RELEASE";
            else if (bs & BUTTON2_CLICKED)        button = "MIDDLE_CLICK";
            else if (bs & BUTTON3_PRESSED)        button = "RIGHT_PRESS";
            else if (bs & BUTTON3_RELEASED)       button = "RIGHT_RELEASE";
            else if (bs & BUTTON3_CLICKED)        button = "RIGHT_CLICK";
#ifdef BUTTON4_PRESSED
            else if (bs & BUTTON4_PRESSED)        button = "SCROLL_UP";
#endif
#ifdef BUTTON5_PRESSED
            else if (bs & BUTTON5_PRESSED)        button = "SCROLL_DOWN";
#endif
            // Fallback — report raw bstate so user can debug
            if (button.empty())
                button = "UNKNOWN(" + std::to_string(bs) + ")";

            result->entries[Value("button")] = Value(button);
            result->entries[Value("bstate")] = Value(static_cast<int64_t>(event.bstate));
            return Value(result);
        }));

    // KEY_MOUSE constant for comparison
    m->entries[Value("KEY_MOUSE")] = Value(std::string("MOUSE"));
}
