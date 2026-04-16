#include "runtime_internal.h"

CalyndaRtWord rt_word_from_signed(long long value) {
    return (CalyndaRtWord)(int64_t)value;
}

long long rt_signed_from_word(CalyndaRtWord value) {
    return (long long)(int64_t)value;
}

bool rt_is_runtime_string_word(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(word);

    return header && header->kind == CALYNDA_RT_OBJECT_STRING;
}

bool rt_format_word_internal(CalyndaRtWord word, char *buffer, size_t buffer_size) {
    const CalyndaRtObjectHeader *header;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    header = calynda_rt_as_object(word);
    if (!header) {
        return snprintf(buffer, buffer_size, "%lld", rt_signed_from_word(word)) >= 0;
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
    case CALYNDA_RT_OBJECT_UNION: {
        const CalyndaRtUnion *u = (const CalyndaRtUnion *)(const void *)header;
        const char *variant_name = "?";
        if (u->type_desc && u->tag < u->type_desc->variant_count) {
            variant_name = u->type_desc->variant_names[u->tag];
        }
        return snprintf(buffer, buffer_size, "<%s::%s>",
                        u->type_desc ? u->type_desc->name : "union",
                        variant_name) >= 0;
    }
    case CALYNDA_RT_OBJECT_HETERO_ARRAY:
        return rt_format_hetero_array_text((const CalyndaRtHeteroArray *)(const void *)header,
                                           buffer,
                                           buffer_size);
    case CALYNDA_RT_OBJECT_THREAD:
        return snprintf(buffer, buffer_size, "<thread>") >= 0;
    case CALYNDA_RT_OBJECT_MUTEX:
        return snprintf(buffer, buffer_size, "<mutex>") >= 0;
    }

    return false;
}

bool calynda_rt_format_word(CalyndaRtWord word, char *buffer, size_t buffer_size) {
    return rt_format_word_internal(word, buffer, buffer_size);
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

static bool append_text(char **buffer, size_t *length, size_t *capacity, const char *text) {
    size_t text_length;

    if (!buffer || !length || !capacity) {
        return false;
    }

    text = text ? text : "";
    text_length = strlen(text);
    if (!rt_reserve_items((void **)buffer,
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

    if (!rt_format_word_internal(word, word_text, sizeof(word_text))) {
        return false;
    }
    return append_text(buffer, length, capacity, word_text);
}

static const char *extern_callable_name(CalyndaRtExternCallableKind kind) {
    switch (kind) {
    case CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT:
        return "print";
    }

    return "unknown";
}

CalyndaRtWord rt_dispatch_extern_callable(const CalyndaRtExternCallable *callable,
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
            if (!rt_format_word_internal(arguments[0], buffer, sizeof(buffer))) {
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
        CalyndaRtString *string_object = rt_new_string_object(buffer, length);

        free(buffer);
        if (!string_object) {
            fprintf(stderr, "runtime: out of memory while boxing template result\n");
            abort();
        }
        return rt_make_object_word(string_object);
    }
}
