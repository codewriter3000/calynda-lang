    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build manual checked runtime failure program");
    char *argv[] = { output_path, NULL };
    REQUIRE_TRUE(run_capture_combined(output_path, argv, output, sizeof(output), &exit_code),
                 "run manual checked runtime failure program");
    ASSERT_EQ_INT(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  exit_code,
                  "manual checked out-of-bounds returns a normal runtime error exit code");
    ASSERT_CONTAINS("calynda bounds-check: out-of-bounds access",
                    output,
                    "manual checked runtime failure reports the bounds-check diagnostic");
    unlink(output_path);
}


/* ------------------------------------------------------------------ */
/*  B6: CAR multi-file E2E native build                               */
/* ------------------------------------------------------------------ */

static bool resolve_calynda_binary(char *buffer, size_t buffer_size) {
    extern const char *test_binary_path;
    if (test_binary_path) {
        char *slash = strrchr(test_binary_path, '/');
        if (slash) {
            size_t dir_len = (size_t)(slash - test_binary_path + 1);
            if (dir_len + sizeof("calynda") > buffer_size) return false;
            memcpy(buffer, test_binary_path, dir_len);
            memcpy(buffer + dir_len, "calynda", sizeof("calynda"));
            return true;
        }
    }
    if (sizeof("./build/calynda") > buffer_size) return false;
    strcpy(buffer, "./build/calynda");
    return true;
}

void test_build_native_car_multifile(void) {
    static const char main_cal[] =
        "start(string[] args) -> {\n"
        "    return add(30, 12);\n"
        "};\n";
    static const char math_cal[] =
        "int32 add = (int32 a, int32 b) -> a + b;\n";
    char car_path[128];
    char out_path[] = "/tmp/calynda-car-exe-XXXXXX";
    char calynda_bin[128];
    int fd_out, exit_code;
    FILE *f;

    REQUIRE_TRUE(resolve_calynda_binary(calynda_bin, sizeof(calynda_bin)),
                 "resolve calynda binary for CAR test");

    /* Write two source files into a temp directory and pack via CAR API */
    char dir_path[] = "/tmp/calynda-car-dir-XXXXXX";
    REQUIRE_TRUE(mkdtemp(dir_path) != NULL, "create temp dir for CAR test");

    /* Build the .car path inside the temp directory */
    snprintf(car_path, sizeof(car_path), "%s/project.car", dir_path);

    char main_path[128], math_path[128];
    snprintf(main_path, sizeof(main_path), "%s/main.cal", dir_path);
    snprintf(math_path, sizeof(math_path), "%s/math.cal", dir_path);

    f = fopen(main_path, "w");
    REQUIRE_TRUE(f != NULL, "write main.cal");
    fputs(main_cal, f);
    fclose(f);

    f = fopen(math_path, "w");
    REQUIRE_TRUE(f != NULL, "write math.cal");
    fputs(math_cal, f);
    fclose(f);

    /* Pack with calynda pack */
    char *pack_argv[] = { calynda_bin, "pack", dir_path, "-o", car_path, NULL };
    exit_code = run_process(calynda_bin, pack_argv);
    ASSERT_EQ_INT(0, exit_code, "calynda pack succeeds");

    /* Build from .car */
    fd_out = mkstemp(out_path);
    REQUIRE_TRUE(fd_out >= 0, "create temp exe for CAR test");
    close(fd_out);

    char *build_argv[] = { calynda_bin, "build", car_path, "-o", out_path, NULL };
    exit_code = run_process(calynda_bin, build_argv);
    ASSERT_EQ_INT(0, exit_code, "calynda build from .car succeeds");

    /* Run the built executable */
    char *run_argv[] = { out_path, NULL };
    exit_code = run_process(out_path, run_argv);
    ASSERT_EQ_INT(42, exit_code, "CAR multi-file program returns 42");

    unlink(main_path);
    unlink(math_path);
    unlink(car_path);
    rmdir(dir_path);
    unlink(out_path);
}

void test_build_native_threading_helpers(void) {
    static const char source[] =
        "type ExitCode = int32;\n"
        "start(string[] args) -> {\n"
        "    Thread t = spawn () -> {\n"
        "    };\n"
        "    Mutex m = Mutex.new();\n"
        "    m.lock();\n"
        "    m.unlock();\n"
        "    t.join();\n"
        "    ExitCode code = 0;\n"
        "    return code;\n"
        "};\n";
    char output_path[64];
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build threading helper program");
    {
        char *argv[] = { output_path, NULL };
        exit_code = run_process(output_path, argv);
    }
    ASSERT_EQ_INT(0, exit_code, "threading helper program returns 0");
    unlink(output_path);
}

void test_build_native_type_query_builtins(void) {
    static const char source[] =
        "start(string[] args) -> {\n"
        "    int32[] values = [1, 2, 3];\n"
        "    arr<?> mixed = [1, true];\n"
        "    string homogeneousType = typeof(values);\n"
        "    string heteroType = typeof(mixed);\n"
        "    bool ok = isint(1) && isfloat(float64(1)) && isbool(true) &&\n"
        "              isstring(homogeneousType) && isarray(values) && isarray(mixed) &&\n"
        "              issametype(values, cdr(values)) && !issametype(values, mixed);\n"
        "    return ok ? int32(homogeneousType.length + heteroType.length) : 1;\n"
        "};\n";
    char output_path[64];
    int exit_code;

    REQUIRE_TRUE(build_native_executable(source, output_path, sizeof(output_path)),
                 "build type-query builtin program");
    {
        char *argv[] = { output_path, NULL };
        exit_code = run_process(output_path, argv);
    }
    ASSERT_EQ_INT(8, exit_code, "type-query builtin program returns combined type-name lengths");
    unlink(output_path);
}
