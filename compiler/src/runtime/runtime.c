#include "runtime_internal.h"

#include <stdarg.h>

typedef struct {
    void *pointer;
    bool  owned;
} RuntimeObjectEntry;

typedef struct {
    RuntimeObjectEntry *items;
    size_t              count;
    size_t              capacity;
} RuntimeObjectRegistry;
static RuntimeObjectRegistry OBJECT_REGISTRY;
static _Thread_local RtFailureContext *ACTIVE_FAILURE_CONTEXT = NULL;
static atomic_int PROCESS_FAILURE_CODE = 0;
CalyndaRtExternCallable STDOUT_PRINT_CALLABLE = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE },
    CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT,
    "print"
};

CalyndaRtExternCallable STDIN_INPUT_CALLABLE = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE },
    CALYNDA_RT_EXTERN_CALL_STDIN_INPUT,
    "input"
};

CalyndaRtPackage __calynda_pkg_stdlib = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "stdlib"
};
static bool registry_contains_pointer(const void *pointer);
static bool registry_append_pointer(void *pointer, bool owned);
static void registry_free_owned_object(void *pointer);
static char *copy_text_n(const char *text, size_t length);
static const char *rt_require_type_text(CalyndaRtWord word);
static size_t rt_type_text_base_length(const char *type_text);
static bool rt_type_text_base_equals(const char *type_text, const char *expected);
static bool rt_type_text_is_array(const char *type_text);

bool rt_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;
    if (*capacity >= needed) {
        return true;
    }
    new_capacity = (*capacity == 0) ? 8 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }
    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }
    *items = resized;
    *capacity = new_capacity;
    return true;
}

static bool registry_contains_pointer(const void *pointer) {
    size_t i;
    for (i = 0; i < OBJECT_REGISTRY.count; i++) {
        if (OBJECT_REGISTRY.items[i].pointer == pointer) {
            return true;
        }
    }
    return false;
}

static bool registry_append_pointer(void *pointer, bool owned) {
    if (!pointer || registry_contains_pointer(pointer)) {
        return true;
    }
    if (!rt_reserve_items((void **)&OBJECT_REGISTRY.items,
                          &OBJECT_REGISTRY.capacity,
                          OBJECT_REGISTRY.count + 1,
                          sizeof(*OBJECT_REGISTRY.items))) {
        return false;
    }
    OBJECT_REGISTRY.items[OBJECT_REGISTRY.count].pointer = pointer;
    OBJECT_REGISTRY.items[OBJECT_REGISTRY.count].owned = owned;
    OBJECT_REGISTRY.count++;
    return true;
}
bool rt_register_object_pointer(void *pointer) { return registry_append_pointer(pointer, true); }
bool rt_register_static_object_pointer(void *pointer) {
    return registry_append_pointer(pointer, false);
}

static void registry_free_owned_object(void *pointer) {
    const CalyndaRtObjectHeader *header = (const CalyndaRtObjectHeader *)pointer;
    const CalyndaRtHeteroArray *hetero_array;
    if (!header || header->magic != CALYNDA_RT_OBJECT_MAGIC) {
        return;
    }
    switch ((CalyndaRtObjectKind)header->kind) {
    case CALYNDA_RT_OBJECT_STRING:
        free(((CalyndaRtString *)pointer)->bytes);
        break;
    case CALYNDA_RT_OBJECT_ARRAY:
        free(((CalyndaRtArray *)pointer)->elements);
        break;
    case CALYNDA_RT_OBJECT_CLOSURE:
        free(((CalyndaRtClosure *)pointer)->captures);
        break;
    case CALYNDA_RT_OBJECT_HETERO_ARRAY:
        hetero_array = (const CalyndaRtHeteroArray *)pointer;
        free(((CalyndaRtHeteroArray *)pointer)->elements);
        if (hetero_array->type_desc) {
            free((CalyndaRtTypeTag *)hetero_array->type_desc->generic_param_tags);
            free((CalyndaRtTypeTag *)hetero_array->type_desc->variant_payload_tags);
            free((CalyndaRtTypeDescriptor *)(uintptr_t)hetero_array->type_desc);
        }
        break;
    case CALYNDA_RT_OBJECT_THREAD:
        if (!((CalyndaRtThread *)pointer)->joined) {
            pthread_join(((CalyndaRtThread *)pointer)->thread, NULL);
            ((CalyndaRtThread *)pointer)->joined = true;
        }
        break;
    case CALYNDA_RT_OBJECT_MUTEX:
        pthread_mutex_destroy(&((CalyndaRtMutex *)pointer)->mutex);
        break;
    case CALYNDA_RT_OBJECT_FUTURE:
        if (!((CalyndaRtFuture *)pointer)->joined) {
            pthread_join(((CalyndaRtFuture *)pointer)->thread, NULL);
            ((CalyndaRtFuture *)pointer)->joined = true;
        }
        break;
    case CALYNDA_RT_OBJECT_ATOMIC:
        /* No extra cleanup needed for atomic objects. */
        break;
    case CALYNDA_RT_OBJECT_PACKAGE:
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE:
    case CALYNDA_RT_OBJECT_UNION:
        break;
    }
    free(pointer);
}

void rt_cleanup_registered_objects(void) {
    size_t i;
    for (i = 0; i < OBJECT_REGISTRY.count; i++) {
        if (OBJECT_REGISTRY.items[i].owned) {
            registry_free_owned_object(OBJECT_REGISTRY.items[i].pointer);
        }
    }
    free(OBJECT_REGISTRY.items);
    memset(&OBJECT_REGISTRY, 0, sizeof(OBJECT_REGISTRY));
}
CalyndaRtWord rt_make_object_word(void *pointer) { return (CalyndaRtWord)(uintptr_t)pointer; }

void rt_failure_context_push(RtFailureContext *context) {
    if (!context) {
        return;
    }

    context->exit_code = 0;
    context->previous = ACTIVE_FAILURE_CONTEXT;
    ACTIVE_FAILURE_CONTEXT = context;
}

void rt_failure_context_pop(RtFailureContext *context) {
    if (!context) {
        return;
    }

    ACTIVE_FAILURE_CONTEXT = context->previous;
}

void rt_record_process_failure(int exit_code) {
    int expected = 0;
    int code = exit_code != 0 ? exit_code : CALYNDA_RT_EXIT_RUNTIME_ERROR;

    (void)atomic_compare_exchange_strong(&PROCESS_FAILURE_CODE, &expected, code);
}

int rt_process_failure_code(void) {
    return atomic_load(&PROCESS_FAILURE_CODE);
}

void rt_reset_process_failure(void) {
    atomic_store(&PROCESS_FAILURE_CODE, 0);
}

_Noreturn void rt_fatal_now(int exit_code) {
    int code = exit_code != 0 ? exit_code : CALYNDA_RT_EXIT_RUNTIME_ERROR;

    rt_record_process_failure(code);
    if (ACTIVE_FAILURE_CONTEXT) {
        ACTIVE_FAILURE_CONTEXT->exit_code = code;
        longjmp(ACTIVE_FAILURE_CONTEXT->jump, 1);
    }

    fprintf(stderr, "runtime: fatal error escaped runtime entrypoint\n");
    pthread_exit(NULL);
}

_Noreturn void rt_fatalf(int exit_code, const char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    rt_fatal_now(exit_code);
}

static char *copy_text_n(const char *text, size_t length) {
    char *copy = malloc(length + 1);
    if (!copy) {
        return NULL;
    }
    if (length > 0) {
        memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

/* Produces a short human-readable type+value description of `word` into
 * `buffer`. Truncates safely.  Never returns NULL / never calls fatal. */
void rt_describe_word(CalyndaRtWord word, char *buffer, size_t buffer_size) {
    const CalyndaRtObjectHeader *header;

    if (!buffer || buffer_size == 0) return;

    header = calynda_rt_as_object(word);
    if (!header) {
        /* Plain scalar — display as int64 value */
        snprintf(buffer, buffer_size, "int64 %" PRId64, (int64_t)word);
        return;
    }

    switch ((CalyndaRtObjectKind)header->kind) {
    case CALYNDA_RT_OBJECT_STRING: {
        const CalyndaRtString *s = (const CalyndaRtString *)(const void *)header;
        if (s->length <= 32) {
            snprintf(buffer, buffer_size, "string \"%s\"", s->bytes ? s->bytes : "");
        } else {
            snprintf(buffer, buffer_size, "string (length %zu)", s->length);
        }
        break;
    }
#include "runtime_p2.inc"
