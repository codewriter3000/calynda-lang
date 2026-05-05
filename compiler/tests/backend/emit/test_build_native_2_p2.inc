    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build manual malloc/free native executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(0, exit_code, "manual malloc/free native executable returns 0");
    unlink(output_path);
}


void test_build_native_typed_ptr_offset_stride(void) {
    /* ptr<int32> offset must advance by 4, not 8, so p[2] == 99 */
    static const char source[] =
        "start(string[] args) -> {\n"
        "    manual {\n"
        "        ptr<int32> p = malloc(12);\n"
        "        store(p, 0);\n"
        "        ptr<int32> q = offset(p, 1);\n"
        "        store(q, 0);\n"
        "        ptr<int32> r = offset(p, 2);\n"
        "        store(r, 99);\n"
        "        int64 val = deref(r);\n"
        "        free(p);\n"
        "        return val;\n"
        "    };\n"
        "    return 0;\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build typed ptr offset stride executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(99, exit_code, "typed ptr<int32> offset uses 4-byte stride");
    unlink(output_path);
}


void test_build_native_supports_string_index_and_length(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    return int32(args.length) + int32(args[0].length) + int32(args[0][0]);\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, (char *)"A", NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build string index/length executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(67, exit_code, "native executable supports array length, string length, and string indexing");
    unlink(output_path);
}

void test_build_native_supports_string_car_and_cdr(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    string text = args[0];\n"
        "    return int32(car(text)) + int32(cdr(text).length);\n"
        "};\n";
    char output_path[64];
    char *run_argv[] = { output_path, (char *)"Hello", NULL };
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build string car/cdr executable");
    exit_code = run_process(output_path, run_argv);
    ASSERT_EQ_INT(76,
                  exit_code,
                  "native executable supports string car and cdr semantics");
    unlink(output_path);
}
