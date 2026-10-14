#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096
#define MAX_TOKENS 5000
#define TOKEN_LEN 128
#define KEYWORD_COUNT 44
#define OPERATOR_COUNT 24

typedef struct {
    char text[TOKEN_LEN];
    int count;
} TokenCount;

static const char *keywords[KEYWORD_COUNT] = {
    "auto", "break", "case", "char", "const", "continue",
    "default", "do", "double", "else", "enum", "extern",
    "float", "for", "goto", "if", "inline", "int",
    "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Alignas",
    "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic",
    "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local"
};

static const char *operators[OPERATOR_COUNT] = {
    ">>=", "<<=", "++", "--", "->", "&&", "||", "<=", ">=",
    "==", "!=", "+=", "-=", "*=", "/=", "%=", "&=", "|=",
    "^=", "<<", ">>", "=", "+", "-"
};

static int is_keyword(const char *word) {
    int i;
    for (i = 0; i < KEYWORD_COUNT; i++) {
        if (strcmp(word, keywords[i]) == 0) return 1;
    }
    return 0;
}

static int is_operator_char(char c) {
    return strchr("+-*/%=!<>&|^~?:", c) != NULL;
}

static void increment_token(TokenCount counts[], int *count,
                            const char *token) {
    int i;

    for (i = 0; i < *count; i++) {
        if (strcmp(counts[i].text, token) == 0) {
            counts[i].count++;
            return;
        }
    }

    if (*count >= MAX_TOKENS) {
        return;
    }

    snprintf(counts[*count].text, TOKEN_LEN, "%s", token);
    counts[*count].count = 1;
    (*count)++;
}

static void print_token_counts(const char *heading,
                               TokenCount counts[], int count) {
    int i;

    printf("\n%s\n", heading);
    printf("----------------------------------------\n");

    if (count == 0) {
        printf("None found.\n");
        return;
    }

    for (i = 0; i < count; i++) {
        printf("%-24s %d\n", counts[i].text, counts[i].count);
    }
}

static int analyze_file(FILE *file) {
    char line[MAX_LINE];
    TokenCount keyword_counts[KEYWORD_COUNT] = {{{0}, 0}};
    TokenCount operator_counts[OPERATOR_COUNT] = {{{0}, 0}};
    TokenCount identifier_counts[MAX_TOKENS] = {{{0}, 0}};

    int keyword_total = 0;
    int operator_total = 0;
    int identifier_total = 0;

    int total_lines = 0;
    int blank_lines = 0;
    int comment_only_lines = 0;
    int code_lines = 0;

    int in_block_comment = 0;
    int in_string = 0;
    int in_char = 0;
    int escape_next = 0;

    int line_has_code;
    int line_has_comment;
    int line_has_nonspace;
    int i;

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        int line_comment_started = 0;
        total_lines++;
        line_has_code = 0;
        line_has_comment = 0;
        line_has_nonspace = 0;

        for (i = 0; i < (int)len; i++) {
            char c = line[i];
            char next = (i + 1 < (int)len) ? line[i + 1] : '\0';

            if (!isspace((unsigned char)c)) line_has_nonspace = 1;

            if (in_block_comment) {
                line_has_comment = 1;
                if (c == '*' && next == '/') {
                    in_block_comment = 0;
                    i++;
                }
                continue;
            }

            if (in_string) {
                line_has_code = 1;
                if (escape_next) {
                    escape_next = 0;
                } else if (c == '\\') {
                    escape_next = 1;
                } else if (c == '"') {
                    in_string = 0;
                }
                continue;
            }

            if (in_char) {
                line_has_code = 1;
                if (escape_next) {
                    escape_next = 0;
                } else if (c == '\\') {
                    escape_next = 1;
                } else if (c == '\'') {
                    in_char = 0;
                }
                continue;
            }

            if (c == '/' && next == '*') {
                in_block_comment = 1;
                line_has_comment = 1;
                i++;
                continue;
            }

            if (c == '/' && next == '/') {
                line_comment_started = 1;
                line_has_comment = 1;
                break;
            }

            if (c == '"') {
                in_string = 1;
                line_has_code = 1;
                continue;
            }

            if (c == '\'') {
                in_char = 1;
                line_has_code = 1;
                continue;
            }

            if (isspace((unsigned char)c)) continue;

            line_has_code = 1;

            if (isalpha((unsigned char)c) || c == '_') {
                char token[TOKEN_LEN];
                size_t j = 0;

                while (i < (int)len &&
                       (isalnum((unsigned char)line[i]) || line[i] == '_')) {
                    if (j + 1 < sizeof(token)) token[j++] = line[i];
                    i++;
                }
                token[j] = '\0';
                i--;

                if (is_keyword(token)) {
                    increment_token(keyword_counts, &keyword_total, token);
                } else {
                    increment_token(identifier_counts, &identifier_total, token);
                }
                continue;
            }

            if (is_operator_char(c)) {
                char op[4] = {c, '\0', '\0', '\0'};
                if (i + 2 < (int)len) {
                    char three[4] = {line[i], line[i + 1], line[i + 2], '\0'};
                    if (strcmp(three, ">>=") == 0 || strcmp(three, "<<=") == 0) {
                        snprintf(op, sizeof(op), "%s", three);
                        i += 2;
                    } else {
                        char two[3] = {line[i], line[i + 1], '\0'};
                        const char *found = NULL;
                        int k;
                        for (k = 0; k < OPERATOR_COUNT; k++) {
                            if (strlen(operators[k]) == 2 &&
                                strcmp(two, operators[k]) == 0) {
                                found = operators[k];
                                break;
                            }
                        }
                        if (found != NULL) {
                            snprintf(op, sizeof(op), "%s", found);
                            i++;
                        }
                    }
                } else if (i + 1 < (int)len) {
                    char two[3] = {line[i], line[i + 1], '\0'};
                    int k;
                    for (k = 0; k < OPERATOR_COUNT; k++) {
                        if (strlen(operators[k]) == 2 &&
                            strcmp(two, operators[k]) == 0) {
                            snprintf(op, sizeof(op), "%s", operators[k]);
                            i++;
                            break;
                        }
                    }
                }
                increment_token(operator_counts, &operator_total, op);
            }
        }

        if (!line_has_nonspace) {
            blank_lines++;
        } else if (!line_has_code && line_has_comment) {
            comment_only_lines++;
        } else if (line_has_code) {
            code_lines++;
        }

        (void)line_comment_started;
    }

    if (ferror(file)) {
        fprintf(stderr, "Error while reading the source file.\n");
        return 0;
    }

    printf("\n========== MINI C CODE ANALYZER ==========\n");
    printf("Lines of source scanned : %d\n", total_lines);
    printf("Code lines              : %d\n", code_lines);
    printf("Comment-only lines      : %d\n", comment_only_lines);
    printf("Blank lines              : %d\n", blank_lines);
    printf("Keyword types found     : %d\n", keyword_total);
    printf("Operator types found    : %d\n", operator_total);
    printf("Identifier types found  : %d\n", identifier_total);

    print_token_counts("KEYWORD FREQUENCIES", keyword_counts, keyword_total);
    print_token_counts("OPERATOR FREQUENCIES", operator_counts, operator_total);
    print_token_counts("IDENTIFIER FREQUENCIES", identifier_counts,
                       identifier_total);

    

    rewind(file);
    {
        int line_number = 0;
        int suspicious = 0;

        printf("\nSIMPLE PATTERN CHECKS (heuristics only)\n");
        printf("----------------------------------------\n");

        while (fgets(line, sizeof(line), file) != NULL) {
            line_number++;
            if (strstr(line, "gets(") != NULL) {
                printf("Line %d: uses gets(), which is unsafe; use fgets().\n",
                       line_number);
                suspicious++;
            }
            if (strstr(line, "strcpy(") != NULL) {
                printf("Line %d: uses strcpy(); verify destination capacity.\n",
                       line_number);
                suspicious++;
            }
            if (strstr(line, "scanf(") != NULL &&
                strstr(line, "%s") != NULL) {
                printf("Line %d: scanf with %%s; ensure a field width is set.\n",
                       line_number);
                suspicious++;
            }
        }

        if (suspicious == 0) {
            printf("No targeted patterns found.\n");
        }
    }

    return 1;
}

int main(int argc, char *argv[]) {
    FILE *file;
    char filename[512];

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [source_file.c]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        snprintf(filename, sizeof(filename), "%s", argv[1]);
    } else {
        printf("Enter path to a C source file: ");
        if (fgets(filename, sizeof(filename), stdin) == NULL) {
            fprintf(stderr, "Could not read filename.\n");
            return EXIT_FAILURE;
        }
        filename[strcspn(filename, "\r\n")] = '\0';
    }

    if (filename[0] == '\0') {
        fprintf(stderr, "Filename cannot be empty.\n");
        return EXIT_FAILURE;
    }

    file = fopen(filename, "r");
    if (file == NULL) {
        perror("Could not open source file");
        return EXIT_FAILURE;
    }

    if (!analyze_file(file)) {
        fclose(file);
        return EXIT_FAILURE;
    }

    if (fclose(file) != 0) {
        perror("Could not close source file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
