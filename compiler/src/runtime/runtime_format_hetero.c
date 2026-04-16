#include "runtime_internal.h"

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

static bool rt_format_tagged_word(CalyndaRtWord word,
                                  CalyndaRtTypeTag tag,
                                  char *buffer,
                                  size_t buffer_size) {
    switch (tag) {
    case CALYNDA_RT_TYPE_VOID:
        return snprintf(buffer, buffer_size, "void") >= 0;
    case CALYNDA_RT_TYPE_BOOL:
        return snprintf(buffer, buffer_size, "%s", word != 0 ? "true" : "false") >= 0;
    case CALYNDA_RT_TYPE_INT32:
    case CALYNDA_RT_TYPE_INT64:
        return snprintf(buffer, buffer_size, "%lld", rt_signed_from_word(word)) >= 0;
    case CALYNDA_RT_TYPE_STRING:
    case CALYNDA_RT_TYPE_ARRAY:
    case CALYNDA_RT_TYPE_CLOSURE:
    case CALYNDA_RT_TYPE_EXTERNAL:
    case CALYNDA_RT_TYPE_RAW_WORD:
    case CALYNDA_RT_TYPE_UNION:
    case CALYNDA_RT_TYPE_HETERO_ARRAY:
        return rt_format_word_internal(word, buffer, buffer_size);
    }
    return false;
}

static bool append_tagged_word_text(char **buffer,
                                    size_t *length,
                                    size_t *capacity,
                                    CalyndaRtWord word,
                                    CalyndaRtTypeTag tag) {
    char word_text[256];

    if (!rt_format_tagged_word(word, tag, word_text, sizeof(word_text))) {
        return false;
    }
    return append_text(buffer, length, capacity, word_text);
}

bool rt_format_hetero_array_text(const CalyndaRtHeteroArray *array,
                                 char *buffer,
                                 size_t buffer_size) {
    char *formatted = NULL;
    size_t length = 0, capacity = 0, i;
    bool ok = false;

    if (!array || !append_text(&formatted, &length, &capacity, "[")) {
        free(formatted);
        return false;
    }
    for (i = 0; i < array->count; i++) {
        CalyndaRtTypeTag tag = (array->type_desc && array->type_desc->variant_payload_tags &&
                                i < array->type_desc->variant_count)
            ? array->type_desc->variant_payload_tags[i] : CALYNDA_RT_TYPE_RAW_WORD;

        if ((i > 0 && !append_text(&formatted, &length, &capacity, ", ")) ||
            !append_tagged_word_text(&formatted, &length, &capacity, array->elements[i], tag)) {
            free(formatted);
            return false;
        }
    }
    if (append_text(&formatted, &length, &capacity, "]")) {
        ok = snprintf(buffer, buffer_size, "%s", formatted) >= 0;
    }
    free(formatted);
    return ok;
}