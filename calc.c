// Mehmet Arslan Vatan 251AIB047
// (optional) https://github.com/ars1an-cod1/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude (Anthropic) AI assistant, conversation: <https://claude.ai/share/0a6ae165-5bd1-45db-a6f0-42b6ac272f1f> — details in comments where used

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

// Student data for output file and folder names (lowercase, like the examples).
// Kept in one place so it is easy to change.
#define STUDENT_NAME     "mehmet_arslan"
#define STUDENT_LASTNAME "vatan"
#define STUDENT_ID       "251aib047"

// AI (Claude): code written by Claude in conversation step 2: read the whole input file into memory Link: <https://claude.ai/share/0a6ae165-5bd1-45db-a6f0-42b6ac272f1f>
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

// AI (Claude): code written by Claude in conversation step 3: tokenizer that tracks positions Link: <https://claude.ai/share/0a6ae165-5bd1-45db-a6f0-42b6ac272f1f>
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
    int line_start;   // 1 if only spaces/tabs were seen since the last '\n'
} Lexer;

static Token next_token(Lexer *lx) {
    // Spaces and tabs only separate tokens, so skip them.
    while (lx->text[lx->i] == ' ' || lx->text[lx->i] == '\t')
        lx->i++;
    // '#' is a comment only if it is the first non-blank char of the line.
    // We skip up to the '\n' (but not past it), so the line looks blank to
    // the parser. The skipped chars still move i, so they count in positions.
    // A '#' anywhere else falls through to the switch and becomes invalid.
    if (lx->line_start && lx->text[lx->i] == '#') {
        while (lx->i < lx->len && lx->text[lx->i] != '\n')
            lx->i++;
    }
    lx->line_start = 0;   // any token we return now is no longer at line start

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
        case '\n': t.type = TOK_NEWLINE; lx->line_start = 1; break;
        case '*':
            // Look one char ahead: "**" is power, a single '*' is multiply.
            if (lx->text[lx->i] == '*') { lx->i++; t.type = TOK_POW; }
            else t.type = TOK_STAR;
            break;
        default:   t.type = TOK_INVALID; break;
    }
    return t;
}

// AI (Claude): code written by Claude in conversation step 4: recursive descent parser, + and - Link: <https://claude.ai/share/0a6ae165-5bd1-45db-a6f0-42b6ac272f1f>
typedef struct {
    Lexer lx;
    Token cur;        // the token the parser is looking at right now
    size_t err_pos;   // 0 = no error yet, otherwise position of the first error
} Parser;

static void advance(Parser *p) { p->cur = next_token(&p->lx); }


// Only the FIRST error is kept, because the output shows only the first error.
static void fail(Parser *p, size_t pos) { if (p->err_pos == 0) p->err_pos = pos; }
// AI (Claude): code written by Claude in conversation step 10: floats, int vs float arithmetic Link: <https://claude.ai/share/0a6ae165-5bd1-45db-a6f0-42b6ac272f1f>
// A value is either a 64-bit integer or a double; is_float says which one.
typedef struct {
    int is_float;
    long long i;
    double f;
} Value;

static Value make_int(long long x)  { Value v = { 0, x, 0.0 }; return v; }
static Value make_float(double x)   { Value v = { 1, 0, x };   return v; }
static double as_double(Value v)    { return v.is_float ? v.f : (double)v.i; }

// How int vs float is decided: if either operand is a float, the operation is
// done in double (7 / 2.0 = 3.5). Only int with int stays an integer
// (7 / 2 = 3, truncated toward zero like C).
// Errors: division by zero is reported at the start of the right operand
// (right_pos); a float result that is infinite or NaN at the operator (op_pos).
static Value apply(Parser *p, TokType op, Value a, Value b,
                   size_t op_pos, size_t right_pos) {
    if (!a.is_float && !b.is_float) {
        switch (op) {
            case TOK_PLUS:  return make_int(a.i + b.i);
            case TOK_MINUS: return make_int(a.i - b.i);
            case TOK_STAR:  return make_int(a.i * b.i);
            default:
                if (b.i == 0) { fail(p, right_pos); return make_int(0); }
                return make_int(a.i / b.i);
        }
    }
    double x = as_double(a), y = as_double(b), r;
    switch (op) {
        case TOK_PLUS:  r = x + y; break;
        case TOK_MINUS: r = x - y; break;
        case TOK_STAR:  r = x * y; break;
        default:
            if (y == 0.0) { fail(p, right_pos); return make_int(0); }
            r = x / y;
            break;
    }
    if (!isfinite(r)) { fail(p, op_pos); return make_int(0); }
    return make_float(r);
}

// Fast integer power (square-and-multiply): about log2(e) steps instead of e
// steps, so even "1 ** 1000000000000" is instant. x ** 0 = 1, also 0 ** 0.
// We only square the base when another step follows, so we never compute a
// value bigger than the final result (no overflow).
static long long int_pow(long long base, long long e) {
    long long result = 1;
    while (e > 0) {
        if (e & 1) result *= base;   // this bit of the exponent is 1
        e >>= 1;                     // move to the next bit
        if (e > 0) base *= base;     // base, base^2, base^4, base^8, ...
    }
    return result;
}

// parse_primary needs parse_expr (for parentheses), but parse_expr is defined
// further down, so we declare it here first.
static Value parse_expr(Parser *p);

// primary := NUMBER | '(' expr ')'
// Why errors are reported here: when an operand is expected but something else
// comes, the token we are looking at is exactly where we "could not continue".
// So "10 + - 3" fails at the second operator, "3 +\n" at the '\n',
// "-5" at the '-' (no unary minus), "2 * ()" at the ')',
// and an invalid char at that char.
static Value parse_primary(Parser *p) {
    if (p->cur.type == TOK_NUM) {
        Value v = p->cur.is_float ? make_float(p->cur.fval) : make_int(p->cur.ival);
        advance(p);
        return v;
    }
    if (p->cur.type == TOK_LPAREN) {
        advance(p);
        // Parentheses start again from the lowest level (expr), so the inside
        // is fully computed first. Nesting is just this function calling
        // parse_expr again, so deep nesting (1000 levels) works too.
        Value v = parse_expr(p);
        if (p->err_pos) return v;
        // Now we need ')'. If something else comes (usually the '\n' or the
        // end of file), that token is where we got stuck: "(2 + 3\n" -> '\n'.
        if (p->cur.type != TOK_RPAREN) { fail(p, p->cur.pos); return v; }
        advance(p);
        return v;
    }
    fail(p, p->cur.pos);
    return make_int(0);
}

// power := primary [ '**' power ]
// Why ** is right-associative: for the exponent we call parse_power itself
// (recursion) instead of looping, so the right side is grouped first:
// 2 ** 3 ** 2 = 2 ** (3 ** 2) = 2 ** 9 = 512.
// It is the highest level, so 2 * 3 ** 2 = 2 * (3 ** 2).
static Value parse_power(Parser *p) {
    Value base = parse_primary(p);
    if (p->err_pos || p->cur.type != TOK_POW) return base;
    size_t op_pos = p->cur.pos;     // for an infinite/NaN float result
    advance(p);
    size_t exp_pos = p->cur.pos;    // for a negative integer exponent
    Value e = parse_power(p);
    if (p->err_pos) return e;

    if (!base.is_float && !e.is_float) {
        // int ** int stays an integer, so a negative exponent is an error
        // (the result would be a fraction), reported at the exponent.
        if (e.i < 0) { fail(p, exp_pos); return e; }
        return make_int(int_pow(base.i, e.i));
    }
    // With a float, pow() decides: 0.5 ** (0 - 1) = 2 is fine,
    // but (0 - 8) ** 0.5 is NaN and 10.0 ** 400 is infinite -> error at '**'.
    double r = pow(as_double(base), as_double(e));
    if (!isfinite(r)) { fail(p, op_pos); return e; }
    return make_float(r);
}

// term := power { ('*' | '/') power }
// Why precedence works: parse_expr calls parse_term for each operand, so
// "2 * 5" is fully computed before + or - can use it: 3 + 2 * 5 = 3 + (2 * 5).
// The loop makes * and / left-associative: 100 / 7 / 2 = (100 / 7) / 2.
static Value parse_term(Parser *p) {
    Value left = parse_power(p);
    while (!p->err_pos && (p->cur.type == TOK_STAR || p->cur.type == TOK_SLASH)) {
        TokType op = p->cur.type;
        size_t op_pos = p->cur.pos;
        advance(p);
        size_t right_pos = p->cur.pos;   // division by zero is reported here
        Value right = parse_power(p);
        if (p->err_pos) break;
        left = apply(p, op, left, right, op_pos, right_pos);
    }
    return left;
}

// expr := term { ('+' | '-') term }
// The loop combines from left to right, so + and - are left-associative:
// 10 - 3 - 2 = (10 - 3) - 2 = 5.
static Value parse_expr(Parser *p) {
    Value left = parse_term(p);
    while (!p->err_pos && (p->cur.type == TOK_PLUS || p->cur.type == TOK_MINUS)) {
        TokType op = p->cur.type;
        size_t op_pos = p->cur.pos;
        advance(p);
        size_t right_pos = p->cur.pos;
        Value right = parse_term(p);
        if (p->err_pos) break;
        left = apply(p, op, left, right, op_pos, right_pos);
    }
    return left;
}

// Parses one line. Returns 1 and stores the result in *out if the line had an
// expression, 0 if it was blank or had an error (then err_pos is set).
static int parse_line(Parser *p, Value *out) {
    if (p->cur.type == TOK_NEWLINE) { advance(p); return 0; }   // blank line

    *out = parse_expr(p);
    if (p->err_pos) return 0;

    // After a full expression only the end of the line may follow.
    // Anything else ("2 3", "2 & 5", an extra ')') is an error at that token.
    if (p->cur.type == TOK_NEWLINE) advance(p);
    else if (p->cur.type != TOK_END) { fail(p, p->cur.pos); return 0; }
    return 1;
}
// AI (Claude): code written by Claude in conversation step 5: collect results and write the output file Link: <https://claude.ai/share/0a6ae165-5bd1-45db-a6f0-42b6ac272f1f>
// Growing text buffer for the results. When it is full, its size is doubled.
// Doubling keeps the total copying proportional to the output size
// (instead of copying on every append), so very large files stay fast.
typedef struct { char *data; size_t len, cap; } OutBuf;

static void out_append(OutBuf *o, const char *s, size_t n) {
    if (o->len + n > o->cap) {
        size_t cap = o->cap ? o->cap : 4096;
        while (o->len + n > cap) cap *= 2;
        char *d = realloc(o->data, cap);
        if (!d) { fprintf(stderr, "out of memory\n"); exit(1); }
        o->data = d;
        o->cap = cap;
    }
    memcpy(o->data + o->len, s, n);
    o->len += n;
}

// Evaluates the whole file, one result per line into out.
// If any error occurs, the collected results are dropped and only
// "ERROR:<pos>" is kept, because the rules forbid partial results.
static void evaluate(const char *text, size_t len, OutBuf *out) {
    Parser p = { { text, len, 0, 1 }, {0}, 0 };   // 1 = file starts at a line start
    advance(&p);    // load the first token
    while (p.cur.type != TOK_END && !p.err_pos) {
        Value v;
        if (parse_line(&p, &v)) {
            char num[64];
            int n;
            // Printing: integers with %lld, floats with %.15g (so 1.25 * 4 -> 5).
            if (v.is_float) n = snprintf(num, sizeof num, "%.15g\n", v.f);
            else            n = snprintf(num, sizeof num, "%lld\n", v.i);
            out_append(out, num, (size_t)n);
        }
    }
    if (p.err_pos) {
        out->len = 0;   // forget the partial results
        char msg[48];
        int n = snprintf(msg, sizeof msg, "ERROR:%zu\n", p.err_pos);
        out_append(out, msg, (size_t)n);
    }
}

// Returns the last part of a path without ".txt", and its length in *n:
// "labs/task1.txt" -> "task1", "../in" -> "in", "../in/" -> "in".
// Used for both the output file name and the default output folder name.
static const char *path_base(const char *path, size_t *n) {
    size_t end = strlen(path);
    while (end > 1 && path[end - 1] == '/') end--;          // ignore trailing '/'
    size_t start = end;
    while (start > 0 && path[start - 1] != '/') start--;    // back to the last '/'
    size_t blen = end - start;
    if (blen >= 4 && memcmp(path + start + blen - 4, ".txt", 4) == 0) blen -= 4;
    *n = blen;
    return path + start;
}


// Builds "<outdir>/<base>_<name>_<lastname>_<id>.txt".
static char *make_output_path(const char *outdir, const char *input) {
    size_t blen;
    const char *base = path_base(input, &blen);
    size_t size = strlen(outdir) + blen + 100;   // 100 = room for names + ".txt"
    char *path = malloc(size);
    if (!path) return NULL;
    snprintf(path, size, "%s/%.*s_%s_%s_%s.txt", outdir, (int)blen, base,
             STUDENT_NAME, STUDENT_LASTNAME, STUDENT_ID);
    return path;
}

// Creates the folder. If it already exists, that is fine: we write into it.
static int ensure_dir(const char *dir) {
    return mkdir(dir, 0777) == 0 || errno == EEXIST;
}

static int write_file(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    size_t written = (len > 0) ? fwrite(data, 1, len, f) : 0;
    return fclose(f) == 0 && written == len;
}
// AI (Claude): code written by Claude in conversation step 9: full command line, -d, default folder, exit codes  Link: <https://claude.ai/share/0a6ae165-5bd1-45db-a6f0-42b6ac272f1f>
// Evaluates one input file and writes its result file into outdir.
// Returns 0 on success, 1 if the input cannot be read or the output written.
// An "ERROR:<pos>" result is a normal result, so it still returns 0.
static int process_file(const char *input, const char *outdir) {
    size_t len;
    char *text = read_file(input, &len);
    if (!text) {
        fprintf(stderr, "calc: cannot read input file '%s'\n", input);
        return 1;
    }
    OutBuf out = {0};
    evaluate(text, len, &out);
    free(text);

    int rc = 0;
    char *path = make_output_path(outdir, input);
    if (!path || !write_file(path, out.data, out.len)) {
        fprintf(stderr, "calc: cannot write output for '%s'\n", input);
        rc = 1;
    }
    free(path);
    free(out.data);
    return rc;
}

// Evaluates every *.txt file directly inside dir. Other files are skipped by
// their name, and subfolders by stat (only regular files are processed).
// readdir does not go into subfolders, so only the top level is read.
static int process_dir(const char *dir, const char *outdir) {
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "calc: cannot open folder '%s'\n", dir);
        return 1;
    }
    int rc = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        size_t n = strlen(e->d_name);
        if (n <= 4 || strcmp(e->d_name + n - 4, ".txt") != 0) continue;

        size_t size = strlen(dir) + n + 2;      // dir + '/' + name + '\0'
        char *path = malloc(size);
        if (!path) { rc = 1; break; }
        snprintf(path, size, "%s/%s", dir, e->d_name);

        struct stat st;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode))
            rc |= process_file(path, outdir);   // remember if any file failed
        free(path);
    }
    closedir(d);
    return rc;
}

static void usage(void) {
    fprintf(stderr, "usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n");
}

int main(int argc, char **argv) {
    const char *dir = NULL, *outdir = NULL, *input = NULL;

    // Read the arguments. -d and -o need a value right after them.
    // Anything else starting with '-' is an unknown option.
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        int is_dir = strcmp(a, "-d") == 0 || strcmp(a, "--dir") == 0;
        int is_out = strcmp(a, "-o") == 0 || strcmp(a, "--output-dir") == 0;
        if (is_dir || is_out) {
            if (i + 1 >= argc) {
                fprintf(stderr, "calc: option '%s' needs a folder name\n", a);
                usage();
                return 1;
            }
            if (is_dir) dir = argv[++i];
            else        outdir = argv[++i];
        } else if (a[0] == '-') {
            fprintf(stderr, "calc: unknown option '%s'\n", a);
            usage();
            return 1;
        } else if (input == NULL) {
            input = a;
        } else {
            fprintf(stderr, "calc: only one input file is allowed\n");
            usage();
            return 1;
        }
    }

    // We need exactly one source: a folder (-d) or one input file.
    // This also covers "no arguments at all".
    if ((dir == NULL) == (input == NULL)) {
        fprintf(stderr, "calc: give either -d DIR or one input file\n");
        usage();
        return 1;
    }

    // No -o: default folder <input_base>_<name>_<studentid> in the current folder.
    char *default_out = NULL;
    if (outdir == NULL) {
        size_t blen;
        const char *base = path_base(dir ? dir : input, &blen);
        size_t size = blen + 100;
        default_out = malloc(size);
        if (!default_out) { fprintf(stderr, "calc: out of memory\n"); return 1; }
        snprintf(default_out, size, "%.*s_%s_%s", (int)blen, base, STUDENT_NAME, STUDENT_ID);
        outdir = default_out;
    }

    int rc;
    if (!ensure_dir(outdir)) {
        fprintf(stderr, "calc: cannot create output folder '%s'\n", outdir);
        rc = 1;
    } else if (dir) {
        rc = process_dir(dir, outdir);
    } else {
        rc = process_file(input, outdir);
    }
    free(default_out);
    return rc;
}