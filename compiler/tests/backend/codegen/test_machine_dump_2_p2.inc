    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump), "emit runtime-backed machine program");
    ASSERT_CONTAINS("Unit name=start kind=start return=int32 frame_slots=6 spills=6 helper_slots=3",
                    dump,
                    "start unit reserves helper slots for runtime pack setup");
    ASSERT_CONTAINS("mov helper(0), global(seed)", dump,
                    "array literal elements stage through helper scratch slots");
    ASSERT_CONTAINS("call __calynda_rt_array_literal", dump,
                    "array literals lower through the runtime ABI");
    ASSERT_CONTAINS("mov helper(0), tag(text)", dump,
                    "template lowering writes tagged runtime template parts");
    ASSERT_CONTAINS("call __calynda_rt_template_build", dump,
                    "template building lowers through the runtime ABI");
    ASSERT_CONTAINS("mov rdi, global(stdlib)", dump,
                    "member load helper receives the target value in rdi");
    ASSERT_CONTAINS("mov rsi, symbol(print)", dump,
                    "member load helper receives the member symbol in rsi");
    ASSERT_CONTAINS("call __calynda_rt_member_load", dump,
                    "member access lowers through the runtime ABI");
    ASSERT_CONTAINS("mov helper(0), int32(2)", dump,
                    "callable dispatch stages arguments through helper scratch slots");
    ASSERT_CONTAINS("call __calynda_rt_call_callable", dump,
                    "callable-object dispatch lowers through the runtime ABI");
    ASSERT_CONTAINS("call __calynda_rt_store_index", dump,
                    "index writes lower through the runtime ABI");
    ASSERT_CONTAINS("mov frame(values), r10", dump,
                    "runtime helper results can still flow back into local frame slots");
    free(dump);
}


void test_machine_dump_routes_throw_terminator_to_runtime_helper(void) {
    static const char source[] =
        "int32 fail = () -> {\n"
        "    throw \"boom\";\n"
        "    return 0;\n"
        "};\n"
        "start(string[] args) -> fail();\n";
    char *dump;

    REQUIRE_TRUE(build_machine_dump_from_source(source, &dump), "emit throw machine program");
    ASSERT_CONTAINS("mov rdi, string(\"boom\")", dump,
                    "throw passes the thrown value in rdi");
    ASSERT_CONTAINS("call __calynda_rt_throw", dump,
                    "throw terminators lower through the throw runtime helper");
    free(dump);
}
