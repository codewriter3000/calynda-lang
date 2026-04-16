#include "runtime_internal.h"

CalyndaRtWord __calynda_rt_closure_new(CalyndaRtClosureEntry code_ptr,
                                       size_t capture_count,
                                       const CalyndaRtWord *captures) {
    CalyndaRtClosure *closure = rt_new_closure_object(code_ptr, capture_count, captures);

    if (!closure) {
        fprintf(stderr, "runtime: out of memory while creating closure\n");
        abort();
    }

    return rt_make_object_word(closure);
}

CalyndaRtWord __calynda_rt_call_callable(CalyndaRtWord callable,
                                         size_t argument_count,
                                         const CalyndaRtWord *arguments) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(callable);

    if (!header) {
        fprintf(stderr, "runtime: attempted to call a non-callable raw word (%" PRIu64 ")\n", callable);
        abort();
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
        abort();
    }
}

CalyndaRtWord calynda_rt_callable_dispatch(CalyndaRtWord callable,
                                           const CalyndaRtWord *arguments,
                                           size_t argument_count) {
    return __calynda_rt_call_callable(callable, argument_count, arguments);
}

CalyndaRtWord __calynda_rt_member_load(CalyndaRtWord target, const char *member) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);

    if (!header) {
        fprintf(stderr, "runtime: attempted member access on non-object value\n");
        abort();
    }
    if (!member) {
        fprintf(stderr, "runtime: attempted member access with a null member symbol\n");
        abort();
    }

    if (header == &__calynda_pkg_stdlib.header && strcmp(member, "print") == 0) {
        return rt_make_object_word(&STDOUT_PRINT_CALLABLE);
    }

    fprintf(stderr,
            "runtime: unsupported member load %s.%s\n",
            header == &__calynda_pkg_stdlib.header ? __calynda_pkg_stdlib.name : calynda_rt_object_kind_name((CalyndaRtObjectKind)header->kind),
            member);
    abort();
}

CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);
    const CalyndaRtWord *elements;
    size_t count;
    size_t offset;

    if (!header ||
        (header->kind != CALYNDA_RT_OBJECT_ARRAY &&
         header->kind != CALYNDA_RT_OBJECT_HETERO_ARRAY)) {
        fprintf(stderr, "runtime: attempted index load on non-array value\n");
        abort();
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

    offset = (size_t)rt_signed_from_word(index);
    if (offset >= count) {
        fprintf(stderr, "runtime: array index out of bounds (%zu)\n", offset);
        abort();
    }

    return elements[offset];
}

CalyndaRtWord __calynda_rt_array_literal(size_t element_count,
                                         const CalyndaRtWord *elements) {
    CalyndaRtArray *array = rt_new_array_object(element_count, elements);

    if (!array) {
        fprintf(stderr, "runtime: out of memory while creating array\n");
        abort();
    }

    return rt_make_object_word(array);
}

void __calynda_rt_store_index(CalyndaRtWord target,
                              CalyndaRtWord index,
                              CalyndaRtWord value) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);
    size_t offset;

    if (!header || header->kind != CALYNDA_RT_OBJECT_ARRAY) {
        fprintf(stderr, "runtime: attempted index store on non-array value\n");
        abort();
    }

    offset = (size_t)rt_signed_from_word(index);
    if (offset >= ((const CalyndaRtArray *)(const void *)header)->count) {
        fprintf(stderr, "runtime: array index out of bounds (%zu)\n", offset);
        abort();
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
    abort();
}

void __calynda_rt_throw(CalyndaRtWord value) {
    char buffer[256];

    if (!rt_format_word_internal(value, buffer, sizeof(buffer))) {
        strcpy(buffer, "<unformattable>");
    }
    fprintf(stderr, "Unhandled throw: %s\n", buffer);
    abort();
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
            abort();
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
    CalyndaRtWord *elements = NULL;
    CalyndaRtWord arguments;
    size_t argument_count = 0;
    size_t i;
    CalyndaRtWord result;

    if (!entry) {
        fprintf(stderr, "runtime: missing native start entry\n");
        return 70;
    }

    if (argc > 1) {
        argument_count = (size_t)(argc - 1);
        elements = calloc(argument_count, sizeof(*elements));
        if (!elements) {
            fprintf(stderr, "runtime: out of memory while boxing process arguments\n");
            return 71;
        }
        for (i = 0; i < argument_count; i++) {
            elements[i] = calynda_rt_make_string_copy(argv[i + 1]);
        }
    }

    arguments = __calynda_rt_array_literal(argument_count, elements);
    free(elements);

    result = entry(arguments);
    rt_cleanup_registered_objects();
    return (int)(int32_t)result;
}
