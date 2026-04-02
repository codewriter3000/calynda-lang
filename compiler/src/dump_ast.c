#include "ast_dump.h"
#include "parser.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_entire_file(const char *path);
static void print_usage(const char *program_name);
static int dump_program_file(const char *path);
static int dump_expression_file(const char *path);

int main(int argc, char **argv) {
    const char *path;
    int expression_mode = 0;

    if (argc == 2) {
        path = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "--expr") == 0) {
        expression_mode = 1;
        path = argv[2];
    } else {
        print_usage(argv[0]);
        return 64;
    }

    return expression_mode ? dump_expression_file(path) : dump_program_file(path);
}

static char *read_entire_file(const char *path) {
    FILE *file;
    char *buffer;
    long size;
    size_t read_size;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: failed to seek: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }

    size = ftell(file);
    if (size < 0) {
        fprintf(stderr, "%s: failed to size file: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: failed to rewind: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "%s: out of memory while reading file\n", path);
        fclose(file);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, file);
    if (read_size != (size_t)size) {
        fprintf(stderr, "%s: failed to read file contents\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [--expr] <source-file>\n", program_name);
    fprintf(stderr, "  Without --expr, parses the file as a full program.\n");
    fprintf(stderr, "  With --expr, parses the file as a single expression.\n");
}

static int dump_program_file(const char *path) {
    char *source;
    Parser parser;
    AstProgram program;
    const ParserError *error;

    source = read_entire_file(path);
    if (!source) {
        return 66;
    }

    parser_init(&parser, source);
    if (!parser_parse_program(&parser, &program)) {
        error = parser_get_error(&parser);
        if (error) {
            fprintf(stderr, "%s:%d:%d: parse error: %s\n",
                    path, error->token.line, error->token.column, error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!ast_dump_program(stdout, &program)) {
        fprintf(stderr, "%s: failed to write AST dump\n", path);
        ast_program_free(&program);
        parser_free(&parser);
        free(source);
        return 1;
    }

    ast_program_free(&program);
    parser_free(&parser);
    free(source);
    return 0;
}

static int dump_expression_file(const char *path) {
    char *source;
    Parser parser;
    AstExpression *expression;
    const ParserError *error;

    source = read_entire_file(path);
    if (!source) {
        return 66;
    }

    parser_init(&parser, source);
    expression = parser_parse_expression(&parser);
    if (!expression) {
        error = parser_get_error(&parser);
        if (error) {
            fprintf(stderr, "%s:%d:%d: parse error: %s\n",
                    path, error->token.line, error->token.column, error->message);
        }
        parser_free(&parser);
        free(source);
        return 1;
    }

    if (!ast_dump_expression(stdout, expression)) {
        fprintf(stderr, "%s: failed to write AST dump\n", path);
        ast_expression_free(expression);
        parser_free(&parser);
        free(source);
        return 1;
    }

    ast_expression_free(expression);
    parser_free(&parser);
    free(source);
    return 0;
}