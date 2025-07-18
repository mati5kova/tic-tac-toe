#include <ctype.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

const int row_offsets[] = {1, 1, 1, 3, 3, 3, 5, 5, 5};   // row numbers
const int col_offsets[] = {2, 6, 10, 2, 6, 10, 2, 6, 10}; // column numbers

typedef struct {
    unsigned int win;
    unsigned int draw;
    unsigned int loss;
} Outcome;

Outcome outcome = {0, 0, 0};

typedef enum {
    Blank = 0,
    X = 1,
    O = 2
} State;

State board[3][3] = {{Blank, Blank, Blank},
                     {Blank, Blank, Blank},
                     {Blank, Blank, Blank}};

State test_board[3][3] = {{X, O, X},
                          {Blank, O, Blank},
                          {Blank, Blank, Blank}};

void enable_raw_mode(struct termios* orig_termios) {
    struct termios raw;

    // get curr terminal settings
    tcgetattr(STDIN_FILENO, orig_termios);
    raw = *orig_termios;

    // disable canonical mode and echo
    raw.c_lflag &= ~(ICANON | ECHO);

    // set new attributes
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(const struct termios* orig_termios) {
    // restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

bool any_winner(State board[3][3], State player) {
    for (int i = 0; i < 3; i++) {
        // vrstice
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player)
            return true;

        // stolpci
        if (board[0][i] == player && board[1][i] == player && board[2][i] == player)
            return true;
    }

    // diagonale
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player)
        return true;

    if (board[0][2] == player && board[1][1] == player && board[2][0] == player)
        return true;

    return false;
}

bool update_board (const int position, State player) {
    const char mark = player == X ? 'X' : 'O';

    const int row = (position - 1) / 3;
    const int col = (position - 1) % 3;

    if (board[row][col] != Blank) return false;

    board[row][col] = player;

    const int offset_row = row_offsets[position - 1];
    const int offset_col = col_offsets[position - 1];

    printf("\033[%d;%dH%c", offset_row, offset_col, mark);  // move to (row, col), place mark
    fflush(stdout);

    return true;
}

void print_board_state() {
    printf("\033[2J"); // clear screen
    printf("\033[H");  // move cursor to top-left

    for (int i = 0; i < 3; i++) {
        printf(" ");
        for (int j = 0; j < 3; j++) {
            const State player = board[i][j];
            unsigned char symbol = 3 * i + j + '1';

            char color_start[10] = ANSI_COLOR_RESET;

            if (player == X) {
                symbol = 'X';
                strcpy(color_start, ANSI_COLOR_RED);
            } else if (player == O) {
                symbol = 'O';
                strcpy(color_start, ANSI_COLOR_GREEN);
            }

            printf("%s%c%s", color_start, symbol, ANSI_COLOR_RESET);
            if (j < 2) printf(" | ");
        }
        printf("\n");
        if (i < 2) printf("---+---+---\n");
    }

    printf("\n\033[10;0HPress keys (q to quit)...\033[K");
    fflush(stdout);
}
void recursive_outcome(State b[3][3], const int moves_made, const State original, const State to_move) {
    // win check za igralca ki je ravnokar igral (12345(6-'O'))
    const State previous_player = (to_move == X ? O : X);
    if (any_winner(b, previous_player)) {
        if (previous_player == original) outcome.win++;
        else                outcome.loss++;
        return;
    }
    // draw check
    if (moves_made == 9) {
        outcome.draw++;
        return;
    }

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if (b[r][c] == Blank) {
                b[r][c] = to_move;
                recursive_outcome(b, moves_made+1, original, to_move == X? O : X);
                b[r][c] = Blank;
            }
        }
    }
}

int main() {
    struct termios orig_termios;
    enable_raw_mode(&orig_termios);
    printf("\033[?25l"); // hide cursor

    print_board_state();
    printf("\033[10;0HPress keys (q to quit)...\033[K");

    int num_moves = 0;

    unsigned char c;
    while ((c = getchar()) != 'q') {
        if (isdigit(c) && c != '0') {
            const State player =  num_moves % 2 == 0 ? X : O;

            if (update_board(c - '0', player)) {
                num_moves++;
                print_board_state();

                outcome.win = outcome.draw = outcome.loss = 0;
                const State next = (player == X ? O : X);

                recursive_outcome(board, num_moves, next, next);
                printf("\033[11;0Hw:%d d:%d l:%d\033[K", outcome.win, outcome.draw, outcome.loss);

                if (num_moves >= 5) {
                    if (any_winner(board, player)) {
                        printf("\033[10;0HWinner: Player %c!\033[K", num_moves % 2 == 1 ? 'X' : 'O');
                        break;
                    }
                }

                if (num_moves >= 9) {
                    printf("\033[10;0HDraw!\033[K");
                    break;
                }
            }
        }
    }

    printf("\033[?25h"); // show cursor again
    disable_raw_mode(&orig_termios);

    return 0;
}
