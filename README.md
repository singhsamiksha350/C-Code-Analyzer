# Code Analyzer

A command-line C program that scans a C source file and reports line counts, keyword frequencies, operator frequencies, identifier frequencies, and a few simple patterns worth reviewing.

## Compile

```bash
gcc -std=c11 -Wall -Wextra -pedantic code_analyzer.c -o code_analyzer
```

## Run

```bash
./code_analyzer sample.c
```

On Windows PowerShell:

```powershell
.\code_analyzer.exe sample.c
```

This is a lightweight lexical analyzer, not a full C parser or compiler. Its pattern checks are simple text-based heuristics.