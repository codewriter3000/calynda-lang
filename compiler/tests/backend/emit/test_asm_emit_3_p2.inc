    REQUIRE_TRUE(build_riscv64_assembly(source, &assembly), "emit rv64 assembly text");
    /* Must NOT contain x86_64 or ARM64 markers */
    ASSERT_TRUE(strstr(assembly, ".intel_syntax") == NULL,
                "rv64 output must not contain .intel_syntax");
    ASSERT_TRUE(strstr(assembly, "QWORD PTR") == NULL,
                "rv64 output must not contain x86_64 memory operands");
    ASSERT_TRUE(strstr(assembly, "stp ") == NULL,
                "rv64 output must not contain arm64 stp instructions");
    /* RV64 structural markers */
    ASSERT_CONTAINS(".globl calynda_unit_add", assembly,
                    "rv64 output exports the add unit symbol");
    ASSERT_CONTAINS(".globl calynda_program_start", assembly,
                    "rv64 output emits a callable start wrapper");
    ASSERT_CONTAINS(".globl main", assembly,
                    "rv64 output emits a process entry point");
    /* RV64 ABI: sd/ld prologue, call instruction, s0 frame pointer */
    ASSERT_CONTAINS("sd ra,", assembly,
                    "rv64 prologue saves return address");
    ASSERT_CONTAINS("sd s0,", assembly,
                    "rv64 prologue saves frame pointer");
    ASSERT_CONTAINS("addi s0, sp,", assembly,
                    "rv64 prologue sets up frame pointer");
    ASSERT_CONTAINS("call calynda_unit_add", assembly,
                    "rv64 direct calls use call instruction");
    free(assembly);
}

void test_asm_emit_riscv64_runtime_backed_program(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "int32 seed = 1;\n"
        "start(string[] args) -> {\n"
        "    bool flag = true;\n"
        "    var values = [seed, 2, 3];\n"
        "    int32 chosen = flag ? values[1] : values[0];\n"
        "    var render = (int32 x) -> `value ${chosen + ((int32 y) -> y + x)(1)}`;\n"
        "    values[0] += chosen;\n"
        "    stdlib.print(render(2));\n"
        "    return values[0];\n"
        "};\n";
    char *assembly;

    REQUIRE_TRUE(build_riscv64_assembly(source, &assembly), "emit rv64 runtime assembly text");
    ASSERT_CONTAINS("call __calynda_rt_array_literal", assembly,
                    "rv64 arrays lower to call runtime helpers");
    ASSERT_CONTAINS("call __calynda_rt_template_build", assembly,
                    "rv64 templates lower to call runtime helpers");
    ASSERT_CONTAINS("call calynda_rt_start_process", assembly,
                    "rv64 start wrapper calls runtime startup");
    ASSERT_CONTAINS("la a0, calynda_program_start", assembly,
                    "rv64 entry glue uses la for PIC addressing");
    ASSERT_CONTAINS("calynda_closure_start_lambda", assembly,
                    "rv64 lambda closures lower through callable wrapper symbols");
    free(assembly);
}

void test_asm_emit_boot_riscv64_emits_start_label(void) {
    static const char source[] = "boot -> 42;\n";
    char *assembly;

    REQUIRE_TRUE(build_riscv64_assembly(source, &assembly),
                 "emit boot rv64 assembly text");
    ASSERT_CONTAINS(".globl _start", assembly, "boot rv64 emits _start label");
    /*
     * alpha.2 freestanding contract: after the boot unit returns, _start
     * spins in an infinite loop (j 1b).  No Linux exit ecall is emitted —
     * that assumption broke any non-Linux target.
     */
    ASSERT_CONTAINS("j 1b", assembly, "boot rv64 spins after boot unit returns (freestanding)");
    ASSERT_TRUE(strstr(assembly, "li a7, 93") == NULL,
                "boot rv64 does not emit Linux sys_exit (ecall) sequence");
    ASSERT_TRUE(strstr(assembly, ".globl main") == NULL,
                "boot rv64 does not emit main entry point");
    ASSERT_TRUE(strstr(assembly, "calynda_program_start") == NULL,
                "boot rv64 does not emit calynda_program_start wrapper");
    free(assembly);
}

void test_asm_emit_boot_riscv64_statically_lowers_imported_print(void) {
    static const char source[] =
        "import io.stdlib;\n"
        "boot -> {\n"
        "    stdlib.print();\n"
        "    return 0;\n"
        "};\n";
    char *assembly;

    REQUIRE_TRUE(build_riscv64_assembly(source, &assembly),
                 "emit rv64 freestanding imported print assembly text");
    ASSERT_CONTAINS("call __calynda_rt_stdlib_print0", assembly,
                    "boot imported print lowers to a direct helper call");
    ASSERT_TRUE(strstr(assembly, "__calynda_pkg_stdlib") == NULL,
                "boot imported print no longer references hosted package objects");
    ASSERT_TRUE(strstr(assembly, "__calynda_rt_member_load") == NULL,
                "boot imported print no longer lowers through hosted member dispatch");
    ASSERT_TRUE(strstr(assembly, "__calynda_rt_call_callable") == NULL,
                "boot imported print no longer lowers through hosted callable dispatch");
    free(assembly);
}
