#include <stdio.h>
#include <curses.h>
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>

struct zoom_stack {
	struct zoom_level {
		double ymin, ymax, xmin, xmax;
	} zoom_level;
	struct zoom_stack *prev;
};

#define ystep ((ymax - ymin) / height)
#define xstep ((xmax - xmin) / width)

int height;
int width;

double xmin = -2;
double xmax = 1;
double ymin = -1;
double ymax = 1;

int iter = 100;
int bailout = 2;
double power = 2;
char colors[] = " .:~#";

struct zoom_stack *zstack = NULL;

int iter_color(double complex c, int max_iter, int ncolors)
{
	double complex z = 0;
	for (int i = 0; i < max_iter; ++i) {
		z = z*z + c;
		if (cabs(z) > bailout || i + 1 == max_iter)
			return i % ncolors;
	}

	return 0; // shut up the compiler; we'll never get here
}

int iter_color_generic(double complex c, double power, int max_iter, int ncolors)
{
	double complex z = 0;
	for (int i = 0; i < max_iter; ++i) {
		z = cpow(z,power) + c;
		if (cabs(z) > bailout || i + 1 == max_iter)
			return i % ncolors;
	}

	return 0; // shut up the compiler; we'll never get here
}

double complex complex_value(int i, int j)
{
	return (xmin + j*xstep) + (ymax - i*ystep)*I;
}

void draw(void)
{
	getmaxyx(stdscr, height, width);
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			mvaddch(i, j, colors[iter_color(complex_value(i, j), iter, sizeof(colors) - 1)]);
		}
	}

	refresh();
}

void scrollup(void)
{
	ymin += ystep;
	ymax += ystep;
}

void scrolldown(void)
{
	ymin -= ystep;
	ymax -= ystep;
}

void scrollleft(void)
{
	xmin -= xstep;
	xmax -= xstep;
}

void scrollright(void)
{
	xmin += xstep;
	xmax += xstep;
}

void zoom_stack_push(struct zoom_stack **stk, struct zoom_level lvl)
{
	struct zoom_stack *newstk = malloc(sizeof(struct zoom_stack));
	newstk->zoom_level = lvl;
	newstk->prev = *stk;
	*stk = newstk;
}

struct zoom_level *zoom_stack_pop(struct zoom_stack **stk)
{
	if (!stk || !(*stk)) return NULL;
	struct zoom_level *ret = &((*stk)->zoom_level); // guaranteed to be the funkiest
	*stk = (*stk)->prev;                            // pointer manipulation you've
	return ret;                                     // seen all day
}

// TODO: fix all the fancy zoom stuff
void hilight_rect(int start_y, int start_x, int end_y, int end_x)
{
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			mvchgat(i, j, 1, A_NORMAL, 0, NULL);
		}
	}

	int ay = end_y > start_y ? start_y : end_y;
	int by = end_y > start_y ? end_y : start_y;
	int ax = end_x > start_x ? start_x : end_x;
	int bx = end_x > start_x ? end_x : start_x;
	for (int i = ay; i <= by; ++i) {
		for (int j = ax; j <= bx; ++j) {
			mvchgat(i, j, 1, A_REVERSE, 0, NULL);
		}
	}
}

void zoom(void)
{
	move(height / 2, width / 2);
	refresh();

	bool box_select = false;
	double old_ymin = ymin, old_ymax = ymax, old_xmin = xmin, old_xmax = xmax;
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
		// TODO: this doesn't work right
		case KEY_BACKSPACE:
			return;
			break;
		case ' ':
			if (box_select) {
				ymin = cimag(y > box_start_y ?
						complex_value(y, x) :
						complex_value(box_start_y, box_start_x));
				xmin = creal(x > box_start_x ?
						complex_value(box_start_y, box_start_x) :
						complex_value(y, x));
				ymax = cimag(y > box_start_y ?
						complex_value(box_start_y, box_start_x) :
						complex_value(y, x));
				xmax = creal(x > box_start_x ?
						complex_value(y, x) :
						complex_value(box_start_y, box_start_x));

				if (xmax - xmin == 0 || ymax - ymin == 0) {
					ymin = old_ymin; ymax = old_ymax;
					xmin = old_xmin; xmax = old_xmax;
				} else {
					// TODO: this doesn't seem to work right
					zoom_stack_push(&zstack, (struct zoom_level) { ymin, ymax, xmin, xmax });
				}

				return;
			} else {
				box_start_y = y;
				box_start_x = x;
				box_select = true;
			}
			break;
		}

		if (box_select)
			hilight_rect(box_start_y, box_start_x, y, x);
		refresh();
	}
}

// TODO: fix the math
void zoom_center(double zoom_factor)
{
	zoom_stack_push(&zstack, (struct zoom_level) { ymin, ymax, xmin, xmax });
	ymin += (ymax - ymin) / (zoom_factor * 2);
	ymax -= (ymax - ymin) / (zoom_factor * 2);
	xmin += (xmax - xmin) / (zoom_factor * 2);
	xmax -= (xmax - xmin) / (zoom_factor * 2);
}

void zoom_out(void)
{
	struct zoom_level *lvl = zoom_stack_pop(&zstack);
	if (!lvl) return;

	ymin = lvl->ymin;
	ymax = lvl->ymax;
	xmin = lvl->xmin;
	xmax = lvl->xmax;

	free(lvl);
}

int main(int argc, char *argv[])
{
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
		case '+':
		case '=':
			zoom_center(2);
			break;
		case '-':
		case '_':
			zoom_out();
			break;
		case 'z':
			zoom();
			break;
		case 'q':
			goto end;
			break;
		}

		draw();
	}

end:
	endwin();
	return 0;
}
