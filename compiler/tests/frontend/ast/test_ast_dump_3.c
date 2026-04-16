#include "ast_dump.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

#define ASSERT_EQ_INT(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((expected) == (actual)) {                                           \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected %d, got %d\n",      \
                __FILE__, __LINE__, (msg), (int)(expected), (int)(actual)); \
    }                                                                       \
} while (0)
#define ASSERT_TRUE(condition, msg) do {                                    \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
    }                                                                       \
} while (0)
#define REQUIRE_TRUE(condition, msg) do {                                   \
    tests_run++;                                                            \
    if (condition) {                                                        \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s\n",                            \
                __FILE__, __LINE__, (msg));                                 \
        return;                                                             \
    }                                                                       \
} while (0)
#define ASSERT_EQ_STR(expected, actual, msg) do {                           \
    tests_run++;                                                            \
    if ((actual) != NULL && strcmp((expected), (actual)) == 0) {            \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: expected \"%s\", got \"%s\"\n", \
                __FILE__, __LINE__, (msg), (expected),                      \
                (actual) ? (actual) : "(null)");                           \
    }                                                                       \
} while (0)

void test_dump_program_parse_error_has_span(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    return 0\n"
        "};\n";
    Parser parser;
    AstProgram program;
    const ParserError *error;
    bool result;

    parser_init(&parser, source);
    result = parser_parse_program(&parser, &program);
    ASSERT_TRUE(!result, "malformed AST dump program is rejected");

    error = parser_get_error(&parser);
    REQUIRE_TRUE(error != NULL, "malformed AST dump program reports parser error");
    ASSERT_EQ_STR("Expected ';' after return statement.", error->message,
                  "parse error message for missing return semicolon");
    ASSERT_EQ_INT(TOK_RBRACE, error->token.type, "error token type is closing brace");
    ASSERT_EQ_INT(3, error->token.line, "error token line for missing return semicolon");
    ASSERT_EQ_INT(1, error->token.column,
                  "error token column for missing return semicolon");

    if (result) {
        ast_program_free(&program);
    }
    parser_free(&parser);
}
