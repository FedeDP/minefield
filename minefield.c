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
static void grid_init(grid a[][N]);
static void num_bombs(void);
static int main_cycle(grid a[][N], int *i, int *k, int *victory, int *correct, int *quit);
static void cascadeuncover(grid a[][N], int i, int k);
static int checknear(grid a[][N], int i, int k);
static void manage_space_press(grid a[][N], int i, int k, int *victory);
static void manage_enter_press(grid a[][N], int i, int k, int *correct);
static void victory_check(grid a[][N], int victory, int correct, int rowtot, int coltot, int quit);

/* Global variables */
static int horiz_space, vert_space; /* scaled values to fit terminal size */
static int bombs;
static WINDOW *field, *score;

int main(void)
{
    int i = 0, k = 0, victory = 1, correct = 1, quit = 0;
    int rowtot, coltot;
    grid a[N][N];
    srand(time(NULL));
    num_bombs();
    grid_init(a);
    initscr();
    getmaxyx(stdscr, rowtot, coltot);
    /* check terminal size */
    if ((rowtot < N + 6) || (coltot < N + 2)) {
        clear();
        endwin();
        printf("This screen has %d rows and %d columns. Enlarge it.\n", rowtot, coltot);
        printf("You need at least %d rows and %d columns.\n", N + 6, N + 2);
        return 1;
    }
    screen_init(a, rowtot, coltot);
    while ((victory) && (bombs > 0) && (!quit))
        main_cycle(a, &i, &k, &victory, &correct, &quit);
    victory_check(a, victory, correct, rowtot, coltot, quit);
    return 0;
}

static void screen_init(grid a[][N], int rowtot, int coltot)
{
    int i, k, rows, cols;
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    raw();
    noecho();
    vert_space = (rowtot - 6) / N;
    horiz_space = (coltot - 2) / N;
    rows = ((N - 1) * vert_space) + 3;
    cols = ((N - 1) * horiz_space) + 3;
    /* create sub windows centered */
    field = subwin(stdscr, rows, cols, (rowtot - 4 - rows) / 2, (coltot - cols) / 2);
    score = subwin(stdscr, 2 + 2, coltot, rowtot - 4, 0);
    keypad(field, TRUE);
    wborder(field, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(score, '|', '|', '-', '-', '+', '+', '+', '+');
    for (i = 0; i < N; i++) {
        for (k = 0; k < N; k++)
            mvwprintw(field, (i * vert_space) + 1, (k * horiz_space) + 1, "%c", a[i][k].sign);
    }
    mvwprintw(score, 2, 1, "Enter to put a bomb (*). Space to uncover. q anytime to *rage* quit.");
    mvwprintw(score, 1, 1, "Still %d bombs.", bombs);
    wrefresh(score);
}

static void grid_init(grid a[][N])
{
    int i, k, row, col;
    /* Generates random bombs */
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

static int main_cycle(grid a[][N], int *i, int *k, int *victory, int *correct, int *quit)
{
    wmove(field, (*i * vert_space) + 1, (*k * horiz_space) + 1);
    wrefresh(field);
    switch (wgetch(field)) {
        case KEY_LEFT:
            if (*k != 0)
                (*k)--;
            break;
        case KEY_RIGHT:
            if (*k != N - 1)
                (*k)++;
            break;
        case KEY_UP:
            if (*i != 0)
                (*i)--;
            break;
        case KEY_DOWN:
            if (*i != N - 1)
                (*i)++;
            break;
        case 32: /* space to uncover */
            if (a[*i][*k].sign == '-')
                manage_space_press(a, *i, *k, victory);
            break;
        case 10: /* Enter to  identify a bomb */
            if ((a[*i][*k].sign == '*') || (a[*i][*k].sign == '-'))
                manage_enter_press(a, *i, *k, correct);
            break;
        case 'q': /* q to exit */
            *quit = 1;
            break;
    }
    return 1;
}

static void cascadeuncover(grid a[][N], int i, int k)
{
    int m, n;
    if ((i >= 0) && (i < N) && (k >= 0) && (k < N) && (a[i][k].sign == '-')) {
        a[i][k].sign = '0' + a[i][k].nearby;
        wmove(field, (i * vert_space) + 1, (k * horiz_space) + 1);
        if (a[i][k].nearby != 0) {
            if (a[i][k].nearby >= 4)
                wattron(field, COLOR_PAIR(1));
            else
                wattron(field, COLOR_PAIR(a[i][k].nearby + 3));
            wprintw(field, "%c", a[i][k].sign);
            wattroff(field, COLOR_PAIR);
        } else {
            wattron(field, A_BOLD);
            wattron(field, COLOR_PAIR(3));
            wprintw(field, "%c", a[i][k].sign);
            wattroff(field, COLOR_PAIR);
            wattroff(field, A_BOLD);
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

static void manage_enter_press(grid a[][N], int i, int k, int *correct)
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
    wprintw(field, "%c", a[i][k].sign);
    mvwprintw(score, 1, 1, "Still %d bombs.", bombs);
    wrefresh(score);
}

static void victory_check(grid a[][N], int victory, int correct, int rowtot, int coltot, int quit)
{
    char winmesg[] = "YOU WIN! It was just luck...";
    char losemesg[] = "You're a **cking loser. :P";
    char exitmsg[] = "Leaving...bye! See you later :)";
    wclear(field);
    wclear(score);
    delwin(field);
    delwin(score);
    attron(A_BOLD);
    attron(COLOR_PAIR(rand() % 6 + 1));
    if (quit == 0) {
        if ((victory) && (correct == 1))
            mvprintw(rowtot / 2, (coltot - strlen(winmesg)) / 2, "%s", winmesg);
        else
            mvprintw(rowtot / 2, (coltot - strlen(losemesg)) / 2, "%s", losemesg);
    } else {
         mvprintw(rowtot / 2, (coltot - strlen(exitmsg)) / 2, "%s", exitmsg);
    }
    refresh();
    sleep(1);
    attroff(A_BOLD);
    attroff(COLOR_PAIR);
    endwin();
}