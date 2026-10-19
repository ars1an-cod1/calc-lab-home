// Mehmet Arslan Vatan 251AIB047
// (optional) https://github.com/ars1an-cod1/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude (Anthropic) AI assistant, conversation: <LINK> — details in comments where used

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// AI (Claude): prompt "C labwork step 2: read the whole input file into memory" Link: <LINK>
// Why one buffer: we find the size first, allocate once and read once,
// so even multi-MB files are never copied repeatedly.
// "rb" (binary mode) keeps every byte as-is, so positions match the file exactly.
static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    // Jump to the end to learn the file size, then go back to the start.
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    rewind(f);

    char *buf = malloc((size_t)size + 1);   // +1 for the closing '\0'
    if (!buf) { fclose(f); return NULL; }

    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[n] = '\0';      // lets us safely look one char past the end
    *out_len = n;
    return buf;
}

// AI (Claude): prompt "C labwork step 3: tokenizer that tracks positions" Link: <LINK>
typedef enum {
    TOK_NUM, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_POW,
    TOK_LPAREN, TOK_RPAREN, TOK_NEWLINE, TOK_END, TOK_INVALID
} TokType;

typedef struct {
    TokType type;
    size_t pos;        // 1-based position of the token's first char in the file
    int is_float;      // only for TOK_NUM: 1 if it had a '.', else 0
    long long ival;
    double fval;
} Token;

// How positions are tracked: i is the 0-based index of the next unread char,
// so a token starting at i has position i + 1. '\n' is a normal char and is
// counted too. At end of file i == len, so the position becomes len + 1,
// exactly what the rules want for a missing operand at the end.
typedef struct {
    const char *text;
    size_t len;
    size_t i;
} Lexer;

static Token next_token(Lexer *lx) {
    // Spaces and tabs only separate tokens, so skip them.
    while (lx->text[lx->i] == ' ' || lx->text[lx->i] == '\t')
        lx->i++;

    Token t = {0};
    t.pos = lx->i + 1;

    if (lx->i >= lx->len) { t.type = TOK_END; return t; }

    char c = lx->text[lx->i];

    if (isdigit((unsigned char)c)) {
        size_t start = lx->i;
        long long v = 0;
        while (isdigit((unsigned char)lx->text[lx->i])) {
            v = v * 10 + (lx->text[lx->i] - '0');
            lx->i++;
        }
        // A '.' belongs to the number only if a digit follows it.
        // Otherwise we stop here and the '.' becomes its own invalid token,
        // which puts the error exactly at the '.' ("5." and "2. + 5").
        // ".5" also fails at the '.', because a number must start with a digit.
        if (lx->text[lx->i] == '.' && isdigit((unsigned char)lx->text[lx->i + 1])) {
            lx->i++;
            while (isdigit((unsigned char)lx->text[lx->i]))
                lx->i++;
            t.is_float = 1;
            // strtod may also read an exponent like "e3", but an 'e' after a
            // number is an invalid char, so that file is an ERROR anyway.
            t.fval = strtod(lx->text + start, NULL);
        } else {
            t.ival = v;
        }
        t.type = TOK_NUM;
        return t;
    }

    lx->i++;   // every other token starts with a single char
    switch (c) {
        case '+':  t.type = TOK_PLUS;    break;
        case '-':  t.type = TOK_MINUS;   break;
        case '/':  t.type = TOK_SLASH;   break;
        case '(':  t.type = TOK_LPAREN;  break;
        case ')':  t.type = TOK_RPAREN;  break;
        case '\n': t.type = TOK_NEWLINE; break;
        case '*':
            // Look one char ahead: "**" is power, a single '*' is multiply.
            if (lx->text[lx->i] == '*') { lx->i++; t.type = TOK_POW; }
            else t.type = TOK_STAR;
            break;
        default:   t.type = TOK_INVALID; break;
    }
    return t;
}

// Temporary main, only to test read_file. Will be replaced later.

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    size_t len;
    char *text = read_file(argv[1], &len);
    if (!text) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }

    Lexer lx = { text, len, 0 };
    for (;;) {
        Token t = next_token(&lx);
        printf("pos %zu  type %d\n", t.pos, t.type);
        if (t.type == TOK_END) break;
    }
    free(text);
    return 0;
}