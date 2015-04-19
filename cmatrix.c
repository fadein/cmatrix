/**************************************************************************
 *   cmatrix.c                                                            *
 *                                                                        *
 *   Copyright (C) 1999 Chris Allegretta                                  *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 1, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *   GNU General Public License for more details.                         *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                        *
 **************************************************************************/

#include "config.h"

#include <ccan/grab_file/grab_file.h>
#include <ccan/talloc/talloc.h> // For talloc_free()
#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else				/* Uh oh */
#include <curses.h>
#endif				/* CURSES_H */

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif				/* HAVE_SYS_IOCTL_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif				/* HAVE_UNISTD_H */

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif				/* HAVE_TERMIOS_H */

#ifdef HAVE_TERMIO_H
#include <termio.h>
#endif				/* HAVE_TERMIO_H */


typedef enum { NORMAL = 1, BOLD = 2 } boldness;

/* Matrix typedef */
typedef struct cmatrix {
    int val;
    boldness bold;
} cmatrix;

/* Global variables, unfortunately */
int console = 0, xwindow = 0;		/* Are we in the console? X? */
cmatrix **matrix = (cmatrix **) NULL;   /* The matrix has you */
int *length = NULL;			/* Length of cols in each line */
int *spaces = NULL;			/* spaces left to fill */
int *updates = NULL;			/* What does this do again? :) */
int bold   = -1,
    update = 4,
    mcolor = COLOR_GREEN,
    screensaver = 0,
    force = 0;

/* floating window of text */
char *filen = NULL;
char **text_buf = NULL;
int text_lines = 0, text_width = 0;
WINDOW *text_win = NULL;
char windowBorder = 0;

int va_system(const char *str, ...)
{
    va_list ap;
    char foo[133];

    va_start(ap, str);
    vsnprintf(foo, 132, str, ap);
    va_end(ap);
    return system(foo);
}

/* What we do when we're all set to exit */
RETSIGTYPE finish(int sigage)
{
    curs_set(1);
    clear();
    refresh();
    resetty();
    endwin();
#ifdef HAVE_CONSOLECHARS
    if (console)
	va_system("consolechars -d");
#elif defined(HAVE_SETFONT)
    if (console)
	va_system("setfont");
#endif
    exit(0);
}

/* What we do when we're all set to exit */
RETSIGTYPE c_die(char *msg, ...)
{
    va_list ap;

    curs_set(1);
    clear();
    refresh();
    resetty();
    endwin();
#ifdef HAVE_CONSOLECHARS
    if (console)
	va_system("consolechars -d");
#elif defined(HAVE_SETFONT)
    if (console)
	va_system("setfont");
#endif

    va_start(ap, msg);
    vfprintf(stderr, "%s", ap);
    va_end(ap);
    exit(0);
}

void usage(void)
{
    printf(" Usage: cmatrix -[bBFwhlsVx] [-u delay] [-C color] [-f file]\n"
	    " -b: Bold characters on\n"
	    " -B: All bold characters (overrides -b)\n"
	    " -F: Force the linux $TERM type to be on\n"
	    " -f [file]: write the contents of a file (- for STDIN) on a floating window\n"
	    " -w: Disable border around floating text window\n"
	    " -l: Linux mode (uses matrix console font)\n"
	    " -h: Print usage and exit\n"
	    " -n: No bold characters (overrides -b and -B, default)\n"
	    " -s: \"Screensaver\" mode, exits on first keystroke\n"
	    " -x: X window mode, use if your xterm is using mtx.pcf\n"
	    " -V: Print version information and exit\n"
	    " -u delay (0 - 10, default 4): Screen update delay\n"
	    " -C [color]: Use this color for matrix (default green)\n"
	    "\nText may be redirected to this process in lieu of specifying a filename with -f\n");
}

void version(void)
{
    printf(" CMatrix version %s by Chris Allegretta (compiled %s, %s)\n",
	   VERSION, __TIME__, __DATE__);
    printf(" Email: cmatrix@asty.org  Web: http://www.asty.org/cmatrix\n");
}

/* nmalloc from nano by Big Gaute */
void *nmalloc(size_t howmany, size_t howbig)
{
    void *r;

    /* Panic save? */

    if (!(r = calloc(howmany, howbig)))
	c_die("CMatrix: calloc: out of memory!");

    return r;
}

/* Initialize the global variables */
RETSIGTYPE var_init(void)
{
    int i, j;

    if (matrix != NULL)
	free(matrix);

    matrix = (cmatrix**)nmalloc(LINES + 1, sizeof(cmatrix));
    for (i = 0; i <= LINES; i++)
	matrix[i] = nmalloc(COLS, sizeof(cmatrix));

    if (length != NULL)
	free(length);
    length = nmalloc(COLS, sizeof(int));

    if (spaces != NULL)
	free(spaces);
    spaces = nmalloc(COLS, sizeof(int));

    if (updates != NULL)
	free(updates);
    updates = nmalloc(COLS, sizeof(int));

    /* Make the matrix */
    for (i = 0; i <= LINES; i++)
	for (j = 0; j < COLS; j += 2)
	    matrix[i][j].val = -1;

    for (j = 0; j <= COLS - 1; j += 2) {
	/* Set up spaces[] array of how many spaces to skip */
	spaces[j] = (int) rand() % LINES + 1;

	/* And length of the stream */
	length[j] = (int) rand() % (LINES - 3) + 3;

	/* Sentinel value for creation of new objects */
	matrix[1][j].val = ' ';

	/* And set updates[] array for update speed. */
	updates[j] = (int) rand() % 3 + 1;
    }

    /* make the floating window of text atop the matrix, if we have any text */
    if (text_lines > 0) {
	if (text_win)
	    delwin(text_win);

	text_win = newwin(text_lines + 0, text_width + 2,
		(LINES / 2) - (text_lines / 2),
		(COLS  / 2) - (text_width / 2));
    }
}

void handle_sigwinch(int s)
{
    char *tty = NULL;
    int fd = 0;
    int result = 0;
    struct winsize win;

    tty = ttyname(0);
    if (!tty)
	return;
    fd = open(tty, O_RDWR);
    if (fd == -1)
	return;
    result = ioctl(fd, TIOCGWINSZ, &win);
    if (result == -1)
	return;

    COLS = win.ws_col;
    LINES = win.ws_row;

#ifdef HAVE_RESIZETERM
    resizeterm(LINES, COLS);
#ifdef HAVE_WRESIZE
    if (wresize(stdscr, LINES, COLS) == ERR)
	c_die("Cannot resize window!");
#endif				/* HAVE_WRESIZE */
#endif				/* HAVE_RESIZETERM */

    var_init();

    /* Do these b/c width may have changed... */
    clear();
    refresh();
}

void handle_keypress(int keypress)
{
    switch (keypress) {
	case 'q':
	    finish(0);
	    break;
	case 'b':
	    bold = 1;
	    break;
	case 'B':
	    bold = 2;
	    break;
	case 'n':
	    bold = 0;
	    break;
	case 'w':
	    windowBorder = windowBorder ? 0 : ' ';
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    update = keypress - 48;
	    break;
	case '!':
	    mcolor = COLOR_RED;
	    break;
	case '@':
	    mcolor = COLOR_GREEN;
	    break;
	case '#':
	    mcolor = COLOR_YELLOW;
	    break;
	case '$':
	    mcolor = COLOR_BLUE;
	    break;
	case '%':
	    mcolor = COLOR_MAGENTA;
	    break;
	case '^':
	    mcolor = COLOR_CYAN;
	    break;
	case '&':
	    mcolor = COLOR_WHITE;
	    break;
    }
}

void do_opts(int argc, char *argv[])
{
    int optchr;

    /* Many thanks to morph- (morph@jmss.com) for this getopt patch */
    opterr = 0;
    while ((optchr = getopt(argc, argv, "bBf:FhlnswxVu:C:")) != EOF) {
	switch (optchr) {
	    case 's':
		screensaver = 1;
		break;
	    case 'b':
		if (bold != 2 && bold != 0)
		    bold = 1;
		break;
	    case 'B':
		if (bold != 0)
		    bold = 2;
		break;
	    case 'C':
		if (!strcasecmp(optarg, "green"))
		    mcolor = COLOR_GREEN;
		else if (!strcasecmp(optarg, "red"))
		    mcolor = COLOR_RED;
		else if (!strcasecmp(optarg, "blue"))
		    mcolor = COLOR_BLUE;
		else if (!strcasecmp(optarg, "white"))
		    mcolor = COLOR_WHITE;
		else if (!strcasecmp(optarg, "yellow"))
		    mcolor = COLOR_YELLOW;
		else if (!strcasecmp(optarg, "cyan"))
		    mcolor = COLOR_CYAN;
		else if (!strcasecmp(optarg, "magenta"))
		    mcolor = COLOR_MAGENTA;
		else if (!strcasecmp(optarg, "black"))
		    mcolor = COLOR_BLACK;
		else {
		    printf(" Invalid color selection\n Valid "
			    "colors are green, red, blue, "
			    "white, yellow, cyan, magenta " "and black.\n");
		    exit(1);
		}
		break;
	    case 'F':
		force = 1;
		break;
	    case 'f':
		filen = strdup(optarg);
		break;
	    case 'w':
		windowBorder = ' ';
		break;
	    case 'l':
		console = 1;
		break;
	    case 'n':
		bold = 0;
		break;
	    case 'h':
	    case '?':
		usage();
		exit(0);
	    case 'u':
		update = atoi(optarg);
		break;
	    case 'x':
		xwindow = 1;
		break;
	    case 'V':
		version();
		exit(0);
	}
    }
}

char** grab_text(char* file, int screenH, int *num_lines, int *max_cols)
{
    char *buf, **lines, *p;
    size_t fsize;
    int i = 0;

    if (file == NULL || (1 == strlen(file) && *file == '-')) {
	// re-open /dev/tty and re-associate it to STDIN
	int newfd;

	buf = grab_file(NULL, NULL, &fsize);

	if (-1 == (newfd = open("/dev/tty", O_RDWR))) {
	    c_die("Couldn't re-open /dev/tty\n");
	}
	else {
	    dup2(newfd, 0);
	}
    }
    else {
	buf = grab_file(NULL, file, &fsize);
    }

    // allocate an array of char* that is at least as tall as the screen
    lines = talloc_array(buf, char*, screenH + 1);

    *max_cols = 0;
    p = strsep(&buf, "\n");
    while (p != NULL && i < screenH) {
	lines[i++] = p;
	if (strlen(p) > *max_cols)
	    *max_cols = strlen(p) + 1;
	p = strsep(&buf, "\n");
    }
    *num_lines = i + 1;
    return lines;
}

void update_matrix(int count, int randnum, int randmin)
{
    int line,
	tail,
	firstcoldone,
	y;

    // for every other column...
    for (int col = 0; col < COLS; col += 2) {
	//
	// Update the matrix array
	//
	// if count is greater than this column's update
	if (count > updates[col]) {

	    if (matrix[0][col].val == -1 && matrix[1][col].val == ' ') {
		if (spaces[col] > 0) {
		    spaces[col]--;
		}
		else {
		    length[col] = (int) rand() % (LINES - 3) + 3;
		    matrix[0][col].val = (int) rand() % randnum + randmin;

		    if ((int) rand() % 2 == 1)
			matrix[0][col].bold = BOLD;

		    spaces[col] = (int) rand() % LINES + 1;
		}
	    }

	    line = 0;
	    y = 0;
	    firstcoldone = 0;
	    while (line <= LINES) {

		/* Skip over spaces */
		while (line <= LINES && (matrix[line][col].val == ' ' ||
			    matrix[line][col].val == -1))
		    line++;

		if (line > LINES)
		    break;

		/* Go to the head of this column */
		tail = line;
		y = 0;
		while (line <= LINES && // while we don't go off the bottom of screen
			(matrix[line][col].val != ' ' && // and the current char isn't a
			 matrix[line][col].val != -1)) { // blank or -1
		    line++;
		    y++;
		}

		if (line > LINES) {
		    matrix[tail][col].val = ' ';
		    matrix[LINES][col].bold = NORMAL;
		    continue;
		}

		// assign a random character to this matrix cell
		matrix[line][col].val = (int) rand() % randnum + randmin;

		// update this cell's boldness based on the one above it
		// if the cell above is bold, make it normal
		// and bold me
		if (matrix[line - 1][col].bold == BOLD) {
		    matrix[line - 1][col].bold = NORMAL;
		    matrix[line][col].bold = BOLD;
		}

		// If we're at the top of the column and it's reached its
		// full length (about to start moving down), we do this
		// to get it moving.  This is also how we keep segments not
		// already growing from growing accidentally =>
		if (y > length[col] || firstcoldone) {
		    matrix[tail][col].val = ' ';
		    matrix[0][col].val = -1;
		}
		firstcoldone = 1;
		line++;
	    }
	} // if (count > updates[col])

    }
}

void draw_matrix()
{
    int line,
	tail,
	y;

    // for every other column...
    for (int col = 0; col < COLS; col += 2) {

	//
	//
	// redraw this column on the screen with curses functions
	// (would it perhaps be moar efficient instead of drawing
	// each column, to draw each line as a string with addstr()
	// and only setting those bold chars as needed with chgat()?)
	// well, in some modes, there are an awful lot of bold chars, and not just
	// the heads of columns.
	y = 1;
	tail = LINES;
	for (line = y; line <= tail; line++) {
	    move(line - y, col);

	    if (matrix[line][col].val == 0 || matrix[line][col].bold == BOLD) {
		if (console || xwindow)
		    attron(A_ALTCHARSET);
		attron(COLOR_PAIR(COLOR_WHITE));
		if (bold)
		    attron(A_BOLD);
		if (matrix[line][col].val == 0) {
		    if (console || xwindow)
			addch(183);
		    else
			addch('&');
		} else
		    addch(matrix[line][col].val);

		attroff(COLOR_PAIR(COLOR_WHITE));
		if (bold)
		    attroff(A_BOLD);
		if (console || xwindow)
		    attroff(A_ALTCHARSET);
	    }
	    else {
		attron(COLOR_PAIR(mcolor));
		if (matrix[line][col].val == 1) {
		    if (bold)
			attron(A_BOLD);
		    addch('|');
		    if (bold)
			attroff(A_BOLD);
		}
		else {
		    if (console || xwindow)
			attron(A_ALTCHARSET);
		    if (bold == 2 ||
			    (bold == 1 && matrix[line][col].val % 2 == 0))
			attron(A_BOLD);
		    if (matrix[line][col].val == -1)
			addch(' ');
		    else
			addch(matrix[line][col].val);
		    if (bold == 2 ||
			    (bold == 1 && matrix[line][col].val % 2 == 0))
			attroff(A_BOLD);
		    if (console || xwindow)
			attroff(A_ALTCHARSET);
		}
		attroff(COLOR_PAIR(mcolor));
	    }
	}
    }
    wnoutrefresh(stdscr);
}

int main(int argc, char *argv[])
{
    int count = 0,
	randnum,
	randmin,
	keypress;

    char *oldtermname = NULL, *syscmd = NULL;

    /* Set up values for random number generation */
    if (console || xwindow) {
	randnum = 51;
	randmin = 166;
    } else {
	randnum = 93;
	randmin = 33;
    }

    do_opts(argc, argv);

    /* If bold hasn't been turned on or off yet, assume off */
    if (bold == -1)
	bold = 0;

    if (force && strcmp("linux", getenv("TERM"))) {
	/* Portability wins out here, apparently putenv is much more
	   common on non-Linux than setenv */
	oldtermname = getenv("TERM");
	putenv("TERM=linux");
    }

    initscr();
    savetty();
    nonl();
    cbreak();
    noecho();
    timeout(0);
    leaveok(stdscr, TRUE);
    curs_set(0);
    signal(SIGINT, finish);
    signal(SIGWINCH, handle_sigwinch);

#ifdef HAVE_CONSOLECHARS
    if (console)
	if (va_system("consolechars -f matrix") != 0) {
	    c_die
		(" There was an error running consolechars. Please make sure the\n"
		 " consolechars program is in your $PATH.  Try running \"setfont matrix\" by hand.\n");
	}
#elif defined(HAVE_SETFONT)
    if (console)
	if (va_system("setfont matrix") != 0) {
	    c_die
		(" There was an error running setfont. Please make sure the\n"
		 " setfont program is in your $PATH.  Try running \"setfont matrix\" by hand.\n");
	}
#endif

    if (has_colors()) {
	start_color();
	/* Add in colors, if available */
#ifdef HAVE_USE_DEFAULT_COLORS
	if (use_default_colors() != ERR) {
	    init_pair(COLOR_BLACK, -1, -1);
	    init_pair(COLOR_GREEN, COLOR_GREEN, -1);
	    init_pair(COLOR_WHITE, COLOR_WHITE, -1);
	    init_pair(COLOR_RED, COLOR_RED, -1);
	    init_pair(COLOR_CYAN, COLOR_CYAN, -1);
	    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
	    init_pair(COLOR_BLUE, COLOR_BLUE, -1);
	    init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
	} else {
#else
	{
#endif
	    init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
	    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
	    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
	    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
	    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
	    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
	    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
	    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
	}
    }

    // draw text from STDIN or the filename given with the -f flag in
    // its own floating window
    if (filen || !isatty(fileno(stdin))) {
	text_buf = grab_text(filen, LINES, &text_lines, &text_width);
    }

    srand(time(NULL));

    var_init();

    while (1) {

	// Handle keypresess; wgetch() actually forces a screen refresh,
	// but at this point there shouldn't be anything left to refresh
	if ((keypress = getch()) != ERR) {
	    if (screensaver == 1)
		finish(0);
	    else
		handle_keypress(keypress);
	}

	// increment and clamp count so that it stays between [1..3]
	count++;
	if (count > 4)
	    count = 1;

	update_matrix(count, randnum, randmin);

	// draw the matrix to ncurses' internal buffer, but don't push the
	// changes to the terminal yet
	draw_matrix();

	// draw the floating text within its own window, but don't force a refresh yet
	if (text_win) {
	    wattron(text_win, A_BOLD|COLOR_PAIR(mcolor));
	    wborder(text_win, windowBorder, windowBorder,
		    windowBorder, windowBorder, windowBorder, windowBorder,
		    windowBorder, windowBorder);
	    for (int i = 0; i < text_lines - 1; ++i)
		if (OK != mvwprintw(text_win, i+1, 1, "%s", text_buf[i]))
		    c_die("mvwprintw(text_win) returned an ERR!\n");
	    wnoutrefresh(text_win);
	}

	// now we may push the completed picture out to the terminal
	doupdate();
	napms(update * 10);
    }

    if (oldtermname) {
	syscmd = nmalloc((strlen(oldtermname) + 6), sizeof(char *));
	sprintf(syscmd, "TERM=%s", oldtermname);
	putenv(syscmd);
    }
    finish(0);
    return 0;
}

/* vim: set tabstop=8: */
