#include "runtime.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void  **items;
    size_t  count;
    size_t  capacity;
} RuntimeObjectRegistry;

static RuntimeObjectRegistry OBJECT_REGISTRY;

static CalyndaRtExternCallable STDOUT_PRINT_CALLABLE = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE },
    CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT,
    "print"
};

CalyndaRtPackage __calynda_pkg_stdlib = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "stdlib"
};

static bool reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size);
static bool registry_contains_pointer(const void *pointer);
static bool register_object_pointer(void *pointer);
static CalyndaRtWord make_object_word(void *pointer);
static CalyndaRtString *new_string_object(const char *bytes, size_t length);
static CalyndaRtArray *new_array_object(size_t element_count, const CalyndaRtWord *elements);
static CalyndaRtClosure *new_closure_object(CalyndaRtClosureEntry code_ptr,
                                            size_t capture_count,
                                            const CalyndaRtWord *captures);
static char *copy_text_n(const char *text, size_t length);
static bool append_text(char **buffer, size_t *length, size_t *capacity, const char *text);
static bool append_word_text(char **buffer,
                             size_t *length,
                             size_t *capacity,
                             CalyndaRtWord word);
static bool is_runtime_string_word(CalyndaRtWord word);
static bool format_word_internal(CalyndaRtWord word, char *buffer, size_t buffer_size);
static CalyndaRtWord word_from_signed(long long value);
static long long signed_from_word(CalyndaRtWord value);
static const char *extern_callable_name(CalyndaRtExternCallableKind kind);
static CalyndaRtWord dispatch_extern_callable(const CalyndaRtExternCallable *callable,
                                              size_t argument_count,
                                              const CalyndaRtWord *arguments);

bool calynda_rt_is_object(CalyndaRtWord word) {
    return calynda_rt_as_object(word) != NULL;
}

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

const char *calynda_rt_object_kind_name(CalyndaRtObjectKind kind) {
    switch (kind) {
    case CALYNDA_RT_OBJECT_STRING:
        return "string";
    case CALYNDA_RT_OBJECT_ARRAY:
        return "array";
    case CALYNDA_RT_OBJECT_CLOSURE:
        return "closure";
    case CALYNDA_RT_OBJECT_PACKAGE:
        return "package";
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE:
        return "extern-callable";
    }

    return "unknown";
}

const char *calynda_rt_type_tag_name(CalyndaRtTypeTag tag) {
    switch (tag) {
    case CALYNDA_RT_TYPE_VOID:
        return "void";
    case CALYNDA_RT_TYPE_BOOL:
        return "bool";
    case CALYNDA_RT_TYPE_INT32:
        return "int32";
    case CALYNDA_RT_TYPE_INT64:
        return "int64";
    case CALYNDA_RT_TYPE_STRING:
        return "string";
    case CALYNDA_RT_TYPE_ARRAY:
        return "array";
    case CALYNDA_RT_TYPE_CLOSURE:
        return "closure";
    case CALYNDA_RT_TYPE_EXTERNAL:
        return "external";
    case CALYNDA_RT_TYPE_RAW_WORD:
        return "raw-word";
    }

    return "unknown";
}

bool calynda_rt_dump_layout(FILE *out) {
    if (!out) {
        return false;
    }

    fprintf(out,
            "RuntimeLayout word=uint64 raw-scalar-or-object-handle\n"
            "  ObjectHeader size=%zu magic=0x%08X fields=[magic:uint32, kind:uint32]\n"
            "  String size=%zu payload=[length:size_t, bytes:char*]\n"
            "  Array size=%zu payload=[count:size_t, elements:uint64*]\n"
            "  Closure size=%zu payload=[entry:void*, capture_count:size_t, captures:uint64*]\n"
            "  Package size=%zu payload=[name:char*]\n"
            "  ExternCallable size=%zu payload=[kind:uint32, name:char*]\n"
            "  TemplatePart size=%zu payload=[tag:uint64, payload:uint64]\n"
            "  TemplateTags text=%d value=%d\n"
            "  Builtins package=stdlib member=print\n",
            sizeof(CalyndaRtWord),
            CALYNDA_RT_OBJECT_MAGIC,
            sizeof(CalyndaRtString),
            sizeof(CalyndaRtArray),
            sizeof(CalyndaRtClosure),
            sizeof(CalyndaRtPackage),
            sizeof(CalyndaRtExternCallable),
            sizeof(CalyndaRtTemplatePart),
            CALYNDA_RT_TEMPLATE_TAG_TEXT,
            CALYNDA_RT_TEMPLATE_TAG_VALUE);
    return !ferror(out);
}

char *calynda_rt_dump_layout_to_string(void) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!calynda_rt_dump_layout(stream) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

bool calynda_rt_format_word(CalyndaRtWord word, char *buffer, size_t buffer_size) {
    return format_word_internal(word, buffer, buffer_size);
}

const char *calynda_rt_string_bytes(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(word);

    if (!header || header->kind != CALYNDA_RT_OBJECT_STRING) {
        return NULL;
    }

    return ((const CalyndaRtString *)(const void *)header)->bytes;
}

size_t calynda_rt_array_length(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(word);

    if (!header || header->kind != CALYNDA_RT_OBJECT_ARRAY) {
        return 0;
    }

    return ((const CalyndaRtArray *)(const void *)header)->count;
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
    return (int)(int32_t)result;
}

CalyndaRtWord calynda_rt_make_string_copy(const char *bytes) {
    CalyndaRtString *string_object;

    if (!bytes) {
        bytes = "";
    }

    string_object = new_string_object(bytes, strlen(bytes));
    if (!string_object) {
        fprintf(stderr, "runtime: out of memory while creating string\n");
        abort();
    }

    return make_object_word(string_object);
}

void calynda_rt_register_static_object(const CalyndaRtObjectHeader *object) {
    if (!object || object->magic != CALYNDA_RT_OBJECT_MAGIC) {
        return;
    }

    register_object_pointer((void *)(uintptr_t)object);
}

CalyndaRtWord __calynda_rt_closure_new(CalyndaRtClosureEntry code_ptr,
                                       size_t capture_count,
                                       const CalyndaRtWord *captures) {
    CalyndaRtClosure *closure = new_closure_object(code_ptr, capture_count, captures);

    if (!closure) {
        fprintf(stderr, "runtime: out of memory while creating closure\n");
        abort();
    }

    return make_object_word(closure);
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
        return dispatch_extern_callable((const CalyndaRtExternCallable *)(const void *)header,
                                        argument_count,
                                        arguments);
    default:
        fprintf(stderr,
                "runtime: object of kind %s is not callable\n",
                calynda_rt_object_kind_name((CalyndaRtObjectKind)header->kind));
        abort();
    }
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
        return make_object_word(&STDOUT_PRINT_CALLABLE);
    }

    fprintf(stderr,
            "runtime: unsupported member load %s.%s\n",
            header == &__calynda_pkg_stdlib.header ? __calynda_pkg_stdlib.name : calynda_rt_object_kind_name((CalyndaRtObjectKind)header->kind),
            member);
    abort();
}

CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);
    size_t offset;

    if (!header || header->kind != CALYNDA_RT_OBJECT_ARRAY) {
        fprintf(stderr, "runtime: attempted index load on non-array value\n");
        abort();
    }

    offset = (size_t)signed_from_word(index);
    if (offset >= ((const CalyndaRtArray *)(const void *)header)->count) {
        fprintf(stderr, "runtime: array index out of bounds (%zu)\n", offset);
        abort();
    }

    return ((const CalyndaRtArray *)(const void *)header)->elements[offset];
}

CalyndaRtWord __calynda_rt_array_literal(size_t element_count,
                                         const CalyndaRtWord *elements) {
    CalyndaRtArray *array = new_array_object(element_count, elements);

    if (!array) {
        fprintf(stderr, "runtime: out of memory while creating array\n");
        abort();
    }

    return make_object_word(array);
}

CalyndaRtWord __calynda_rt_template_build(size_t part_count,
                                          const CalyndaRtTemplatePart *parts) {
    char *buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    size_t i;

    if (part_count > 0 && !parts) {
        fprintf(stderr, "runtime: template parts were missing\n");
        abort();
    }

    for (i = 0; i < part_count; i++) {
        if (parts[i].tag == CALYNDA_RT_TEMPLATE_TAG_TEXT) {
            if (!append_text(&buffer, &length, &capacity, (const char *)(uintptr_t)parts[i].payload)) {
                fprintf(stderr, "runtime: out of memory while building template text\n");
                free(buffer);
                abort();
            }
        } else if (parts[i].tag == CALYNDA_RT_TEMPLATE_TAG_VALUE) {
            if (!append_word_text(&buffer, &length, &capacity, parts[i].payload)) {
                fprintf(stderr, "runtime: out of memory while formatting template value\n");
                free(buffer);
                abort();
            }
        } else {
            fprintf(stderr, "runtime: unknown template tag (%" PRIu64 ")\n", parts[i].tag);
            free(buffer);
            abort();
        }
    }

    if (!buffer) {
        return calynda_rt_make_string_copy("");
    }

    {
        CalyndaRtString *string_object = new_string_object(buffer, length);

        free(buffer);
        if (!string_object) {
            fprintf(stderr, "runtime: out of memory while boxing template result\n");
            abort();
        }
        return make_object_word(string_object);
    }
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

    offset = (size_t)signed_from_word(index);
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

    if (!format_word_internal(value, buffer, sizeof(buffer))) {
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
            return word_from_signed(strtoll(((const CalyndaRtString *)(const void *)header)->bytes,
                                            NULL,
                                            10));
        }
        return source;
    }

    case CALYNDA_RT_TYPE_STRING: {
        char buffer[128];

        if (is_runtime_string_word(source)) {
            return source;
        }
        if (!format_word_internal(source, buffer, sizeof(buffer))) {
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
        return source;
    }

    return source;
}

static bool reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size) {
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
        if (OBJECT_REGISTRY.items[i] == pointer) {
            return true;
        }
    }

    return false;
}

static bool register_object_pointer(void *pointer) {
    if (!pointer || registry_contains_pointer(pointer)) {
        return true;
    }

    if (!reserve_items((void **)&OBJECT_REGISTRY.items,
                       &OBJECT_REGISTRY.capacity,
                       OBJECT_REGISTRY.count + 1,
                       sizeof(*OBJECT_REGISTRY.items))) {
        return false;
    }

    OBJECT_REGISTRY.items[OBJECT_REGISTRY.count] = pointer;
    OBJECT_REGISTRY.count++;
    return true;
}

static CalyndaRtWord make_object_word(void *pointer) {
    return (CalyndaRtWord)(uintptr_t)pointer;
}

static CalyndaRtString *new_string_object(const char *bytes, size_t length) {
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
    if (!register_object_pointer(string_object)) {
        free(copied_bytes);
        free(string_object);
        return NULL;
    }

    return string_object;
}

static CalyndaRtArray *new_array_object(size_t element_count, const CalyndaRtWord *elements) {
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
    if (!register_object_pointer(array_object)) {
        free(array_object->elements);
        free(array_object);
        return NULL;
    }

    return array_object;
}

static CalyndaRtClosure *new_closure_object(CalyndaRtClosureEntry code_ptr,
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
    if (!register_object_pointer(closure_object)) {
        free(closure_object->captures);
        free(closure_object);
        return NULL;
    }

    return closure_object;
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

static bool append_text(char **buffer, size_t *length, size_t *capacity, const char *text) {
    size_t text_length;

    if (!buffer || !length || !capacity) {
        return false;
    }

    text = text ? text : "";
    text_length = strlen(text);
    if (!reserve_items((void **)buffer,
                       capacity,
                       *length + text_length + 1,
                       sizeof(**buffer))) {
        return false;
    }
    memcpy(*buffer + *length, text, text_length);
    *length += text_length;
    (*buffer)[*length] = '\0';
    return true;
}

static bool append_word_text(char **buffer,
                             size_t *length,
                             size_t *capacity,
                             CalyndaRtWord word) {
    char word_text[256];

    if (!format_word_internal(word, word_text, sizeof(word_text))) {
        return false;
    }
    return append_text(buffer, length, capacity, word_text);
}

static bool is_runtime_string_word(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(word);

    return header && header->kind == CALYNDA_RT_OBJECT_STRING;
}

static bool format_word_internal(CalyndaRtWord word, char *buffer, size_t buffer_size) {
    const CalyndaRtObjectHeader *header;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    header = calynda_rt_as_object(word);
    if (!header) {
        return snprintf(buffer, buffer_size, "%lld", signed_from_word(word)) >= 0;
    }

    switch ((CalyndaRtObjectKind)header->kind) {
    case CALYNDA_RT_OBJECT_STRING:
        return snprintf(buffer,
                        buffer_size,
                        "%s",
                        ((const CalyndaRtString *)(const void *)header)->bytes) >= 0;
    case CALYNDA_RT_OBJECT_ARRAY:
        return snprintf(buffer,
                        buffer_size,
                        "<array:%zu>",
                        ((const CalyndaRtArray *)(const void *)header)->count) >= 0;
    case CALYNDA_RT_OBJECT_CLOSURE:
        return snprintf(buffer, buffer_size, "<closure>") >= 0;
    case CALYNDA_RT_OBJECT_PACKAGE:
        return snprintf(buffer,
                        buffer_size,
                        "<package:%s>",
                        ((const CalyndaRtPackage *)(const void *)header)->name) >= 0;
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE:
        return snprintf(buffer,
                        buffer_size,
                        "<extern:%s>",
                        ((const CalyndaRtExternCallable *)(const void *)header)->name) >= 0;
    }

    return false;
}

static CalyndaRtWord word_from_signed(long long value) {
    return (CalyndaRtWord)(int64_t)value;
}

static long long signed_from_word(CalyndaRtWord value) {
    return (long long)(int64_t)value;
}

static const char *extern_callable_name(CalyndaRtExternCallableKind kind) {
    switch (kind) {
    case CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT:
        return "print";
    }

    return "unknown";
}

static CalyndaRtWord dispatch_extern_callable(const CalyndaRtExternCallable *callable,
                                              size_t argument_count,
                                              const CalyndaRtWord *arguments) {
    char buffer[256];

    if (!callable) {
        fprintf(stderr, "runtime: missing extern callable dispatch target\n");
        abort();
    }

    switch (callable->kind) {
    case CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT:
        if (argument_count > 0) {
            if (!format_word_internal(arguments[0], buffer, sizeof(buffer))) {
                strcpy(buffer, "<unformattable>");
            }
            printf("%s\n", buffer);
        } else {
            printf("\n");
        }
        return 0;
    }

    fprintf(stderr,
            "runtime: unsupported extern callable %s\n",
            extern_callable_name(callable->kind));
    abort();
}