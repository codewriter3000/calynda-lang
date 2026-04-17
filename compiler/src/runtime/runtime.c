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
            type_text[length - 2] == '[' &&
            type_text[length - 1] == ']') ||
           rt_type_text_base_equals(type_text, "arr");
}

CalyndaRtString *rt_new_string_object(const char *bytes, size_t length) {
    CalyndaRtString *string_object;
    char *copied_bytes;
    copied_bytes = copy_text_n(bytes, length);
    if (!copied_bytes) {
        return NULL;
    }
    string_object = calloc(1, sizeof(*string_object));
    if (!string_object) {
        free(copied_bytes);
        return NULL;
    }
    string_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    string_object->header.kind = CALYNDA_RT_OBJECT_STRING;
    string_object->length = length;
    string_object->bytes = copied_bytes;
    if (!rt_register_object_pointer(string_object)) {
        free(copied_bytes);
        free(string_object);
        return NULL;
    }
    return string_object;
}

CalyndaRtArray *rt_new_array_object(size_t element_count, const CalyndaRtWord *elements) {
    CalyndaRtArray *array_object;
    array_object = calloc(1, sizeof(*array_object));
    if (!array_object) {
        return NULL;
    }
    array_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    array_object->header.kind = CALYNDA_RT_OBJECT_ARRAY;
    array_object->count = element_count;
    if (element_count > 0) {
        array_object->elements = calloc(element_count, sizeof(*array_object->elements));
        if (!array_object->elements) {
            free(array_object);
            return NULL;
        }
        memcpy(array_object->elements, elements, element_count * sizeof(*elements));
    }
    if (!rt_register_object_pointer(array_object)) {
        free(array_object->elements);
        free(array_object);
        return NULL;
    }
    return array_object;
}

CalyndaRtClosure *rt_new_closure_object(CalyndaRtClosureEntry code_ptr,
                                        size_t capture_count,
                                        const CalyndaRtWord *captures) {
    CalyndaRtClosure *closure_object;
    closure_object = calloc(1, sizeof(*closure_object));
    if (!closure_object) {
        return NULL;
    }
    closure_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    closure_object->header.kind = CALYNDA_RT_OBJECT_CLOSURE;
    closure_object->entry = code_ptr;
    closure_object->capture_count = capture_count;
    if (capture_count > 0) {
        closure_object->captures = calloc(capture_count, sizeof(*closure_object->captures));
        if (!closure_object->captures) {
            free(closure_object);
            return NULL;
        }
        memcpy(closure_object->captures,
               captures,
               capture_count * sizeof(*closure_object->captures));
    }
    if (!rt_register_object_pointer(closure_object)) {
        free(closure_object->captures);
        free(closure_object);
        return NULL;
    }
    return closure_object;
}

CalyndaRtThread *rt_new_thread_object(void) {
    CalyndaRtThread *thread_object = calloc(1, sizeof(*thread_object));

    if (!thread_object) {
        return NULL;
    }
    thread_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    thread_object->header.kind = CALYNDA_RT_OBJECT_THREAD;
    if (!rt_register_object_pointer(thread_object)) {
        free(thread_object);
        return NULL;
    }
    return thread_object;
}

CalyndaRtMutex *rt_new_mutex_object(void) {
    CalyndaRtMutex *mutex_object = calloc(1, sizeof(*mutex_object));

    if (!mutex_object) {
        return NULL;
    }
    mutex_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    mutex_object->header.kind = CALYNDA_RT_OBJECT_MUTEX;
    if (pthread_mutex_init(&mutex_object->mutex, NULL) != 0) {
        free(mutex_object);
        return NULL;
    }
    if (!rt_register_object_pointer(mutex_object)) {
        pthread_mutex_destroy(&mutex_object->mutex);
        free(mutex_object);
        return NULL;
    }
    return mutex_object;
}

CalyndaRtFuture *rt_new_future_object(CalyndaRtWord callable) {
    CalyndaRtFuture *future_object = calloc(1, sizeof(*future_object));

    if (!future_object) {
        return NULL;
    }
    future_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    future_object->header.kind = CALYNDA_RT_OBJECT_FUTURE;
    future_object->callable = callable;
    if (!rt_register_object_pointer(future_object)) {
        free(future_object);
        return NULL;
    }
    return future_object;
}

CalyndaRtAtomic *rt_new_atomic_object(void) {
    CalyndaRtAtomic *atomic_object = calloc(1, sizeof(*atomic_object));

    if (!atomic_object) {
        return NULL;
    }
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
    if (pointer == &__calynda_pkg_stdlib || pointer == &STDOUT_PRINT_CALLABLE) {
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
