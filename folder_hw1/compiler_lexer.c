// compiler_lexer.c
// Build:  gcc -std=c11 -O2 -Wall -Wextra -o compiler_lexer compiler_lexer.c
// Run:    ./compiler_lexer simple.jive

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Token kinds used by the lexer */
typedef enum Token_Kind
{
	TOKEN_KEYWORD,
	TOKEN_IDENT,
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
	TOKEN_ARROW,
	TOKEN_TYPE,
	TOKEN_OPEN_BRACE,
	TOKEN_CLOSE_BRACE,
	TOKEN_INTEGER,
	TOKEN_STRING,
	TOKEN_EOF,
} Token_Kind;

/* Optional enums for clarity */
typedef enum Keyword
{
	KW_FN,
	KW_LET,
	KW_SET,
	KW_IF,
	KW_WHILE,
	KW_CALL,
	KW_RETURN,
	KW_TRUE,
	KW_FALSE,
} Keyword;

typedef enum Type_Keyword
{
	TK_INT,
	TK_STR,
	TK_BOOL,
} Type_Keyword;

/* Token structure */
typedef struct Token
{
	Token_Kind kind;
	const char *file;
	int line;
	int col;
	char *lexeme; 
	long ivalue;  
} Token;

/* Dynamic array of tokens */
typedef struct Token_Array
{
	Token *data;
	size_t len;
	size_t cap;
} Token_Array;

/* Simple growable array helpers */
static void
ta_init(Token_Array *a)
{
	a->data = NULL;
	a->len = 0;
	a->cap = 0;
}

static void
ta_push(Token_Array *a, Token t)
{
	if (a->len == a->cap)
	{
		a->cap = a->cap ? a->cap * 2 : 64;
		a->data = (Token *)realloc(a->data, a->cap * sizeof(Token));
		if (!a->data)
		{
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
	}
	a->data[a->len++] = t;
}

static char *
owned_strndup(const char *s, size_t n)
{
	char *p = (char *)malloc(n + 1);
	if (!p)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}

/* Lexer state */
typedef struct Lexer
{
	const char *file;
	const char *src;
	size_t n;
	size_t i;
	int line;
	int col;
} Lexer;

/* cursor helpers */
static int
lx_eof(Lexer *L)
{
	return L->i >= L->n;
}

static char
lx_peek(Lexer *L)
{
	return L->i < L->n ? L->src[L->i] : 0;
}

static char
lx_peek2(Lexer *L)
{
	return (L->i + 1) < L->n ? L->src[L->i + 1] : 0;
}

static char
lx_adv(Lexer *L)
{
	if (L->i >= L->n) return 0;
	char c = L->src[L->i++];
	if (c == '\n')
	{
		L->line++;
		L->col = 1;
	}
	else
	{
		L->col++;
	}
	return c;
}

/* Skip whitespace and // comments */
static void
skip_ws_and_comments(Lexer *L)
{
	for (;;)
	{
		while (!lx_eof(L) && isspace((unsigned char)lx_peek(L))) lx_adv(L);
		if (!lx_eof(L) && lx_peek(L) == '/' && lx_peek2(L) == '/')
		{
			while (!lx_eof(L) && lx_peek(L) != '\n') lx_adv(L);
			continue;
		}
		break;
	}
}

/* Identifier rules */
static int is_ident_start(int c)    { return isalpha(c) || c == '_'; }
static int is_ident_continue(int c) { return isalnum(c) || c == '_'; }

static int
str_in_list(const char *s, const char *list[], size_t cnt)
{
	for (size_t i = 0; i < cnt; i++)
	{
		if (strcmp(s, list[i]) == 0) return 1;
	}
	return 0;
}

static Token
make_simple(Lexer *L, Token_Kind k, int line, int col, const char *lex, size_t n)
{
	Token t = (Token){0};
	t.kind = k;
	t.file = L->file;
	t.line = line;
	t.col = col;
	t.lexeme = owned_strndup(lex, n);
	return t;
}

/* Read next token from the stream */
static Token
next_token(Lexer *L)
{
	skip_ws_and_comments(L);

	if (lx_eof(L))
	{
		Token t = (Token){0};
		t.kind = TOKEN_EOF;
		t.file = L->file;
		t.line = L->line;
		t.col = L->col;
		t.lexeme = owned_strndup("EOF", 3);
		return t;
	}

	int line = L->line;
	int col  = L->col;
	char c = lx_peek(L);

	/* single-char symbols */
	if (c == '(') { lx_adv(L); return make_simple(L, TOKEN_OPEN_PAREN,  line, col, "(", 1); }
	if (c == ')') { lx_adv(L); return make_simple(L, TOKEN_CLOSE_PAREN, line, col, ")", 1); }
	if (c == '{') { lx_adv(L); return make_simple(L, TOKEN_OPEN_BRACE,  line, col, "{", 1); }
	if (c == '}') { lx_adv(L); return make_simple(L, TOKEN_CLOSE_BRACE, line, col, "}", 1); }

	/* arrow "->" */
	if (c == '-' && lx_peek2(L) == '>')
	{
		lx_adv(L); lx_adv(L);
		return make_simple(L, TOKEN_ARROW, line, col, "->", 2);
	}

	/* integer */
	if (isdigit((unsigned char)c))
	{
		size_t start = L->i;
		while (!lx_eof(L) && isdigit((unsigned char)lx_peek(L))) lx_adv(L);
		size_t end = L->i;
		char *lex = owned_strndup(L->src + start, end - start);

		Token t = (Token){0};
		t.kind = TOKEN_INTEGER;
		t.file = L->file;
		t.line = line;
		t.col = col;
		t.lexeme = lex;
		t.ivalue = strtol(lex, NULL, 10);
		return t;
	}

	/* identifier / keyword / type */
	if (is_ident_start((unsigned char)c))
	{
		size_t start = L->i;
		lx_adv(L);
		while (!lx_eof(L) && is_ident_continue((unsigned char)lx_peek(L))) lx_adv(L);
		size_t end = L->i;
		char *lex = owned_strndup(L->src + start, end - start);

		static const char *keywords[] = { "fn","let","set","if","while","call","return","true","false" };
		static const char *types[]    = { "int","str","bool" };

		Token t = (Token){0};
		t.file = L->file;
		t.line = line;
		t.col = col;
		t.lexeme = lex;

		if (str_in_list(lex, keywords, sizeof(keywords)/sizeof(keywords[0])))
		{
			t.kind = TOKEN_KEYWORD;
		}
		else if (str_in_list(lex, types, sizeof(types)/sizeof(types[0])))
		{
			t.kind = TOKEN_TYPE;
		}
		else
		{
			t.kind = TOKEN_IDENT;
		}
		return t;
	}

	/* string literal */
	if (c == '"')
	{
		lx_adv(L); /* opening quote */
		char *buf = (char *)malloc((L->n - L->i) + 1);
		if (!buf) { fprintf(stderr, "out of memory\n"); exit(1); }

		size_t w = 0;
		int closed = 0;

		while (!lx_eof(L))
		{
			char ch = lx_adv(L);
			if (ch == '"') { closed = 1; break; }
			if (ch == '\\')
			{
				if (lx_eof(L)) break;
				char e = lx_adv(L);
				switch (e)
				{
					case 'n': buf[w++] = '\n'; break;
					case 't': buf[w++] = '\t'; break;
					case '"': buf[w++] = '"';  break;
					case '\\': buf[w++] = '\\'; break;
					default:  buf[w++] = e;    break;
				}
			}
			else
			{
				buf[w++] = ch;
			}
		}
		buf[w] = '\0';

		if (!closed)
		{
			fprintf(stderr, "%s:%d:%d: unterminated string literal\n", L->file, line, col);
			free(buf);
			exit(1);
		}

		Token t = (Token){0};
		t.kind = TOKEN_STRING;
		t.file = L->file;
		t.line = line;
		t.col = col;
		t.lexeme = buf;
		return t;
	}

	/* unknown character -> error */
	fprintf(stderr, "%s:%d:%d: unexpected character '%c'\n", L->file, line, col, c);
	exit(1);
}

/* Read whole file into memory */
static char *
read_entire_file(const char *path, size_t *out_n)
{
	FILE *f = fopen(path, "rb");
	if (!f) { fprintf(stderr, "cannot open file: %s\n", path); exit(1); }
	if (fseek(f, 0, SEEK_END) != 0) { fprintf(stderr, "fseek failed\n"); exit(1); }
	long n = ftell(f);
	if (n < 0) { fprintf(stderr, "ftell failed\n"); exit(1); }
	if (fseek(f, 0, SEEK_SET) != 0) { fprintf(stderr, "fseek failed\n"); exit(1); }

	char *buf = (char *)malloc((size_t)n + 1);
	if (!buf) { fprintf(stderr, "out of memory\n"); exit(1); }
	size_t rd = fread(buf, 1, (size_t)n, f);
	fclose(f);

	buf[rd] = '\0';
	if (out_n) *out_n = rd;
	return buf;
}

/* Map kind to printable name */
static const char *
kind_name(Token_Kind k)
{
	switch (k)
	{
		case TOKEN_KEYWORD:     return "KEYWORD";
		case TOKEN_IDENT:       return "IDENTIFIER";
		case TOKEN_OPEN_PAREN:  return "(";
		case TOKEN_CLOSE_PAREN: return ")";
		case TOKEN_ARROW:       return "ARROW";
		case TOKEN_TYPE:        return "TYPE";
		case TOKEN_OPEN_BRACE:  return "{";
		case TOKEN_CLOSE_BRACE: return "}";
		case TOKEN_INTEGER:     return "INTEGER";
		case TOKEN_STRING:      return "STRING";
		case TOKEN_EOF:         return "EOF";
		default:                return "UNKNOWN";
	}
}

/* Lex the whole file into a Token_Array */
static Token_Array
lex_file(const char *file_name)
{
	Token_Array result;
	ta_init(&result);

	size_t n = 0;
	char *src = read_entire_file(file_name, &n);

	Lexer L = (Lexer){0};
	L.file = file_name;
	L.src  = src;
	L.n    = n;
	L.i    = 0;
	L.line = 1;
	L.col  = 1;

	for (;;)
	{
		Token t = next_token(&L);
		ta_push(&result, t);
		if (t.kind == TOKEN_EOF) break;
	}

	free(src);
	return result;
}

/* print tokens in required format */
int
main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <source.jive>\n", argv[0]);
		return 1;
	}

	const char *file = argv[1];
	Token_Array arr = lex_file(file);

	for (size_t i = 0; i < arr.len; i++)
	{
		Token *t = &arr.data[i];
		if (t->kind == TOKEN_INTEGER)
		{
			printf("%s:%d:%d\t%s\t%ld\n", t->file, t->line, t->col, kind_name(t->kind), t->ivalue);
		}
		else if (t->kind == TOKEN_EOF)
		{
			printf("%s:%d:%d\t%s\n", t->file, t->line, t->col, kind_name(t->kind));
		}
		else
		{
			printf("%s:%d:%d\t%s\t%s\n", t->file, t->line, t->col, kind_name(t->kind), t->lexeme ? t->lexeme : "");
		}
		free(t->lexeme);
	}
	free(arr.data);
	return 0;
}
