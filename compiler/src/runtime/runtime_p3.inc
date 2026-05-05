    atomic_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    atomic_object->header.kind = CALYNDA_RT_OBJECT_ATOMIC;
    if (!rt_register_object_pointer(atomic_object)) {
        free(atomic_object);
        return NULL;
    }
    return atomic_object;
}

CalyndaRtWord __calynda_typeof(CalyndaRtWord value, CalyndaRtWord type_text) {
    const char *type_bytes = rt_require_type_text(type_text);
    size_t base_length = rt_type_text_base_length(type_bytes);
    char *base_text;
    CalyndaRtWord result;

    (void)value;

    base_text = copy_text_n(type_bytes, base_length);
    if (!base_text) {
        fprintf(stderr, "runtime: out of memory while computing typeof()\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    result = calynda_rt_make_string_copy(base_text);
    free(base_text);
    return result;
}

CalyndaRtWord __calynda_isint(CalyndaRtWord value, CalyndaRtWord type_text) {
    const char *type_bytes = rt_require_type_text(type_text);

    (void)value;

    return rt_type_text_base_equals(type_bytes, "int8") ||
           rt_type_text_base_equals(type_bytes, "int16") ||
           rt_type_text_base_equals(type_bytes, "int32") ||
           rt_type_text_base_equals(type_bytes, "int64") ||
           rt_type_text_base_equals(type_bytes, "uint8") ||
           rt_type_text_base_equals(type_bytes, "uint16") ||
           rt_type_text_base_equals(type_bytes, "uint32") ||
           rt_type_text_base_equals(type_bytes, "uint64") ||
           rt_type_text_base_equals(type_bytes, "char") ||
           rt_type_text_base_equals(type_bytes, "byte") ||
           rt_type_text_base_equals(type_bytes, "sbyte") ||
           rt_type_text_base_equals(type_bytes, "short") ||
           rt_type_text_base_equals(type_bytes, "int") ||
           rt_type_text_base_equals(type_bytes, "long") ||
           rt_type_text_base_equals(type_bytes, "ulong") ||
           rt_type_text_base_equals(type_bytes, "uint");
}

CalyndaRtWord __calynda_isfloat(CalyndaRtWord value, CalyndaRtWord type_text) {
    const char *type_bytes = rt_require_type_text(type_text);

    (void)value;

    return rt_type_text_base_equals(type_bytes, "float32") ||
           rt_type_text_base_equals(type_bytes, "float64") ||
           rt_type_text_base_equals(type_bytes, "float") ||
           rt_type_text_base_equals(type_bytes, "double");
}

CalyndaRtWord __calynda_isbool(CalyndaRtWord value, CalyndaRtWord type_text) {
    (void)value;
    return rt_type_text_base_equals(rt_require_type_text(type_text), "bool");
}

CalyndaRtWord __calynda_isstring(CalyndaRtWord value, CalyndaRtWord type_text) {
    (void)value;
    return rt_type_text_base_equals(rt_require_type_text(type_text), "string");
}

CalyndaRtWord __calynda_isarray(CalyndaRtWord value, CalyndaRtWord type_text) {
    (void)value;
    return rt_type_text_is_array(rt_require_type_text(type_text));
}

CalyndaRtWord __calynda_issametype(CalyndaRtWord left_value,
                                   CalyndaRtWord left_type_text,
                                   CalyndaRtWord right_value,
                                   CalyndaRtWord right_type_text) {
    (void)left_value;
    (void)right_value;
    return strcmp(rt_require_type_text(left_type_text),
                  rt_require_type_text(right_type_text)) == 0;
}

bool calynda_rt_is_object(CalyndaRtWord word) { return calynda_rt_as_object(word) != NULL; }

const CalyndaRtObjectHeader *calynda_rt_as_object(CalyndaRtWord word) {
    const void *pointer = (const void *)(uintptr_t)word;
    if (word == 0) {
        return NULL;
    }
    if (pointer == &__calynda_pkg_stdlib ||
        pointer == &STDOUT_PRINT_CALLABLE ||
        pointer == &STDIN_INPUT_CALLABLE) {
        return (const CalyndaRtObjectHeader *)pointer;
    }
    if (!registry_contains_pointer(pointer)) {
        return NULL;
    }
    return (const CalyndaRtObjectHeader *)pointer;
}

CalyndaRtWord calynda_rt_make_string_copy(const char *bytes) {
    CalyndaRtString *string_object;
    if (!bytes) {
        bytes = "";
    }
    string_object = rt_new_string_object(bytes, strlen(bytes));
    if (!string_object) {
        rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_OOM,
                  "runtime: out of memory while creating string\n");
    }
    return rt_make_object_word(string_object);
}

void calynda_rt_register_static_object(const CalyndaRtObjectHeader *object) {
    if (!object || object->magic != CALYNDA_RT_OBJECT_MAGIC) {
        return;
    }

    rt_register_static_object_pointer((void *)(uintptr_t)object);
}
