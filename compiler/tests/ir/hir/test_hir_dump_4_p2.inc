    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse car/cdr HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build car/cdr HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check car/cdr HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build car/cdr HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "car/cdr HIR dump string is not NULL");
    ASSERT_CONTAINS("__calynda_rt_array_car", dump, "HIR lowers car to a direct helper");
    ASSERT_CONTAINS("__calynda_rt_array_cdr", dump, "HIR lowers cdr to a direct helper");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_lowers_string_car_and_cdr(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = \"hello\";\n"
        "    char head = car(text);\n"
        "    string tail = cdr(text);\n"
        "    return int32(head) + int32(tail.length);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse string car/cdr HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build string car/cdr HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check string car/cdr HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build string car/cdr HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "string car/cdr HIR dump string is not NULL");
    ASSERT_CONTAINS("Index type=char", dump, "HIR lowers string car to an index expression");
    ASSERT_CONTAINS("Literal kind=0 type=int32 text=0",
                    dump,
                    "HIR lowers string car with a zero literal index");
    ASSERT_CONTAINS("__calynda_rt_string_cdr",
                    dump,
                    "HIR lowers string cdr to the runtime string tail helper");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_inlines_default_arguments(void) {
    static const char source[] =
        "int32 add = (int32 left, int32 right = left + 1) -> left + right;\n"
        "start -> add(4);\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse HIR default argument source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build HIR default argument symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check HIR default argument source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for default arguments");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "HIR default-argument dump is not NULL");
    ASSERT_CONTAINS("Arg 1:\n                Literal kind=0 type=int32 text=4",
                    dump,
                    "HIR preserves explicit argument");
    ASSERT_CONTAINS("Arg 2:\n                Binary op=+ type=int32",
                    dump,
                    "HIR synthesizes omitted default argument");
    ASSERT_CONTAINS("Left:\n                    Literal kind=0 type=int32 text=4",
                    dump,
                    "default argument inlines earlier explicit arguments at the call site");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_synthesizes_default_initializers_for_omitted_bindings(void) {
    static const char source[] =
        "int32 counter;\n"
        "start(string[] args) -> {\n"
        "    bool ready;\n"
        "    string name;\n"
        "    return counter;\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program),
                 "parse HIR omitted initializer source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program),
                 "build HIR omitted initializer symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check HIR omitted initializer source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for omitted initializer bindings");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "HIR omitted-initializer dump is not NULL");
    ASSERT_CONTAINS("Literal kind=0 type=int32 text=0",
                    dump,
                    "HIR synthesizes zero for omitted int32 binding initializers");
    ASSERT_CONTAINS("Literal kind=2 type=bool value=false",
                    dump,
                    "HIR synthesizes false for omitted bool binding initializers");
    ASSERT_CONTAINS("Literal kind=6 type=string",
                    dump,
                    "HIR synthesizes null for omitted string binding initializers");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_expands_swap_statement(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    values[0] >< values[2];\n"
        "    return values[0];\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse swap HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build swap HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check swap HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for swap statement");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "swap HIR dump string is not NULL");
    ASSERT_CONTAINS("Block statements=5", dump, "swap expands into extra HIR statements");
    ASSERT_CONTAINS("Local name=__swap_tmp_0 type=int32", dump, "swap introduces temp local");
    ASSERT_CONTAINS("Assignment op=0 type=int32", dump, "swap lowers to assignment expressions");
    ASSERT_CONTAINS("Symbol name=__swap_tmp_0 kind=local type=int32", dump,
                    "swap reuses temp local in final assignment");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

void test_hir_dump_lowers_overload_calls_to_selected_signature(void) {
    static const char source[] =
        "int32 pick = (int32 value) -> value;\n"
        "int32 pick = (int32 left, int32 right) -> left + right;\n"
        "start(string[] args) -> {\n"
        "    return pick(1, 2);\n"
        "};\n";
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse overload HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build overload HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check overload HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build overload HIR");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "overload HIR dump string is not NULL");
    ASSERT_CONTAINS("callable=(int32) -> int32", dump,
                    "HIR dump keeps unary overload signature");
    ASSERT_CONTAINS("Symbol name=pick kind=top-level binding type=int32 callable=(int32, int32) -> int32",
                    dump,
                    "HIR call uses the selected binary overload signature");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}

static bool append_scene60_source(char *buffer,
                                  size_t buffer_size,
                                  size_t *length,
                                  const char *format,
                                  ...) {
    va_list args;
    int written;

    if (!buffer || !length || !format || *length >= buffer_size) {
        return false;
    }

    va_start(args, format);
    written = vsnprintf(buffer + *length,
                        buffer_size - *length,
                        format,
                        args);
    va_end(args);

    if (written < 0 || (size_t)written >= buffer_size - *length) {
        return false;
    }

    *length += (size_t)written;
    return true;
}

void test_hir_dump_lowers_monolithic_scene60_hetero_array(void) {
    static const int column_x[10] = { 24, 196, 368, 540, 712, 884, 1056, 1228, 1400, 1572 };
    static const int row_y[6] = { 48, 204, 360, 516, 672, 828 };
    static const int vx[10] = { -5, -4, -3, -2, -1, 1, 2, 3, 4, 5 };
    static const int vy[6] = { -5, -3, -1, 1, 3, 5 };
    static const int rgb_values[5] = { 0x00, 0x40, 0x80, 0xC0, 0xFF };
    char source[32768];
    size_t length = 0;
    size_t scene_count = 0;
    Parser parser;
    AstProgram program;
    SymbolTable symbols;
    TypeChecker checker;
    HirProgram hir;
    char *dump;
    size_t ri;
    size_t gi;
    size_t bi;

    REQUIRE_TRUE(append_scene60_source(
                     source,
                     sizeof(source),
                     &length,
                     "package app;\n\n"
                     "type ScreenObject = arr<?>;\n\n"
                     "uint32 RGB = (uint32 r, uint32 g, uint32 b) -> {\n"
                     "    return (r << 16) | (g << 8) | b;\n"
                     "};\n\n"
                     "ScreenObject screen_make_object = (int32 x, int32 y, int32 vx, int32 vy, uint32 color) -> {\n"
                     "    return [0, x, y, 168, 32, vx, vy, color, 0];\n"
                     "};\n\n"
                     "int32 run = manual() -> {\n"),
                 "format monolithic scene source prelude");

    for (ri = 0; ri < 5; ri++) {
        for (gi = 0; gi < 5; gi++) {
            for (bi = 0; bi < 5; bi++) {
                size_t row;
                size_t col;

                if (ri == gi || ri == bi || gi == bi) {
                    continue;
                }

                row = scene_count / 10;
                col = scene_count % 10;
                REQUIRE_TRUE(
                    append_scene60_source(source,
                                          sizeof(source),
                                          &length,
                                          "    ScreenObject heysyd_%02zu = screen_make_object(%d, %d, %d, %d, RGB(0x%02X, 0x%02X, 0x%02X));\n",
                                          scene_count,
                                          column_x[col],
                                          row_y[row],
                                          vx[col],
                                          vy[row],
                                          rgb_values[ri],
                                          rgb_values[gi],
                                          rgb_values[bi]),
                    "format monolithic scene object binding");
                scene_count++;
            }
        }
    }

    REQUIRE_TRUE(scene_count == 60, "generate 60 ordered RGB scene bindings");
    REQUIRE_TRUE(append_scene60_source(source,
                                       sizeof(source),
                                       &length,
                                       "\n    arr<?> sceneObjects = ["),
                 "format monolithic scene array prelude");
    for (scene_count = 0; scene_count < 60; scene_count++) {
        REQUIRE_TRUE(append_scene60_source(source,
                                           sizeof(source),
                                           &length,
                                           scene_count == 0
                                               ? "heysyd_%02zu"
                                               : ", heysyd_%02zu",
                                           scene_count),
                     "format monolithic scene array element");
    }
    REQUIRE_TRUE(append_scene60_source(source,
                                       sizeof(source),
                                       &length,
                                       "];\n"
                                       "    return sceneObjects.length;\n"
                                       "};\n\n"
                                       "boot -> run();\n"),
                 "format monolithic scene source suffix");

    symbol_table_init(&symbols);
    type_checker_init(&checker);
    hir_program_init(&hir);
    parser_init(&parser, source);
    REQUIRE_TRUE(parser_parse_program(&parser, &program), "parse monolithic scene HIR source");
    REQUIRE_TRUE(symbol_table_build(&symbols, &program), "build monolithic scene HIR symbols");
    REQUIRE_TRUE(type_checker_check_program(&checker, &program, &symbols),
                 "type check monolithic scene HIR source");
    REQUIRE_TRUE(hir_build_program(&hir, &program, &symbols, &checker),
                 "build HIR for monolithic scene array");

    dump = hir_dump_program_to_string(&hir);
    REQUIRE_TRUE(dump != NULL, "monolithic scene HIR dump string is not NULL");
    ASSERT_CONTAINS("Local name=heysyd_14", dump,
                    "HIR dump contains the previously missing local symbol");
    ASSERT_CONTAINS("Local name=sceneObjects", dump,
                    "HIR dump contains the monolithic scene array local");

    free(dump);
    hir_program_free(&hir);
    type_checker_free(&checker);
    symbol_table_free(&symbols);
    ast_program_free(&program);
    parser_free(&parser);
}
