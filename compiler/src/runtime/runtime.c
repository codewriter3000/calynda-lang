#include "runtime_internal.h"

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
        fprintf(stderr, "runtime: out of memory while creating string\n");
        abort();
    }
    return rt_make_object_word(string_object);
}

void calynda_rt_register_static_object(const CalyndaRtObjectHeader *object) {
    if (!object || object->magic != CALYNDA_RT_OBJECT_MAGIC) {
        return;
    }

    rt_register_static_object_pointer((void *)(uintptr_t)object);
}