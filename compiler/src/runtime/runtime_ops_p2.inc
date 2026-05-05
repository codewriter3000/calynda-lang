        if (offset >= string->length) {
            rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                      "runtime: string index %zu out of bounds (string length: %zu)\n",
                      offset, string->length);
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
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: array index %zu out of bounds (array length: %zu)\n",
                  offset, count);
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
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);

    if (header && header->kind == CALYNDA_RT_OBJECT_HETERO_ARRAY) {
        const CalyndaRtHeteroArray *arr = (const CalyndaRtHeteroArray *)(const void *)header;

        if (arr->count == 0) {
            fprintf(stderr, "runtime: car() called on an empty hetero array\n");
            rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
        }
        return arr->elements[0];
    }
    {
        const CalyndaRtArray *array = rt_require_builtin_array("car", target);

        if (array->count == 0) {
            fprintf(stderr, "runtime: car() called on an empty array\n");
            rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
        }

        return array->elements[0];
    }
}

CalyndaRtWord __calynda_rt_array_cdr(CalyndaRtWord target) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);

    if (header && header->kind == CALYNDA_RT_OBJECT_HETERO_ARRAY) {
        const CalyndaRtHeteroArray *arr = (const CalyndaRtHeteroArray *)(const void *)header;

        if (arr->count == 0) {
            fprintf(stderr, "runtime: cdr() called on an empty hetero array\n");
            rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
        }
        /* Return a new hetero array without the first element */
        return __calynda_rt_hetero_array_cdr(target);
    }
    {
        const CalyndaRtArray *array = rt_require_builtin_array("cdr", target);
        CalyndaRtArray *tail;

        if (array->count == 0) {
            fprintf(stderr, "runtime: cdr() called on an empty array\n");
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
}

CalyndaRtWord __calynda_rt_string_cdr(CalyndaRtWord target) {
    const CalyndaRtString *string = rt_require_builtin_string("cdr", target);
    CalyndaRtString *tail;

    if (string->length == 0) {
        fprintf(stderr, "runtime: cdr() called on an empty string\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    tail = rt_new_string_object(string->bytes + 1, string->length - 1);
    if (!tail) {
        fprintf(stderr, "runtime: out of memory while creating string tail\n");
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
        char desc[64];
        rt_describe_word(target, desc, sizeof(desc));
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: cannot store by index into %s — only arrays support index assignment\n",
                  desc);
    }

    offset = (size_t)rt_signed_from_word(index);
    if (offset >= ((const CalyndaRtArray *)(const void *)header)->count) {
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_ERROR,
                  "runtime: array index %zu out of bounds (array length: %zu)\n",
                  offset, ((const CalyndaRtArray *)(const void *)header)->count);
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

#include "runtime_ops_p3.inc"
