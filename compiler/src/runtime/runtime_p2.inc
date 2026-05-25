    case CALYNDA_RT_OBJECT_ARRAY:
        snprintf(buffer, buffer_size, "array[%zu]",
                 ((const CalyndaRtArray *)(const void *)header)->count);
        break;
    case CALYNDA_RT_OBJECT_CLOSURE:
        snprintf(buffer, buffer_size, "closure");
        break;
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE:
        snprintf(buffer, buffer_size, "extern-callable \"%s\"",
                 ((const CalyndaRtExternCallable *)(const void *)header)->name);
        break;
    case CALYNDA_RT_OBJECT_PACKAGE:
        snprintf(buffer, buffer_size, "package \"%s\"",
                 ((const CalyndaRtPackage *)(const void *)header)->name);
        break;
    case CALYNDA_RT_OBJECT_UNION: {
        const CalyndaRtUnion *u = (const CalyndaRtUnion *)(const void *)header;
        const char *variant = "?";
        if (u->type_desc && u->tag < u->type_desc->variant_count) {
            variant = u->type_desc->variant_names[u->tag];
        }
        snprintf(buffer, buffer_size, "union %s::%s",
                 u->type_desc ? u->type_desc->name : "?", variant);
        break;
    }
    case CALYNDA_RT_OBJECT_HETERO_ARRAY:
        snprintf(buffer, buffer_size, "hetero-array[%zu]",
                 ((const CalyndaRtHeteroArray *)(const void *)header)->count);
        break;
    case CALYNDA_RT_OBJECT_THREAD:
        snprintf(buffer, buffer_size, "thread");
        break;
    case CALYNDA_RT_OBJECT_MUTEX:
        snprintf(buffer, buffer_size, "mutex");
        break;
    case CALYNDA_RT_OBJECT_FUTURE:
        snprintf(buffer, buffer_size, "future");
        break;
    case CALYNDA_RT_OBJECT_ATOMIC:
        snprintf(buffer, buffer_size, "atomic");
        break;
    default:
        snprintf(buffer, buffer_size, "unknown-object");
        break;
    }
}

static const char *rt_require_type_text(CalyndaRtWord word) {
    if (!rt_is_runtime_string_word(word)) {
        fprintf(stderr, "runtime: type helper expects a string metadata argument\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return calynda_rt_string_bytes(word);
}

static size_t rt_type_text_base_length(const char *type_text) {
    size_t length;
    size_t i;

    if (!type_text) {
        return 0;
    }

    length = strlen(type_text);
    while (length >= 2 &&
           type_text[length - 2] == '[' &&
           type_text[length - 1] == ']') {
        length -= 2;
    }

    for (i = 0; i < length; i++) {
        if (type_text[i] == '<') {
            return i;
        }
    }

    return length;
}

static bool rt_type_text_base_equals(const char *type_text, const char *expected) {
    size_t base_length;

    if (!type_text || !expected) {
        return false;
    }

    base_length = rt_type_text_base_length(type_text);
    return strlen(expected) == base_length &&
           strncmp(type_text, expected, base_length) == 0;
}

static bool rt_type_text_is_array(const char *type_text) {
    size_t length;

    if (!type_text) {
        return false;
    }

    length = strlen(type_text);
        return (length >= 2 &&
            type_text[length - 1] == ']' &&
            strchr(type_text, '[') != NULL) ||
           rt_type_text_base_equals(type_text, "arr");
}

CalyndaRtString *rt_new_string_object(const char *bytes, size_t length) {
    CalyndaRtString *string_object;
    char *copied_bytes;
    copied_bytes = copy_text_n(bytes, length);
    if (!copied_bytes) {
        return NULL;
    }
    string_object = rt_alloc_managed(sizeof(*string_object));
    string_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    string_object->header.kind = CALYNDA_RT_OBJECT_STRING;
    string_object->length = length;
    string_object->bytes = copied_bytes;
    return string_object;
}

CalyndaRtArray *rt_new_array_object(size_t element_count, const CalyndaRtWord *elements) {
    CalyndaRtArray *array_object;
    array_object = rt_alloc_managed(sizeof(*array_object));
    array_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    array_object->header.kind = CALYNDA_RT_OBJECT_ARRAY;
    array_object->count = element_count;
    if (element_count > 0) {
        array_object->elements = calloc(element_count, sizeof(*array_object->elements));
        if (!array_object->elements) {
            rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_OOM,
                      "runtime: out of memory allocating array elements\n");
        }
        memcpy(array_object->elements, elements, element_count * sizeof(*elements));
    }
    return array_object;
}

CalyndaRtClosure *rt_new_closure_object(CalyndaRtClosureEntry code_ptr,
                                        size_t capture_count,
                                        const CalyndaRtWord *captures) {
    CalyndaRtClosure *closure_object;
    closure_object = rt_alloc_managed(sizeof(*closure_object));
    closure_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    closure_object->header.kind = CALYNDA_RT_OBJECT_CLOSURE;
    closure_object->entry = code_ptr;
    closure_object->capture_count = capture_count;
    if (capture_count > 0) {
        closure_object->captures = calloc(capture_count, sizeof(*closure_object->captures));
        if (!closure_object->captures) {
            rt_fatalf(CALYNDA_RT_EXIT_RUNTIME_OOM,
                      "runtime: out of memory allocating closure captures\n");
        }
        memcpy(closure_object->captures,
               captures,
               capture_count * sizeof(*closure_object->captures));
    }
    return closure_object;
}

CalyndaRtThread *rt_new_thread_object(void) {
    CalyndaRtThread *thread_object = rt_alloc_managed(sizeof(*thread_object));
    thread_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    thread_object->header.kind = CALYNDA_RT_OBJECT_THREAD;
    return thread_object;
}

CalyndaRtMutex *rt_new_mutex_object(void) {
    CalyndaRtMutex *mutex_object = rt_alloc_managed(sizeof(*mutex_object));
    mutex_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    mutex_object->header.kind = CALYNDA_RT_OBJECT_MUTEX;
    if (pthread_mutex_init(&mutex_object->mutex, NULL) != 0) {
        fprintf(stderr, "runtime: failed to initialise mutex\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }
    return mutex_object;
}

CalyndaRtFuture *rt_new_future_object(CalyndaRtWord callable) {
    CalyndaRtFuture *future_object = rt_alloc_managed(sizeof(*future_object));
    future_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    future_object->header.kind = CALYNDA_RT_OBJECT_FUTURE;
    future_object->callable = callable;
    return future_object;
}

CalyndaRtAtomic *rt_new_atomic_object(void) {
    CalyndaRtAtomic *atomic_object = rt_alloc_managed(sizeof(*atomic_object));
#include "runtime_p3.inc"
