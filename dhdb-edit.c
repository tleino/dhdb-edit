#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "dhdb.h"
#include "dhdb_json.h"

const char _dhdb_type_to_str[NUM_DHDB_VALUE][20] = {
	"undefined", "object", "array", "number", "string", "bool", "null"
};

dhdb_t *_root;

static void _print_cmds(dhdb_t *, dhdb_t *);
static void _dhdb_print_val(dhdb_t *, bool, int, int, int);
static void _screen_init();

int
main(int argc, char **argv)
{	
	dhdb_t *d, *root, *parent, *prev;
	FILE *fp;
	char buf[65000];
	int c, line, prevLine;
	double num;

	if (argc == 2) {
		fp = fopen(argv[1], "r");
		fread(&buf, sizeof(buf), 1, fp);
		d = dhdb_create_from_json(buf);
	} else
		d = dhdb_create();
	root = _root = d;

	_screen_init();
	num = 0.0;
	line = 0;
	prevLine = 0;
	while (1) {
		parent = dhdb_parent(d);

		_print_cmds(parent, d);

		if (parent)
			_dhdb_print_val(parent, true, 0, 0, line);
		else
			_dhdb_print_val(d, false, 0, 0, 0);

		c = wgetch(stdscr);
		if (c == KEY_LEFT && dhdb_parent(d)) {
			wclear(stdscr);
			d = dhdb_parent(d);
			line = prevLine;
		}
		if (c == KEY_RIGHT && dhdb_is_container(d)) {
			wclear(stdscr);
			d = dhdb_first(d);
			prevLine = line;
			line = 0;
		}
		if (c == KEY_UP && dhdb_prev(d)) {
			wclear(stdscr);
			d = dhdb_prev(d);
			line--;
		}
		if (c == KEY_DOWN && dhdb_next(d)) {
			wclear(stdscr);
			d = dhdb_next(d);
			line++;
		}
		if (c == '\r' && dhdb_parent(d) &&
		    dhdb_type(d) != DHDB_VALUE_UNDEFINED) { 
			wclear(stdscr);
			if (dhdb_type(dhdb_parent(d)) == DHDB_VALUE_OBJECT) {
				char buf[64];
				snprintf(buf, sizeof(buf), "%d",
				    dhdb_len(dhdb_parent(d)));
				dhdb_set_obj(dhdb_parent(d), buf,
				    dhdb_create());
			} else
				dhdb_add(dhdb_parent(d), dhdb_create());
			line = dhdb_len(dhdb_parent(d)) - 1;
			prev = d;
			d = dhdb_at(dhdb_parent(d), line);
			if (dhdb_type(prev) == DHDB_VALUE_NUMBER)
				dhdb_set_num(d, 0);
			else if (dhdb_type(prev) == DHDB_VALUE_BOOL)
				dhdb_set_bool(d, false);
			else if (dhdb_type(prev) == DHDB_VALUE_NULL)
				dhdb_set_null(d);
			else if (dhdb_type(prev) == DHDB_VALUE_STRING)
				dhdb_set_str(d, "");
		}
		if (c == KEY_BACKSPACE || c == 'n')
			dhdb_set_null(d);
		if (c == '0' && c <= '9')
			dhdb_set_num(d, c - 48);
		if (c == '[')
			dhdb_add(d, dhdb_create());
		if (c == '{')
			dhdb_set_obj(d, "0", dhdb_create());
		if (c == 't' || c == 'f')
			dhdb_set_bool(d, c == 't' ? true : false);
		if (c == 'q') {
			endwin();
			printf("%s\n", dhdb_to_json_pretty(root));
			dhdb_free(root);
			break;
		}
	}
}

static void
_print_cmds(dhdb_t *parent, dhdb_t *d)
{
	wmove(stdscr, 23, 0);
	wclrtoeol(stdscr);

	wprintw(stdscr, "%d/%d bytes", dhdb_size(_root), dhdb_size(d));

	if (parent)
		wprintw(stdscr, "<left> ");
	if (dhdb_is_container(d))
		wprintw(stdscr, "<right> ");
	if (dhdb_next(d))
		wprintw(stdscr, "<down> ");
	if (dhdb_prev(d))
		wprintw(stdscr, "<up> ");
	if (dhdb_parent(d))
		wprintw(stdscr, "<enter> ");
	if (dhdb_type(d) == DHDB_VALUE_UNDEFINED)
		wprintw(stdscr, "0-9 n t f \" { [ ");
	if (dhdb_type(d) == DHDB_VALUE_NUMBER)
		wprintw(stdscr, "0-9 . ");
	if (dhdb_type(d) == DHDB_VALUE_BOOL)
		wprintw(stdscr, "t f ");
	if (dhdb_type(d) == DHDB_VALUE_STRING)
		wprintw(stdscr, "<ascii> ");
	if (dhdb_type(d) != DHDB_VALUE_UNDEFINED)
		wprintw(stdscr, "<backspace> ");

	wclrtoeol(stdscr);
}

static void
_dhdb_print_val(dhdb_t *val, bool expand, int col, int line, int selected_line)
{
	dhdb_t *n, *selected_n;
	int i;

	wmove(stdscr, line, col);

	if (dhdb_name(val)) {
		wcolor_set(stdscr, 2, NULL);
		wprintw(stdscr, "%s ", dhdb_name(val));
		wcolor_set(stdscr, 0, NULL);
		wprintw(stdscr, ": ");
	}

	if (line == selected_line)
		wattron(stdscr, A_REVERSE);

	if (dhdb_type(val) == DHDB_VALUE_UNDEFINED) {
		wprintw(stdscr, "< undefined >");
	} else if (dhdb_type(val) == DHDB_VALUE_NUMBER) {
		wprintw(stdscr, "%f", dhdb_num(val));
	} else if (dhdb_type(val) == DHDB_VALUE_STRING) {
		wprintw(stdscr, "\"%s\"", dhdb_str(val));
	} else if (dhdb_type(val) == DHDB_VALUE_BOOL) {
		wprintw(stdscr, "%s", dhdb_num(val) ? "true" : "false");
	} else if (dhdb_type(val) == DHDB_VALUE_NULL) {
		wprintw(stdscr, "null");
	} else if (dhdb_is_container(val) && !expand) {
		if (dhdb_type(val) == DHDB_VALUE_ARRAY)
			wprintw(stdscr, "[");
		else
			wprintw(stdscr, "{");

		if (dhdb_first(val) == DHDB_VALUE_UNDEFINED)
			wprintw(stdscr, " < empty > ");
		else
			wprintw(stdscr, " < %d elements > ", dhdb_len(val));

		if (dhdb_type(val) == DHDB_VALUE_ARRAY)
			wprintw(stdscr, "]");
		else
			wprintw(stdscr, "}");
	} else if (dhdb_is_container(val)) {
		wattroff(stdscr, A_REVERSE);

		if (dhdb_type(val) == DHDB_VALUE_ARRAY)
			wprintw(stdscr, "[");
		else
			wprintw(stdscr, "{");

		n = dhdb_first(val);
		selected_n = NULL;
		i = 0;
		while (n) {
			if (i == selected_line)
				selected_n = n;
			_dhdb_print_val(n, false, 10, i + 1, selected_line + 1);
			i++;
			n = dhdb_next(n);
		}
		wmove(stdscr, line + (i + 1), col);

		if (dhdb_type(val) == DHDB_VALUE_ARRAY)
			wprintw(stdscr, "}");
		else
			wprintw(stdscr, "}");

		_dhdb_print_val(selected_n, false, 10, selected_line + 1,
		    selected_line + 1);
	}

	wattroff(stdscr, A_REVERSE);
	wclrtoeol(stdscr);
}

static void
_screen_init()
{
	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_CYAN, COLOR_BLACK);
	wcolor_set(stdscr, 0, NULL);
	touchwin(stdscr);
}
