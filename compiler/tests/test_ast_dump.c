#include "../src/ast_dump.h"
#include "../src/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

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

#define RUN_TEST(fn) do {                                                   \
    printf("  %s ...\n", #fn);                                            \
    fn();                                                                   \
} while (0)

static char *read_stream_contents(FILE *stream) {
    char *buffer;
    long size;
    size_t read_size;

    if (!stream) {
        return NULL;
    }

    if (fflush(stream) != 0 || fseek(stream, 0, SEEK_END) != 0) {
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static void test_dump_program_to_string(void) {
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

static void test_dump_expression_to_string(void) {
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

static void test_dump_expression_to_file(void) {
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

int main(void) {
    printf("Running AST dump tests...\n\n");

    RUN_TEST(test_dump_program_to_string);
    RUN_TEST(test_dump_expression_to_string);
    RUN_TEST(test_dump_expression_to_file);

    printf("\n========================================\n");
    printf("  Total: %d  |  Passed: %d  |  Failed: %d\n",
           tests_run, tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}