#include "ast_dump.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
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
#define ASSERT_CONTAINS(needle, haystack, msg) do {                         \
    tests_run++;                                                            \
    if ((haystack) != NULL && strstr((haystack), (needle)) != NULL) {       \
        tests_passed++;                                                     \
    } else {                                                                \
        tests_failed++;                                                     \
        fprintf(stderr, "  FAIL [%s:%d] %s: missing \"%s\"\n",           \
                __FILE__, __LINE__, (msg), (needle));                       \
    }                                                                       \
} while (0)
#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

char *read_stream_contents(FILE *stream);


void test_dump_program_to_string(void) {
    static const char source[] =
        "package sample.app;\n"
        "import io.stdlib;\n"
        "public int32 inc = (int32 value) -> value + 1;\n"
        "start(string[] args) -> { return inc(41); };\n";
    static const char expected[] =
        "Program\n"
        "  PackageDecl: sample.app\n"
        "  ImportDecl: io.stdlib\n"
        "  BindingDecl name=inc type=int32 modifiers=[public]\n"
        "    Initializer:\n"
        "      LambdaExpr\n"
        "        Parameters:\n"
        "          Parameter name=value type=int32\n"
        "        BodyExpr:\n"
        "          BinaryExpr op=\"+\"\n"
        "            Left:\n"
        "              IdentifierExpr name=value\n"
        "            Right:\n"
        "              IntegerLiteral value=1\n"
        "  StartDecl\n"
        "    Parameters:\n"
        "      Parameter name=args type=string[]\n"
        "    BodyBlock:\n"
        "      Block\n"
        "        ReturnStmt\n"
        "          Value:\n"
        "            CallExpr\n"
        "              Callee:\n"
        "                IdentifierExpr name=inc\n"
        "              Arguments:\n"
        "                IntegerLiteral value=41\n";
    Parser parser;
    AstProgram program;
    char *dump;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse program for dump test");

    dump = ast_dump_program_to_string(&program);
    REQUIRE_TRUE(dump != NULL, "render program dump to string");
    ASSERT_EQ_STR(expected, dump, "program dump string");

    free(dump);
    ast_program_free(&program);
    parser_free(&parser);
}


void test_dump_expression_to_string(void) {
    static const char source[] = "foo(1, 2)[0].bar + int32(-value)";
    static const char expected[] =
        "BinaryExpr op=\"+\"\n"
        "  Left:\n"
        "    MemberExpr name=bar\n"
        "      Target:\n"
        "        IndexExpr\n"
        "          Target:\n"
        "            CallExpr\n"
        "              Callee:\n"
        "                IdentifierExpr name=foo\n"
        "              Arguments:\n"
        "                IntegerLiteral value=1\n"
        "                IntegerLiteral value=2\n"
        "          Index:\n"
        "            IntegerLiteral value=0\n"
        "  Right:\n"
        "    CastExpr type=int32\n"
        "      Expression:\n"
        "        UnaryExpr op=\"-\"\n"
        "          Operand:\n"
        "            IdentifierExpr name=value\n";
    Parser parser;
    AstExpression *expression;
    char *dump;

    parser_init(&parser, source);
    expression = parser_parse_expression(&parser);
    REQUIRE_TRUE(expression != NULL, "parse expression for dump test");

    dump = ast_dump_expression_to_string(expression);
    REQUIRE_TRUE(dump != NULL, "render expression dump to string");
    ASSERT_EQ_STR(expected, dump, "expression dump string");

    free(dump);
    ast_expression_free(expression);
    parser_free(&parser);
}


void test_dump_expression_to_file(void) {
    static const char source[] = "`prefix ${(() -> { return 1; })()} suffix`";
    static const char expected[] =
        "TemplateLiteral\n"
        "  TextPart \"prefix \"\n"
        "  ExpressionPart:\n"
        "    CallExpr\n"
        "      Callee:\n"
        "        GroupingExpr\n"
        "          Expression:\n"
        "            LambdaExpr\n"
        "              Parameters: []\n"
        "              BodyBlock:\n"
        "                Block\n"
        "                  ReturnStmt\n"
        "                    Value:\n"
        "                      IntegerLiteral value=1\n"
        "      Arguments: []\n"
        "  TextPart \" suffix\"\n";
    Parser parser;
    AstExpression *expression;
    FILE *stream;
    char *dump;

    parser_init(&parser, source);
    expression = parser_parse_expression(&parser);
    REQUIRE_TRUE(expression != NULL, "parse template expression for file dump test");

    stream = tmpfile();
    REQUIRE_TRUE(stream != NULL, "create temporary stream for dump output");
    REQUIRE_TRUE(ast_dump_expression(stream, expression), "write expression dump to file");

    dump = read_stream_contents(stream);
    REQUIRE_TRUE(dump != NULL, "read dumped expression from file");
    ASSERT_EQ_STR(expected, dump, "expression dump file output");

    free(dump);
    fclose(stream);
    ast_expression_free(expression);
    parser_free(&parser);
}


void test_dump_manual_block_with_memory_ops(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        int64 ptr = malloc(64);\n"
        "        free(ptr);\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    char *dump;

    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse manual block for dump");

    dump = ast_dump_program_to_string(&program);
    REQUIRE_TRUE(dump != NULL, "render manual block dump to string");
    ASSERT_CONTAINS("ManualStmt", dump, "dump contains ManualStmt");
    ASSERT_CONTAINS("MemoryOpExpr", dump, "dump contains MemoryOpExpr");
    ASSERT_CONTAINS("malloc", dump, "dump contains malloc op");
    ASSERT_CONTAINS("free", dump, "dump contains free op");

    free(dump);
    ast_program_free(&program);
    parser_free(&parser);
}

