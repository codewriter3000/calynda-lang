#include "asm_emit_internal.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

bool ae_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size) {
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

char *ae_copy_text(const char *text) {
    return ae_copy_text_n(text ? text : "", text ? strlen(text) : 0);
}

char *ae_copy_text_n(const char *text, size_t length) {
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

char *ae_copy_format(const char *format, ...) {
    va_list args;
    va_list args_copy;
    int needed;
    char *buffer;

    va_start(args, format);
    va_copy(args_copy, args);
    needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        va_end(args);
        return NULL;
    }
    buffer = malloc((size_t)needed + 1);
    if (!buffer) {
        va_end(args);
        return NULL;
    }
    vsnprintf(buffer, (size_t)needed + 1, format, args);
    va_end(args);
    return buffer;
}

char *ae_sanitize_symbol(const char *name) {
    size_t i;
    size_t length;
    char *sanitized;

    if (!name) {
        return NULL;
    }
    length = strlen(name);
    sanitized = malloc(length + 1);
    if (!sanitized) {
        return NULL;
    }
    for (i = 0; i < length; i++) {
        sanitized[i] = (char)(isalnum((unsigned char)name[i]) ? name[i] : '_');
    }
    sanitized[length] = '\0';
    return sanitized;
}

bool ae_is_register_name(const char *text) {
    static const char *const registers[] = {
        /* x86_64 */
        "rax", "rbp", "rsp", "rdi", "rsi", "rdx", "rcx",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        /* AArch64 */
        "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
        "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
        "x24", "x25", "x26", "x27", "x28", "x29", "x30", "sp"
    };
    size_t i;

    for (i = 0; i < sizeof(registers) / sizeof(registers[0]); i++) {
        if (strcmp(text, registers[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool ae_starts_with(const char *text, const char *prefix) {
    return text && prefix && strncmp(text, prefix, strlen(prefix)) == 0;
}

bool ae_ends_with(const char *text, const char *suffix) {
    size_t text_length;
    size_t suffix_length;

    if (!text || !suffix) {
        return false;
    }
    text_length = strlen(text);
    suffix_length = strlen(suffix);
    if (suffix_length > text_length) {
        return false;
    }
    return strcmp(text + (text_length - suffix_length), suffix) == 0;
}

char *ae_trim_copy(const char *text) {
    const char *start = text;
    const char *end;

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) {
        end--;
    }
    return ae_copy_text_n(start, (size_t)(end - start));
}

bool ae_split_binary_operands(const char *text, char **left, char **right) {
    const char *comma;
    char *raw_left;

    if (!text || !left || !right) {
        return false;
    }
    comma = strchr(text, ',');
    if (!comma) {
        return false;
    }
    raw_left = ae_copy_text_n(text, (size_t)(comma - text));
    if (!raw_left) {
        return false;
    }
    *left = ae_trim_copy(raw_left);
    free(raw_left);
    if (!*left) {
        return false;
    }
    *right = ae_trim_copy(comma + 1);
    if (!*right) {
        free(*left);
        *left = NULL;
        return false;
    }
    return true;
}

char *ae_between_parens(const char *text, const char *prefix) {
    size_t prefix_length;

    if (!text || !prefix || !ae_starts_with(text, prefix) || !ae_ends_with(text, ")")) {
        return NULL;
    }
    prefix_length = strlen(prefix);
    return ae_copy_text_n(text + prefix_length, strlen(text) - prefix_length - 1);
}

bool ae_decode_quoted_text(const char *quoted, char **decoded, size_t *decoded_length) {
    char *buffer;
    size_t i;
    size_t length = 0;

    if (!quoted || !decoded || !decoded_length) {
        return false;
    }
    if (quoted[0] != '"' || strlen(quoted) < 2 || quoted[strlen(quoted) - 1] != '"') {
        return false;
    }

    buffer = malloc(strlen(quoted));
    if (!buffer) {
        return false;
    }
    for (i = 1; i + 1 < strlen(quoted); i++) {
        if (quoted[i] == '\\' && i + 2 < strlen(quoted)) {
            i++;
            switch (quoted[i]) {
            case 'n':
                buffer[length++] = '\n';
                break;
            case 'r':
                buffer[length++] = '\r';
                break;
            case 't':
                buffer[length++] = '\t';
                break;
            case '\\':
                buffer[length++] = '\\';
                break;
            case '"':
                buffer[length++] = '"';
                break;
            case '0':
                buffer[length++] = '\0';
                break;
            default:
                buffer[length++] = quoted[i];
                break;
            }
        } else {
            buffer[length++] = quoted[i];
        }
    }
    buffer[length] = '\0';
    *decoded = buffer;
    *decoded_length = length;
    return true;
}
