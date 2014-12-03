#include <stdio.h>
#include <curses.h>
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <string.h>

struct zoom_stack {
	struct zoom_level {
		double ymin, ymax, xmin, xmax;
	} zoom_level;
	struct zoom_stack *prev;
};

enum CHOICE { XMIN, XMAX, YMIN, YMAX, COLOR, ITER, ZOOMF, BAIL, POW, XBGSCR, YBGSCR };

#define MENU_HEIGHT    15
#define MENU_WIDTH     50
#define MENU_OFFSET_Y  3
#define MENU_OFFSET_X  1
#define INPUT_OFFSET   22
#define MAX_INPUT      25
#define NCHOICES       11

#define DEF_XMIN   -2
#define DEF_XMAX   1
#define DEF_YMIN   -1
#define DEF_YMAX   1
#define DEF_ITER   100
#define DEF_BAIL   2
#define DEF_POW    2
#define DEF_ZF     2
#define DEF_COLORS " .:~#"
#define DEF_BGSCRX 20
#define DEF_BGSCRY 15

#define ystep ((ymax - ymin) / height)
#define xstep ((xmax - xmin) / width)

// Global variables aren't really evil here because there's going to be shared
// mutable state and otherwise it'd be in a godlike struct, which is basically
// the same thing.

int height;
int width;

double xmin            = DEF_XMIN;
double xmax            = DEF_XMAX;
double ymin            = DEF_YMIN;
double ymax            = DEF_YMAX;

int iter               = DEF_ITER;
int bailout            = DEF_BAIL;
double power           = DEF_POW;
double zoom_factor     = DEF_ZF;
char colors[MAX_INPUT] = DEF_COLORS;
int big_scroll_x       = DEF_BGSCRX;
int big_scroll_y       = DEF_BGSCRY;

struct zoom_stack *zstack = NULL;

int
get_color(double complex c, int max_iter, int ncolors)
{
	double complex z = 0;
	for (int i = 0; i < max_iter; ++i) {
		z = z*z + c;
		if (cabs(z) > bailout || i + 1 == max_iter)
			return i % ncolors;
	}

	return 0; // shut up the compiler; we'll never get here
}

int
get_color_generic(double complex c, int max_iter, int ncolors)
{
	double complex z = 0;
	for (int i = 0; i < max_iter; ++i) {
		z = cpow(z, power) + c;
		if (cabs(z) > bailout || i + 1 == max_iter)
			return i % ncolors;
	}

	return 0;
}

double complex
complex_value(int i, int j)
{
	return (xmin + j*xstep) + (ymax - i*ystep)*I;
}

void
draw(void)
{
	// Yes, equality with floats is evil, but it's OK becuase we only need
	// to worry about one value, which I've checked works.
	int (*color_alg)(double complex, int, int) = power == 2 ? get_color : get_color_generic;
	int ncolors = strlen(colors);

	getmaxyx(stdscr, height, width);
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			mvaddch(i, j, colors[color_alg(complex_value(i, j), iter, ncolors)]);
		}
	}

	refresh();
}

void
scrollup(void)
{
	ymin += ystep;
	ymax += ystep;
}

void
scrolldown(void)
{
	ymin -= ystep;
	ymax -= ystep;
}

void
scrollleft(void)
{
	xmin -= xstep;
	xmax -= xstep;
}

void
scrollright(void)
{
	xmin += xstep;
	xmax += xstep;
}

void
zoom_stack_push(struct zoom_stack **stk, struct zoom_level lvl)
{
	struct zoom_stack *newstk = malloc(sizeof(struct zoom_stack));
	newstk->zoom_level = lvl;
	newstk->prev = *stk;
	*stk = newstk;
}

struct zoom_level *
zoom_stack_pop(struct zoom_stack **stk)
{
	if (!stk || !(*stk)) return NULL;
	struct zoom_level *ret = &((*stk)->zoom_level); // guaranteed to be the funkiest
	*stk = (*stk)->prev;                            // pointer manipulation you've
	return ret;                                     // seen all day
}

void
hilight_rect(int starty, int startx, int endy, int endx)
{
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			mvchgat(i, j, 1, A_NORMAL, 0, NULL);
		}
	}

	int ay = endy > starty ? starty : endy;
	int by = endy > starty ? endy : starty;
	int ax = endx > startx ? startx : endx;
	int bx = endx > startx ? endx : startx;
	for (int i = ay; i <= by; ++i) {
		for (int j = ax; j <= bx; ++j) {
			mvchgat(i, j, 1, A_REVERSE, 0, NULL);
		}
	}

	move(endy, endx);
}

void
zoom_in_center(void)
{
	zoom_stack_push(&zstack, (struct zoom_level) { ymin, ymax, xmin, xmax });

	ymin += (ymax - ymin) * 0.5 * (1 - (1 / zoom_factor));
	ymax -= (ymax - ymin) * 0.5 * (1 - (1 / zoom_factor));
	xmin += (xmax - xmin) * 0.5 * (1 - (1 / zoom_factor));
	xmax -= (xmax - xmin) * 0.5 * (1 - (1 / zoom_factor));
}

void zoom_out_center(void)
{
	ymin -= 0.5 * (ymax - ymin) * (zoom_factor - 1);
	ymax += 0.5 * (ymax - ymin) * (zoom_factor - 1);
	xmin -= 0.5 * (xmax - xmin) * (zoom_factor - 1);
	xmax += 0.5 * (xmax - xmin) * (zoom_factor - 1);

	struct zoom_level *junk = zoom_stack_pop(&zstack);
	free(junk);
}

void
zoom_out(void)
{
	struct zoom_level *lvl = zoom_stack_pop(&zstack);
	if (!lvl) return;

	ymin = lvl->ymin;
	ymax = lvl->ymax;
	xmin = lvl->xmin;
	xmax = lvl->xmax;

	free(lvl);
}

void
zoom_box(void)
{
	move(height / 2, width / 2);
	refresh();

	bool box_select = false;
	int ch, y, x, box_start_y = 0, box_start_x = 0;
	while (true) {
		getyx(stdscr, y, x);
		ch = getch();
		switch (ch) {
		case KEY_LEFT:
		case 'h':
			move(y, --x);
			break;
		case KEY_DOWN:
		case 'j':
			move(++y, x);
			break;
		case KEY_UP:
		case 'k':
			move(--y, x);
			break;
		case KEY_RIGHT:
		case 'l':
			move(y, ++x);
			break;
		case 'H':
			for (int i = 0; i < big_scroll_x; ++i)
				move(y, --x);
			break;
		case 'J':
			for (int i = 0; i < big_scroll_y; ++i)
				move(++y, x);
			break;
		case 'K':
			for (int i = 0; i < big_scroll_y; ++i)
				move(--y, x);
			break;
		case 'L':
			for (int i = 0; i < big_scroll_x; ++i)
				move(y, ++x);
			break;
		case 'z':
			return;
			break;
		case 'q':
			ungetch('q'); // Quit when we get back to main()
			return;
			break;
		case ' ':
			if (box_select) {
				// new_ vars are needed because complex_value() uses xmin, ymin, etc.
				double new_ymin, new_xmin, new_ymax, new_xmax;
				zoom_stack_push(&zstack, (struct zoom_level) { ymin, ymax, xmin, xmax });

				new_ymin = cimag(y > box_start_y ?
						complex_value(y, x) :
						complex_value(box_start_y, box_start_x));
				new_xmin = creal(x > box_start_x ?
						complex_value(box_start_y, box_start_x) :
						complex_value(y, x));
				new_ymax = cimag(y > box_start_y ?
						complex_value(box_start_y, box_start_x) :
						complex_value(y, x));
				new_xmax = creal(x > box_start_x ?
						complex_value(y, x) :
						complex_value(box_start_y, box_start_x));

				ymin = new_ymin; ymax = new_ymax;
				xmin = new_xmin; xmax = new_xmax;

				return;
			} else {
				box_start_y = y;
				box_start_x = x;
				box_select = true;
			}
			break;
		default: break;
		}

		if (box_select)
			hilight_rect(box_start_y, box_start_x, y, x);
		refresh();
	}
}

void
change_var(WINDOW *win, enum CHOICE choice)
{
	int i;
	double d;
	char buf[MAX_INPUT];

	wmove(win, MENU_OFFSET_Y + choice, INPUT_OFFSET);
	wclrtoeol(win);
	for (int i = 0; i < MENU_WIDTH - 2; ++i)
		mvwchgat(win, MENU_OFFSET_Y + choice, MENU_OFFSET_X + i, 1, A_REVERSE, 0, NULL);

	echo();
	wattr_on(win, A_REVERSE, NULL);
	mvwgetnstr(win, MENU_OFFSET_Y + choice, INPUT_OFFSET, buf, sizeof(buf));
	wattr_off(win, A_REVERSE, NULL);
	noecho();

	sscanf(buf, "%lf", &d);
	sscanf(buf, "%d", &i);

	if (choice == XMIN || choice == XMAX || choice == YMIN || choice == YMAX)
		zoom_stack_push(&zstack, (struct zoom_level) { ymin, ymax, xmin, xmax });

	switch (choice) {
	case XMIN: xmin = d; break;
	case XMAX: xmax = d; break;
	case YMIN: ymin = d; break;
	case YMAX: ymax = d; break;
	case ZOOMF: zoom_factor = d; break;
	case POW: power = d; break;
	case ITER: iter = i; break;
	case BAIL: bailout = i; break;
	case XBGSCR: big_scroll_x = i; break;
	case YBGSCR: big_scroll_y = i; break;
	case COLOR: strncpy(colors, buf, MAX_INPUT - 1); break;
	default: break;
	}
}

void
draw_menu(WINDOW *win, int choice)
{
	mvwprintw(	win, 1, 0,
			" VARIABLES\n\n"
			" Min x:               %f\n"
			" Max x:               %f\n"
			" Min y:               %f\n"
			" Max y:               %f\n"
			" Colors:              \'%s\'\n"
			" Iterations:          %d\n"
			" Zoom factor:         %f\n"
			" Bailout value:       %d\n"
			" Mandelbrot power:    %f\n"
			" X big scroll amount: %d\n"
			" Y big scroll amount: %d\n",
			xmin,
			xmax,
			ymin,
			ymax,
			colors,
			iter,
			zoom_factor,
			bailout,
			power,
			big_scroll_x,
			big_scroll_y);

	for (int i = 0; i < MENU_WIDTH - 2; ++i)
		mvwchgat(win, MENU_OFFSET_Y + choice, MENU_OFFSET_X + i, 1, A_REVERSE, 0, NULL);

	box(win, 0, 0);
	wrefresh(win);
}

void
set_defaults(void)
{
	xmin         = DEF_XMIN;
	xmax         = DEF_XMAX;
	ymin         = DEF_YMIN;
	ymax         = DEF_YMAX;

	iter         = DEF_ITER;
	bailout      = DEF_BAIL;
	power        = DEF_POW;
	zoom_factor  = DEF_ZF;
	big_scroll_x = DEF_BGSCRX;
	big_scroll_y = DEF_BGSCRY;

	strncpy(colors, DEF_COLORS, MAX_INPUT - 1);

	struct zoom_level *junk;
	while ((junk = zoom_stack_pop(&zstack)) != NULL)
		free(junk);
}

void
menu(void)
{
	WINDOW *win;
	int startx = (height - MENU_HEIGHT) / 2, starty = (width - MENU_WIDTH) / 2;

	win = newwin(MENU_HEIGHT, MENU_WIDTH, startx, starty);
	wrefresh(win);

	int ch, choice = 0;
	while (true) {
		draw_menu(win, choice);
		ch = getch();
		switch (ch) {
		case KEY_DOWN:
		case 'j':
			choice = choice + 1 >= NCHOICES ? 0 : choice + 1;
			break;
		case KEY_UP:
		case 'k':
			choice = choice - 1 < 0 ? NCHOICES - 1 : choice - 1;
			break;
		case 'm':
			goto end;
			break;
		case ' ':
			change_var(win, choice);
			break;
		case 'd':
			set_defaults();
			break;
		case 'q':
			ungetch('q');
			goto end;
			break;
		default: break;
		}

		draw();
	}

end:
	delwin(win);
}

int
main(int argc, char *argv[])
{
	(void)argc; // get rid of -Wextra warning
	(void)argv;

	// Various options everyone uses with curses
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	draw();

	int ch;
	while (true) {
		ch = getch();
		switch (ch) {
		case KEY_LEFT:
		case 'h':
			scrollleft();
			break;
		case KEY_DOWN:
		case 'j':
			scrolldown();
			break;
		case KEY_UP:
		case 'k':
			scrollup();
			break;
		case KEY_RIGHT:
		case 'l':
			scrollright();
			break;
		case 'H':
			for (int i = 0; i < big_scroll_x; ++i)
				scrollleft();
			break;
		case 'J':
			for (int i = 0; i < big_scroll_y; ++i)
				scrolldown();
			break;
		case 'K':
			for (int i = 0; i < big_scroll_y; ++i)
				scrollup();
			break;
		case 'L':
			for (int i = 0; i < big_scroll_x; ++i)
				scrollright();
			break;
		case '+':
		case '=':
			zoom_in_center();
			break;
		case '-':
		case '_':
			zoom_out();
			break;
		case 'z':
			zoom_box();
			break;
		case 'o':
			zoom_out_center();
			break;
		case 'm':
			menu();
			break;
		case 'd':
			set_defaults();
			break;
		case 'q':
			goto end;
			break;
		default: break;
		}

		draw();
	}

	struct zoom_level *junk;
end:
	while ((junk = zoom_stack_pop(&zstack)) != NULL)
		free(junk);

	endwin();
	return 0;
}
