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
