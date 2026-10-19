// Mehmet Arslan Vatan 251AIB047
// (optional) https://github.com/ars1an-cod1/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Claude (Anthropic) AI assistant, conversation: <LINK> — details in comments where used

#include <stdio.h>
#include <stdlib.h>

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

// Temporary main, only to test read_file. Will be replaced later.
int main(int argc, char **argv) {
    if (argc < 2) return 1;
    size_t len;
    char *text = read_file(argv[1], &len);
    if (!text) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }
    printf("%zu bytes\n", len);
    free(text);
    return 0;
}