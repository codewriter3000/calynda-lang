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
        fprintf(stderr,
                "runtime: %s() expects an array argument\n",
                builtin_name ? builtin_name : "array builtin");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return (const CalyndaRtArray *)(const void *)header;
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
        fprintf(stderr, "runtime: attempted to call a non-callable raw word (%" PRIu64 ")\n", callable);
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
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
    default:
        fprintf(stderr,
                "runtime: object of kind %s is not callable\n",
                calynda_rt_object_kind_name((CalyndaRtObjectKind)header->kind));
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
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

CalyndaRtWord __calynda_rt_stdlib_print1(CalyndaRtWord value) {
    return rt_stdout_print(true, value);
}

CalyndaRtWord __calynda_rt_member_load(CalyndaRtWord target, const char *member) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);

    if (!header) {
        fprintf(stderr, "runtime: attempted member access on non-object value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    if (!member) {
        fprintf(stderr, "runtime: attempted member access with a null member symbol\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    if (header == &__calynda_pkg_stdlib.header && strcmp(member, "print") == 0) {
        return rt_make_object_word(&STDOUT_PRINT_CALLABLE);
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
    }

    fprintf(stderr,
            "runtime: unsupported member load %s.%s\n",
            header == &__calynda_pkg_stdlib.header ? __calynda_pkg_stdlib.name : calynda_rt_object_kind_name((CalyndaRtObjectKind)header->kind),
            member);
    rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
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
        fprintf(stderr, "runtime: attempted index load on non-indexable value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    offset = (size_t)rt_signed_from_word(index);

    if (header->kind == CALYNDA_RT_OBJECT_STRING) {
        const CalyndaRtString *string = (const CalyndaRtString *)(const void *)header;

        if (offset >= string->length) {
            fprintf(stderr, "runtime: string index out of bounds (%zu)\n", offset);
            rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
        }
        return rt_word_from_signed((long long)(unsigned char)string->bytes[offset]);
    }

    if (header->kind == CALYNDA_RT_OBJECT_ARRAY) {
        const CalyndaRtArray *array = (const CalyndaRtArray *)(const void *)header;

        count = array->count;
        elements = array->elements;
    } else {
        const CalyndaRtHeteroArray *array = (const CalyndaRtHeteroArray *)(const void *)header;

        count = array->count;
        elements = array->elements;
    }

    if (offset >= count) {
        fprintf(stderr, "runtime: array index out of bounds (%zu)\n", offset);
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return elements[offset];
}

CalyndaRtWord __calynda_rt_array_literal(size_t element_count,
                                         const CalyndaRtWord *elements) {
    CalyndaRtArray *array = rt_new_array_object(element_count, elements);

    if (!array) {
        fprintf(stderr, "runtime: out of memory while creating array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    return rt_make_object_word(array);
}

CalyndaRtWord __calynda_rt_array_car(CalyndaRtWord target) {
    const CalyndaRtArray *array = rt_require_builtin_array("car", target);

    if (array->count == 0) {
        fprintf(stderr, "runtime: car() requires a non-empty array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return array->elements[0];
}

CalyndaRtWord __calynda_rt_array_cdr(CalyndaRtWord target) {
    const CalyndaRtArray *array = rt_require_builtin_array("cdr", target);
    CalyndaRtArray *tail;

    if (array->count == 0) {
        fprintf(stderr, "runtime: cdr() requires a non-empty array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    tail = rt_new_array_object(array->count - 1,
                               array->count > 1 ? array->elements + 1 : NULL);
    if (!tail) {
        fprintf(stderr, "runtime: out of memory while creating array tail\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    return rt_make_object_word(tail);
}

void __calynda_rt_store_index(CalyndaRtWord target,
                              CalyndaRtWord index,
                              CalyndaRtWord value) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);
    size_t offset;

    if (!header || header->kind != CALYNDA_RT_OBJECT_ARRAY) {
        fprintf(stderr, "runtime: attempted index store on non-array value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    offset = (size_t)rt_signed_from_word(index);
    if (offset >= ((const CalyndaRtArray *)(const void *)header)->count) {
        fprintf(stderr, "runtime: array index out of bounds (%zu)\n", offset);
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    ((CalyndaRtArray *)(void *)header)->elements[offset] = value;
}

void __calynda_rt_store_member(CalyndaRtWord target,
                               const char *member,
                               CalyndaRtWord value) {
    (void)target;
    (void)member;
    (void)value;
    fprintf(stderr, "runtime: member stores are not implemented for this first runtime surface\n");
    rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
}

void __calynda_rt_throw(CalyndaRtWord value) {
    char buffer[256];

    if (!rt_format_word_internal(value, buffer, sizeof(buffer))) {
        strcpy(buffer, "<unformattable>");
    }
    rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR, "Unhandled throw: %s\n", buffer);
}

CalyndaRtWord __calynda_rt_cast_value(CalyndaRtWord source,
                                      CalyndaRtTypeTag target_type_tag) {
    switch (target_type_tag) {
    case CALYNDA_RT_TYPE_BOOL:
        return source != 0 ? 1 : 0;

    case CALYNDA_RT_TYPE_INT32:
    case CALYNDA_RT_TYPE_INT64: {
        const CalyndaRtObjectHeader *header = calynda_rt_as_object(source);

        if (header && header->kind == CALYNDA_RT_OBJECT_STRING) {
            return rt_word_from_signed(strtoll(((const CalyndaRtString *)(const void *)header)->bytes,
                                               NULL,
                                               10));
        }
        return source;
    }

    case CALYNDA_RT_TYPE_STRING: {
        char buffer[128];

        if (rt_is_runtime_string_word(source)) {
            return source;
        }
        if (!rt_format_word_internal(source, buffer, sizeof(buffer))) {
            fprintf(stderr, "runtime: failed to stringify cast source\n");
            rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
        }
        return calynda_rt_make_string_copy(buffer);
    }

    case CALYNDA_RT_TYPE_ARRAY:
    case CALYNDA_RT_TYPE_CLOSURE:
    case CALYNDA_RT_TYPE_EXTERNAL:
    case CALYNDA_RT_TYPE_RAW_WORD:
    case CALYNDA_RT_TYPE_VOID:
    case CALYNDA_RT_TYPE_UNION:
    case CALYNDA_RT_TYPE_HETERO_ARRAY:
        return source;
    }

    return source;
}

int calynda_rt_start_process(CalyndaRtProgramStartEntry entry, int argc, char **argv) {
    CalyndaRtWord *volatile elements = NULL;
    CalyndaRtWord arguments = 0;
    size_t argument_count = 0;
    size_t i;
    CalyndaRtWord result = 0;
    RtFailureContext failure;
    int exit_code = 0;
    int process_failure;

    if (!entry) {
        fprintf(stderr, "runtime: missing native start entry\n");
        return CALYNDA_RT_EXIT_RUNTIME_ERROR;
    }

    rt_reset_process_failure();
    rt_failure_context_push(&failure);
    if (setjmp(failure.jump) == 0) {
        if (argc > 1) {
            argument_count = (size_t)(argc - 1);
            elements = calloc(argument_count, sizeof(*elements));
            if (!elements) {
                fprintf(stderr, "runtime: out of memory while boxing process arguments\n");
                exit_code = CALYNDA_RT_EXIT_RUNTIME_OOM;
                goto cleanup;
            }
            for (i = 0; i < argument_count; i++) {
                ((CalyndaRtWord *)elements)[i] = calynda_rt_make_string_copy(argv[i + 1]);
            }
        }

        arguments = __calynda_rt_array_literal(argument_count, (const CalyndaRtWord *)elements);
        free((void *)elements);
        elements = NULL;

        result = entry(arguments);
        exit_code = (int)(int32_t)result;
    } else {
        exit_code = failure.exit_code != 0
            ? failure.exit_code
            : CALYNDA_RT_EXIT_RUNTIME_ERROR;
    }

cleanup:
    free((void *)elements);
    rt_cleanup_registered_objects();
    rt_failure_context_pop(&failure);

    process_failure = rt_process_failure_code();
    rt_reset_process_failure();
    if (exit_code == 0 && process_failure != 0) {
        return process_failure;
    }
    return exit_code;
}
