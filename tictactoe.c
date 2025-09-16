#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SIZE 3
#define CELLS (SIZE * SIZE)

void print_board(const char *board);
int index_of(long row, long col);
int in_range(long row, long col);
int is_empty(const char *board, long row, long col);
void read_move(long *row, long *col);
void make_move(char *borad, char player);
char check_winner(const char *borad);
int board_full(const char *board);

void print_board(const char *board)
{
	for (int row = 0; row < SIZE; ++row)
	{
		if (row != 0)
		{
			printf("-+-+-\n");
		}
		for (int col = 0; col < SIZE; ++col)
		{
			if (col != 0) printf("|");

			int idx = index_of(row, col);
			char ch = board[idx] ? board[idx] : ' ';
			printf("%c", ch);
		}
		printf("\n");
	}
}
int index_of(long row, long col)
{
	return (int)(row * SIZE + col);
}
int in_range(long row, long col)
{
	return row >= 0 && row < SIZE&& col >= 0 && col < SIZE;
}
int is_empty(const char *board, long row, long col)
{
	return board[index_of(row, col)] == '\0';
}

void read_move(long *row, long *col)
{
	while (1)
	{
		printf("make move at row, col: ");

		char buf[64] = {0};
		if (fgets(buf, sizeof(buf), stdin) == NULL)
		{
			clearerr(stdin);
			continue;
		}

		char *endptr = buf;
		long r = strtol(endptr, &endptr, 10);

		/*here is for skip non digits between numbers */
		while (*endptr != 0 && !isdigit((unsigned char)*endptr) && *endptr != '-' && *endptr != '+')
		{
			++endptr;
		}

		long c = strtol(endptr, &endptr, 10);

		/* we should have at least parsed two longs */
		if ((r == 0 && c== 0 && endptr == buf))
		{
			puts("invalid input! Please enter two integers, like 1,2, 3.");
			continue;
		}

		*row = r;
		*col = c;
		return;
	}
}

/* place the player's mark after validating range and vacancy */

void make_move(char *board, char player)
{
	long prev_row = -999, prev_col = -999;

	while (1)
	{
		long row = -1, col = -1;
		read_move(&row, &col);

		if (row == prev_row && col == prev_col)
		{
			puts("You can't choose the same coordinates twice in a row!");
			continue;
		}

		/*remember the most recent attempt*/
		prev_row = row;
		prev_col = col;

		if (!in_range(row, col))
		{
			puts("out of range! Row and col must be like 0...2..");
			continue;
		}
		if (!is_empty(board, row, col))
		{
			puts("cell already occupied. Try anotehr.");
			continue;
		}

		board[index_of(row, col)] = player;
		return;
	}
}

/*return x or o if someone wins*/

char check_winner(const char *board)
{
	/*row*/
	for (int r = 0; r < SIZE; ++r)
	{
		int i =index_of(r, 0);
		char a = board[i], b = board[i+1], c = board[i+2];
		if (a && a == b && b == c) return a;
	}

	/*col*/
	for (int c = 0; c < SIZE; ++c)
	{
		char a = board[index_of(0, c)];
		char b = board[index_of(1, c)];
		char d = board[index_of(2, c)];
		if (a && a == b && b == d) return a;
	}

	/*diagonal*/
	{
		char a = board[index_of(0,0)];
		char b = board[index_of(1,1)];
		char c = board[index_of(2,2)];
		if (a && a == b && b == c) return a;
	}
	{
		char a = board[index_of(0,2)];
		char b = board[index_of(1,1)];
		char c = board[index_of(2,0)];
		if (a && a == b && b == c) return a;
	}

	return '\0';
}

int board_full(const char *board)
{
	for (int i = 0; i < CELLS; ++i)
	{
		if (board[i] == '\0') return 0;
	}
	return 1;
}

int main(void)
{
	char board[CELLS] = {0};
	char current = 'X';

	while (1)
	{
		print_board(board);
		make_move(board, current);

		char w = check_winner(board);
		if (w)
		{
			print_board(board);
			printf("player %c wins!\n", w);
			break;
		}

		current = (current == 'X') ? 'O' : 'X';
	}
	return 0;
}

