#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

#define N 20

/*
 * char to be printed ('*' = has bomb, '-' = covered)
 * nearby = -1 if bomb. Else it is the number of bombs near the cell.
 */
typedef struct {
	char sign;
	int nearby;
} grid;

static void screen_init(grid a[][N], int rowtot, int coltot);
static void screen_end(int rowtot, int coltot);
static void grid_init(grid a[][N]);
static void num_bombs(void);
static int main_cycle(grid a[][N], int *i, int *k, int *victory, int *correct, int rowtot, int coltot);
static void cascadeuncover(grid a[][N], int i, int k);
static int checknear(grid a[][N], int i, int k);
static void manage_space_press(grid a[][N], int i, int k, int *victory);
static void manage_enter_press(grid a[][N], int i, int k, int *correct, int rowtot);
static void victory_check(grid a[][N], int victory, int correct, int rowtot, int coltot);

/* Global variables */
int horiz_space, vert_space; /* scaled values to fit terminal size */
int row, col, bombs;

int main(void)
{
	int i = 0, k = 0, victory = 1, correct = 1;
    int rowtot, coltot;
	grid a[N][N];
	srand(time(NULL));
	num_bombs();
	grid_init(a);
    initscr();
    getmaxyx(stdscr, rowtot, coltot);
	/* check terminal size */
	if ((rowtot < N + 4) || (coltot < N)) {
		clear();
		endwin();
		printf("This screen has %d rows and %d columns. Enlarge it.\n", rowtot, coltot);
		printf("You need at least %d rows and %d columns.\n", N + 4, N);
		return 1;
	}
	screen_init(a, rowtot, coltot);
	while ((victory) && (bombs > 0)) {
		if (!main_cycle(a, &i, &k, &victory, &correct, rowtot, coltot))
			return 0;
	}
	victory_check(a, victory, correct, rowtot, coltot);
	return 0;
}

static void screen_init(grid a[][N], int rowtot, int coltot)
{
	int i, k;
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_YELLOW, COLOR_BLACK);
	init_pair(4, COLOR_BLUE, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_CYAN, COLOR_BLACK);
	raw();
	noecho();
	keypad(stdscr, TRUE);
	/* -4 to leave vertical space to the 3 helper lines below. */
	vert_space = (rowtot - 4) / (N - 1);
	horiz_space = coltot / (N - 1);
	/* print grid centered */
	row = (rowtot - 4 - (N - 1) * vert_space) / 2;
	col = (coltot - (N - 1) * horiz_space) / 2;
	for (i = 0; i < N; i++) {
		for (k = 0; k < N; k++)
			mvprintw(row + i * vert_space, col + k * horiz_space, "%c", a[i][k].sign);
	}
	mvprintw(rowtot - 1, 1, "Enter to put a bomb (*). Space to uncover.\n");
	mvprintw(rowtot - 2, 1, "F2 anytime to *rage* quit.\n");
	mvprintw(rowtot - 3, 1, "Still %d bombs.\n", bombs);
}

static void screen_end(int rowtot, int coltot)
{
	char exitmsg[] = "Leaving...bye! See you later :)";
	clear();
	attron(COLOR_PAIR(rand()%6 + 1));
	mvprintw(rowtot / 2, (coltot - strlen(exitmsg)) / 2, "%s", exitmsg);
	refresh();
	sleep(1);
	attroff(COLOR_PAIR);
	endwin();
}

static void grid_init(grid a[][N])
{
	int i, k;
	for (i = 0; i < bombs; i++) {
		do {
			row = rand()%N;
			col = rand()%N;
		} while (a[row][col].nearby == -1);
		a[row][col].nearby = -1;
	}
	for (i = 0; i < N; i++) {
		for (k = 0; k < N; k++) {
			a[i][k].sign = '-';
			if (a[i][k].nearby != -1)
				a[i][k].nearby = checknear(a, i, k);
		}
	}
}

static void num_bombs(void)
{
	printf("Select level.\n1 for easy, 2 for medium, 3 for hard, 4 for...good luck!.\n");
	scanf("%d", &bombs);
	switch (bombs) {
	case 1:
		bombs = 25;
		break;
	case 2:
		bombs = 40;
		break;
	case 3:
		bombs = 65;
		break;
	case 4:
		bombs = 80;
		break;
	default:
		return num_bombs();
	}
}

static int main_cycle(grid a[][N], int *i, int *k, int *victory, int *correct, int rowtot, int coltot)
{
	move(row + *i * vert_space, col + *k * horiz_space);
	refresh();
	switch (getch()) {
	case KEY_LEFT:
		if (*k != 0)
			(*k)--;
		break;
	case KEY_RIGHT:
		if (*k != N-1)
			(*k)++;
		break;
	case KEY_UP:
		if (*i != 0)
			(*i)--;
		break;
	case KEY_DOWN:
		if (*i != N-1)
			(*i)++;
		break;
	case 32: /* space to uncover */
		if (a[*i][*k].sign == '-')
			manage_space_press(a, *i, *k, victory);
		break;
	case 10: /* Enter to  identify a bomb */
		if ((a[*i][*k].sign == '*') || (a[*i][*k].sign == '-'))
			manage_enter_press(a, *i, *k, correct, rowtot);
		break;
	case KEY_F(2): /* f2 to exit */
		screen_end(rowtot, coltot);
		return 0;
	}
	return 1;
}

static void cascadeuncover(grid a[][N], int i, int k)
{
	int m, n;
	if ((i >= 0) && (i < N) && (k >= 0) && (k < N) && (a[i][k].sign == '-')) {
		a[i][k].sign = '0' + a[i][k].nearby;
		move(row + i * vert_space, col + k * horiz_space);
		if (a[i][k].nearby != 0) {
			if (a[i][k].nearby >= 4)
				attron(COLOR_PAIR(1));
			else
				attron(COLOR_PAIR(a[i][k].nearby + 3));
			printw("%c", a[i][k].sign);
			attroff(COLOR_PAIR);
		} else {
			attron(A_BOLD);
			attron(COLOR_PAIR(3));
			printw("%c", a[i][k].sign);
			attroff(COLOR_PAIR);
			attroff(A_BOLD);
			for (m = -1; m < 2; m++) {
				for (n = -1; n < 2; n++)
					cascadeuncover(a, i + m, k + n);
			}
		}
	}
}

static int checknear(grid a[][N], int i, int k)
{
	int m, n, sum = 0;
	for (m = -1; m < 2; m++) {
		if ((i + m >= 0) && (i + m < N)) {
			for (n = -1; n < 2; n++) {
				if ((k + n >= 0) && (k + n < N) && (a[i + m][k + n].nearby == -1))
					sum++;
			}
		}
	}
	return sum;
}

static void manage_space_press(grid a[][N], int i, int k, int *victory)
{
	if (a[i][k].nearby == -1)
		*victory = 0;
	else
		cascadeuncover(a, i, k);
}

static void manage_enter_press(grid a[][N], int i, int k, int *correct, int rowtot)
{
	if (a[i][k].sign == '*') {
		a[i][k].sign = '-';
		bombs++;
		if (a[i][k].nearby != -1)
			(*correct)++;
	} else {
		a[i][k].sign = '*';
		bombs--;
		if (a[i][k].nearby != -1)
			(*correct)--;
	}
	printw("%c", a[i][k].sign);
	mvprintw(rowtot - 3, 1, "Still %d bombs.\n", bombs);
}

static void victory_check(grid a[][N], int victory, int correct, int rowtot, int coltot)
{
	char winmesg[] = "YOU WIN! It was just luck...";
	char losemesg[] = "You're a **cking loser. :P";
	clear();
	attron(A_BOLD);
	attron(COLOR_PAIR(2));
	if ((victory) && (correct == 1))
		mvprintw(rowtot / 2, (coltot - strlen(winmesg)) / 2, "%s", winmesg);
	else
		mvprintw(rowtot / 2, (coltot - strlen(losemesg)) / 2, "%s", losemesg);
	refresh();
	sleep(1);
	attroff(A_BOLD);
	attroff(COLOR_PAIR);
	endwin();
}
