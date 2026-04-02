#include "../src/parser.h"
#include "../src/semantic_dump.h"

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

static void test_semantic_dump_program_to_string(void) {
    static const char source[] =
        "package sample.app;\n"
        "import core.math;\n"
        "int32 add = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    var math = 1;\n"
        "    var total = add(math, 2);\n"
        "    return total;\n"
        "};\n";
    static const char expected[] =
        "SemanticProgram\n"
        "  Package: name=app qualified=sample.app type=<external> span=1:16\n"
        "  Imports:\n"
        "    Import name=math qualified=core.math type=<external> span=2:13\n"
        "  Scopes:\n"
        "    Scope kind=program\n"
        "      Symbols:\n"
        "        Symbol name=add kind=top-level binding declared=int32 type=int32 callable=(int32, int32) -> int32 final=false span=3:7\n"
        "      Resolutions: []\n"
        "      Unresolved: []\n"
        "      Children:\n"
        "        Scope kind=lambda span=3:13\n"
        "          Symbols:\n"
        "            Symbol name=left kind=parameter declared=int32 type=int32 final=false span=3:20\n"
        "            Symbol name=right kind=parameter declared=int32 type=int32 final=false span=3:32\n"
        "          Resolutions:\n"
        "            Resolution name=left at=3:42 -> parameter left type=int32\n"
        "            Resolution name=right at=3:49 -> parameter right type=int32\n"
        "          Unresolved: []\n"
        "          Children: []\n"
        "        Scope kind=start span=4:1\n"
        "          Symbols:\n"
        "            Symbol name=args kind=parameter declared=string[] type=string[] final=false span=4:16\n"
        "          Resolutions: []\n"
        "          Unresolved: []\n"
        "          Children:\n"
        "            Scope kind=block statements=3\n"
        "              Symbols:\n"
        "                Symbol name=math kind=local declared=var type=int32 final=false span=5:9 shadows=import math at 2:13\n"
        "                Symbol name=total kind=local declared=var type=int32 final=false span=6:9\n"
        "              Resolutions:\n"
        "                Resolution name=add at=6:17 -> top-level binding add type=int32 callable=(int32, int32) -> int32\n"
        "                Resolution name=math at=6:21 -> local math type=int32\n"
        "                Resolution name=total at=7:12 -> local total type=int32\n"
        "              Unresolved: []\n"
        "              Children: []\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse program for semantic dump test");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for semantic dump test");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check program for semantic dump test");

    dump = semantic_dump_program_to_string(&symbols, &checker);
    REQUIRE_TRUE(dump != NULL, "render semantic dump to string");
    ASSERT_EQ_STR(expected, dump, "semantic dump string");

    free(dump);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static void test_semantic_dump_program_to_file_with_unresolved_identifier(void) {
    static const char source[] = "start(string[] args) -> missing + 1;\n";
    static const char expected[] =
        "SemanticProgram\n"
        "  Package: <none>\n"
        "  Imports: []\n"
        "  TypeCheckError: 1:25: Unresolved identifier 'missing'.\n"
        "  Scopes:\n"
        "    Scope kind=program\n"
        "      Symbols: []\n"
        "      Resolutions: []\n"
        "      Unresolved: []\n"
        "      Children:\n"
        "        Scope kind=start span=1:1\n"
        "          Symbols:\n"
        "            Symbol name=args kind=parameter declared=string[] type=string[] final=false span=1:16\n"
        "          Resolutions: []\n"
        "          Unresolved:\n"
        "            Unresolved name=missing at=1:25\n"
        "          Children: []\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    FILE *stream;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse unresolved semantic program");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build symbols for unresolved semantic program");
    ASSERT_TRUE(!type_checker_check_program(&checker, &program, &symbols),
                "unresolved identifier fails type checking");

    stream = tmpfile();
    REQUIRE_TRUE(stream != NULL, "create temporary stream for semantic dump output");
    REQUIRE_TRUE(semantic_dump_program(stream, &symbols, &checker),
                 "write semantic dump to file");

    dump = read_stream_contents(stream);
    REQUIRE_TRUE(dump != NULL, "read semantic dump from file");
    ASSERT_EQ_STR(expected, dump, "semantic dump file output");

    fclose(stream);
    free(dump);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

int main(void) {
    printf("Running semantic dump tests...\n");

    RUN_TEST(test_semantic_dump_program_to_string);
    RUN_TEST(test_semantic_dump_program_to_file_with_unresolved_identifier);

    printf("\nSemantic dump tests run: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}