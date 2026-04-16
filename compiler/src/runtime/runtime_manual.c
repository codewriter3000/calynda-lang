#include "runtime.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

CalyndaRtWord __calynda_deref(CalyndaRtWord ptr) {
    CalyndaRtWord result;

    memcpy(&result, (const void *)(uintptr_t)ptr, sizeof(result));
    return result;
}

CalyndaRtWord __calynda_deref_sized(CalyndaRtWord ptr, CalyndaRtWord size) {
    CalyndaRtWord result = 0;

    memcpy(&result, (const void *)(uintptr_t)ptr, (size_t)size);
    return result;
}

CalyndaRtWord __calynda_addr(CalyndaRtWord value) {
    /* addr() returns the raw value unchanged; in Calynda's manual model,
       malloc already returns a raw address as an int64 word. */
    return value;
}

CalyndaRtWord __calynda_offset(CalyndaRtWord ptr, CalyndaRtWord count) {
    return ptr + count * sizeof(CalyndaRtWord);
}

CalyndaRtWord __calynda_offset_stride(CalyndaRtWord ptr,
                                      CalyndaRtWord count,
                                      CalyndaRtWord stride) {
    return ptr + count * stride;
}

void __calynda_store(CalyndaRtWord ptr, CalyndaRtWord value) {
    memcpy((void *)(uintptr_t)ptr, &value, sizeof(value));
}

void __calynda_store_sized(CalyndaRtWord ptr,
                           CalyndaRtWord value,
                           CalyndaRtWord size) {
    memcpy((void *)(uintptr_t)ptr, &value, (size_t)size);
}

CalyndaRtWord __calynda_stackalloc(CalyndaRtWord size) {
    /* stackalloc() provides manual-scope scratch storage. The compiler
       auto-registers cleanup for the returned address, so the runtime may
       back it with an ordinary allocation without changing the language
       contract. */
    return (CalyndaRtWord)(uintptr_t)malloc((size_t)size);
}
