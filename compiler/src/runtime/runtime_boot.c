#include "runtime.h"

#include <stddef.h>
#include <stdint.h>

#define CALYNDA_RT_BOOT_HEAP_CAPACITY (64u * 1024u)

static unsigned char BOOT_HEAP[CALYNDA_RT_BOOT_HEAP_CAPACITY];
static size_t BOOT_HEAP_USED = 0;

static _Noreturn void frt_fail(void) {
    __builtin_trap();
    for (;;) {
    }
}

static size_t frt_align_up(size_t value, size_t alignment) {
    size_t mask = alignment - 1;

    return (value + mask) & ~mask;
}

static void *frt_alloc_bytes(size_t size) {
    size_t aligned_size = frt_align_up(size == 0 ? 1 : size, sizeof(CalyndaRtWord));
    size_t offset = frt_align_up(BOOT_HEAP_USED, sizeof(CalyndaRtWord));
    size_t i;
    unsigned char *memory;

    if (offset > sizeof(BOOT_HEAP) || aligned_size > sizeof(BOOT_HEAP) - offset) {
        frt_fail();
    }

    memory = &BOOT_HEAP[offset];
    BOOT_HEAP_USED = offset + aligned_size;
    for (i = 0; i < aligned_size; i++) {
        memory[i] = 0;
    }
    return memory;
}

static CalyndaRtWord frt_word_from_signed(long long value) {
    return (CalyndaRtWord)(int64_t)value;
}

static long long frt_signed_from_word(CalyndaRtWord value) {
    return (long long)(int64_t)value;
}

static const CalyndaRtObjectHeader *frt_as_object(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *header = (const CalyndaRtObjectHeader *)(uintptr_t)word;

    if (!header || header->magic != CALYNDA_RT_OBJECT_MAGIC) {
        return NULL;
    }
    return header;
}

static const CalyndaRtArray *frt_require_array(CalyndaRtWord target) {
    const CalyndaRtObjectHeader *header = frt_as_object(target);

    if (!header || header->kind != CALYNDA_RT_OBJECT_ARRAY) {
        frt_fail();
    }
    return (const CalyndaRtArray *)(const void *)header;
}

static CalyndaRtArray *frt_require_mut_array(CalyndaRtWord target) {
    return (CalyndaRtArray *)(void *)frt_require_array(target);
}

static const CalyndaRtString *frt_require_string(CalyndaRtWord target) {
    const CalyndaRtObjectHeader *header = frt_as_object(target);

    if (!header || header->kind != CALYNDA_RT_OBJECT_STRING) {
        frt_fail();
    }
    return (const CalyndaRtString *)(const void *)header;
}

static char *frt_copy_bytes(const char *source, size_t length) {
    char *bytes = (char *)frt_alloc_bytes(length + 1);
    size_t i;

    for (i = 0; i < length; i++) {
        bytes[i] = source[i];
    }
    bytes[length] = '\0';
    return bytes;
}

static CalyndaRtWord *frt_copy_words(const CalyndaRtWord *source, size_t count) {
    CalyndaRtWord *copy;
    size_t i;

    if (count == 0) {
        return NULL;
    }

    copy = (CalyndaRtWord *)frt_alloc_bytes(count * sizeof(*copy));
    for (i = 0; i < count; i++) {
        copy[i] = source[i];
    }
    return copy;
}

static CalyndaRtArray *frt_new_array(size_t count, const CalyndaRtWord *elements) {
    CalyndaRtArray *array = (CalyndaRtArray *)frt_alloc_bytes(sizeof(*array));

    array->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    array->header.kind = CALYNDA_RT_OBJECT_ARRAY;
    array->count = count;
    array->elements = frt_copy_words(elements, count);
    return array;
}

static CalyndaRtString *frt_new_string(const char *bytes, size_t length) {
    CalyndaRtString *string = (CalyndaRtString *)frt_alloc_bytes(sizeof(*string));

    string->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    string->header.kind = CALYNDA_RT_OBJECT_STRING;
    string->length = length;
    string->bytes = frt_copy_bytes(bytes, length);
    return string;
}

CalyndaRtWord __calynda_rt_member_load(CalyndaRtWord target, const char *member) {
    const CalyndaRtObjectHeader *header = frt_as_object(target);

    if (!header || !member || member[0] != 'l' || member[1] != 'e' || member[2] != 'n' ||
        member[3] != 'g' || member[4] != 't' || member[5] != 'h' || member[6] != '\0') {
        frt_fail();
    }

    if (header->kind == CALYNDA_RT_OBJECT_STRING) {
        return frt_word_from_signed((long long)((const CalyndaRtString *)(const void *)header)->length);
    }
    if (header->kind == CALYNDA_RT_OBJECT_ARRAY) {
        return frt_word_from_signed((long long)((const CalyndaRtArray *)(const void *)header)->count);
    }

    frt_fail();
}

CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader *header = frt_as_object(target);
    long long signed_index = frt_signed_from_word(index);
    size_t offset;

    if (!header || signed_index < 0) {
        frt_fail();
    }

    offset = (size_t)signed_index;
    if (header->kind == CALYNDA_RT_OBJECT_STRING) {
        const CalyndaRtString *string = (const CalyndaRtString *)(const void *)header;

        if (offset >= string->length) {
            frt_fail();
        }
        return frt_word_from_signed((long long)(unsigned char)string->bytes[offset]);
    }

    if (header->kind == CALYNDA_RT_OBJECT_ARRAY) {
        const CalyndaRtArray *array = (const CalyndaRtArray *)(const void *)header;

        if (offset >= array->count) {
            frt_fail();
        }
        return array->elements[offset];
    }

    frt_fail();
}

CalyndaRtWord __calynda_rt_array_literal(size_t element_count,
                                         const CalyndaRtWord *elements) {
    return (CalyndaRtWord)(uintptr_t)frt_new_array(element_count, elements);
}

CalyndaRtWord __calynda_rt_array_car(CalyndaRtWord target) {
    const CalyndaRtArray *array = frt_require_array(target);

    if (array->count == 0) {
        frt_fail();
    }
    return array->elements[0];
}

CalyndaRtWord __calynda_rt_array_cdr(CalyndaRtWord target) {
    const CalyndaRtArray *array = frt_require_array(target);

    if (array->count == 0) {
        frt_fail();
    }
    return (CalyndaRtWord)(uintptr_t)frt_new_array(array->count - 1,
                                                    array->count > 1 ? array->elements + 1 : NULL);
}

CalyndaRtWord __calynda_rt_string_cdr(CalyndaRtWord target) {
    const CalyndaRtString *string = frt_require_string(target);

    if (string->length == 0) {
        frt_fail();
    }
    return (CalyndaRtWord)(uintptr_t)frt_new_string(string->bytes + 1, string->length - 1);
}

void __calynda_rt_store_index(CalyndaRtWord target,
                              CalyndaRtWord index,
                              CalyndaRtWord value) {
    CalyndaRtArray *array = frt_require_mut_array(target);
    long long signed_index = frt_signed_from_word(index);
    size_t offset;

    if (signed_index < 0) {
        frt_fail();
    }

    offset = (size_t)signed_index;
    if (offset >= array->count) {
        frt_fail();
    }
    array->elements[offset] = value;
}