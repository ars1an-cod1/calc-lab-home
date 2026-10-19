# calc-lab-home

Arithmetic expression parser in C — graded lab for Programming Languages (RTU).

Reads arithmetic expressions from text files, evaluates them, and writes each
result (or the position of the first error) to an output file.

## Build
gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm

## Usage
calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]

## Tests
sh calc-tests/run_tests.sh ./calc
sh calc-tests/cli.sh ./calc

## Author
Mehmet Arslan Vatan (251AIB047)


## AI usage

I used **Claude (Anthropic)** for this lab. The full conversation is here: <LINK>

- The conversation is in **Turkish** (my native language). Code and comments are in English.
- We worked step by step, following the grade table (4 → 10):
  file reading, tokenizer, parser (+ -), output file, * and /, parentheses,
  ** and comments, command line, floats.
- **Claude wrote most of the code.** I pasted each step into `calc.c`,
  compiled it, ran the sample tests and committed after each step.
- After each step Claude explained the idea and asked me questions about it
  (for example: why `**` uses recursion instead of a loop, why errors are
  reported in `parse_primary`, why the output buffer doubles its size).
  I asked questions when I did not understand.
- Every place that came from the conversation is marked in `calc.c`
  with a comment `// AI (Claude): prompt "..." Link: ...`.
