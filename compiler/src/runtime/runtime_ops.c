#include "runtime_internal.h"

static CalyndaRtWord rt_stdout_print(bool has_value, CalyndaRtWord value) {
    char buffer[256];

    if (has_value) {
        if (!rt_format_word_internal(value, buffer, sizeof(buffer))) {
            strcpy(buffer, "<unformattable>");
        }
        printf("%s\n", buffer);
    } else {
        printf("\n");
    }

    return 0;
}

static const CalyndaRtArray *rt_require_builtin_array(const char *builtin_name,
                                                      CalyndaRtWord target) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);

    if (!header || header->kind != CALYNDA_RT_OBJECT_ARRAY) {
        char desc[64];
        rt_describe_word(target, desc, sizeof(desc));
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: %s() expects an array argument, got %s\n",
                  builtin_name ? builtin_name : "array builtin",
                  desc);
    }

    return (const CalyndaRtArray *)(const void *)header;
}

static const CalyndaRtString *rt_require_builtin_string(const char *builtin_name,
                                                        CalyndaRtWord target) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);

    if (!header || header->kind != CALYNDA_RT_OBJECT_STRING) {
        char desc[64];
        rt_describe_word(target, desc, sizeof(desc));
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: %s() expects a string argument, got %s\n",
                  builtin_name ? builtin_name : "string builtin",
                  desc);
    }

    return (const CalyndaRtString *)(const void *)header;
}

CalyndaRtWord __calynda_rt_closure_new(CalyndaRtClosureEntry code_ptr,
                                       size_t capture_count,
                                       const CalyndaRtWord *captures) {
    CalyndaRtClosure *closure = rt_new_closure_object(code_ptr, capture_count, captures);

    if (!closure) {
        fprintf(stderr, "runtime: out of memory while creating closure\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    return rt_make_object_word(closure);
}

CalyndaRtWord __calynda_rt_call_callable(CalyndaRtWord callable,
                                         size_t argument_count,
                                         const CalyndaRtWord *arguments) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(callable);

    if (!header) {
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: attempted to call int64 %" PRId64 ", which is not callable\n"
                  "   note: only closures and extern-callables are callable\n",
                  (int64_t)callable);
    }

    switch ((CalyndaRtObjectKind)header->kind) {
    case CALYNDA_RT_OBJECT_CLOSURE: {
        const CalyndaRtClosure *closure = (const CalyndaRtClosure *)(const void *)header;

        return closure->entry(closure->captures,
                              closure->capture_count,
                              arguments,
                              argument_count);
    }
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE:
        return rt_dispatch_extern_callable((const CalyndaRtExternCallable *)(const void *)header,
                                           argument_count,
                                           arguments);
    default: {
        char desc[64];
        rt_describe_word(callable, desc, sizeof(desc));
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: %s is not callable\n"
                  "   note: only closures and extern-callables are callable\n",
                  desc);
    }
    }
}

CalyndaRtWord calynda_rt_callable_dispatch(CalyndaRtWord callable,
                                           const CalyndaRtWord *arguments,
                                           size_t argument_count) {
    return __calynda_rt_call_callable(callable, argument_count, arguments);
}

CalyndaRtWord __calynda_rt_stdlib_print0(void) {
    return rt_stdout_print(false, 0);
}

/* ---- Heap cells for capture-by-reference -------------------------------- */

CalyndaRtWord __calynda_rt_cell_alloc(CalyndaRtWord initial_value) {
    CalyndaRtWord *cell = malloc(sizeof(CalyndaRtWord));
    if (!cell) {
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_OOM,
                  "runtime: out of memory while creating capture cell\n");
    }
    *cell = initial_value;
    return (CalyndaRtWord)(uintptr_t)cell;
}

CalyndaRtWord __calynda_rt_cell_read(CalyndaRtWord cell) {
    return *(CalyndaRtWord *)(uintptr_t)cell;
}

CalyndaRtWord __calynda_rt_cell_write(CalyndaRtWord cell, CalyndaRtWord value) {
    *(CalyndaRtWord *)(uintptr_t)cell = value;
    return value;
}

/* ------------------------------------------------------------------------- */

CalyndaRtWord __calynda_rt_stdlib_print1(CalyndaRtWord value) {
    return rt_stdout_print(true, value);
}

static CalyndaRtWord rt_stdin_input(bool has_prompt, CalyndaRtWord prompt) {
    char line[4096];
    size_t length;
    CalyndaRtString *str;

    if (has_prompt) {
        char prompt_buf[256];
        if (!rt_format_word_internal(prompt, prompt_buf, sizeof(prompt_buf))) {
            prompt_buf[0] = '\0';
        }
        printf("%s", prompt_buf);
        fflush(stdout);
    }

    if (!fgets(line, sizeof(line), stdin)) {
        line[0] = '\0';
    }

    length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
        line[--length] = '\0';
    }

    str = rt_new_string_object(line, length);
    if (!str) {
        fprintf(stderr, "runtime: out of memory while reading input\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    return rt_make_object_word(str);
}

CalyndaRtWord __calynda_rt_stdlib_input0(void) {
    return rt_stdin_input(false, 0);
}

CalyndaRtWord __calynda_rt_stdlib_input1(CalyndaRtWord prompt) {
    return rt_stdin_input(true, prompt);
}

CalyndaRtWord __calynda_rt_member_load(CalyndaRtWord target, const char *member) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);

    if (!header) {
        char desc[64];
        rt_describe_word(target, desc, sizeof(desc));
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: cannot access member '.%s' on %s\n",
                  member ? member : "?", desc);
    }
    if (!member) {
        fprintf(stderr, "runtime: attempted member access with a null member symbol\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    if (header == &__calynda_pkg_stdlib.header && strcmp(member, "print") == 0) {
        return rt_make_object_word(&STDOUT_PRINT_CALLABLE);
    }
    if (header == &__calynda_pkg_stdlib.header && strcmp(member, "input") == 0) {
        return rt_make_object_word(&STDIN_INPUT_CALLABLE);
    }
    if (strcmp(member, "length") == 0) {
        if (header->kind == CALYNDA_RT_OBJECT_STRING) {
            return rt_word_from_signed(
                (long long)((const CalyndaRtString *)(const void *)header)->length);
        }
        if (header->kind == CALYNDA_RT_OBJECT_ARRAY) {
            return rt_word_from_signed(
                (long long)((const CalyndaRtArray *)(const void *)header)->count);
        }
        if (header->kind == CALYNDA_RT_OBJECT_HETERO_ARRAY) {
            return rt_word_from_signed(
                (long long)((const CalyndaRtHeteroArray *)(const void *)header)->count);
        }
    }

    {
        char desc[64];
        rt_describe_word(target, desc, sizeof(desc));
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: %s has no member '%s'\n",
                  desc, member);
    }
}

CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);
    const CalyndaRtWord *elements;
    size_t count;
    size_t offset;

    if (!header ||
        (header->kind != CALYNDA_RT_OBJECT_ARRAY &&
         header->kind != CALYNDA_RT_OBJECT_HETERO_ARRAY &&
         header->kind != CALYNDA_RT_OBJECT_STRING)) {
        char desc[64];
        rt_describe_word(target, desc, sizeof(desc));
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: cannot index %s — only arrays and strings are indexable\n",
                  desc);
    }

    offset = (size_t)rt_signed_from_word(index);

    if (header->kind == CALYNDA_RT_OBJECT_STRING) {
        const CalyndaRtString *string = (const CalyndaRtString *)(const void *)header;

#include "runtime_ops_p2.inc"
