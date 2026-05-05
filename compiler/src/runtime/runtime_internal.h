#ifndef RUNTIME_INTERNAL_H
#define RUNTIME_INTERNAL_H

#include "runtime.h"

#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global static objects (defined in runtime.c) */
extern CalyndaRtExternCallable STDOUT_PRINT_CALLABLE;
extern CalyndaRtExternCallable STDIN_INPUT_CALLABLE;

/* runtime.c — registry and object creation */
bool rt_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size);
bool rt_register_object_pointer(void *pointer);
bool rt_register_static_object_pointer(void *pointer);
void rt_cleanup_registered_objects(void);
typedef struct RtFailureContext {
    jmp_buf jump;
    int exit_code;
    struct RtFailureContext *previous;
} RtFailureContext;
void rt_failure_context_push(RtFailureContext *context);
void rt_failure_context_pop(RtFailureContext *context);
void rt_record_process_failure(int exit_code);
int rt_process_failure_code(void);
void rt_reset_process_failure(void);
_Noreturn void rt_fatal_now(int exit_code);
_Noreturn void rt_fatalf(int exit_code, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
void rt_describe_word(CalyndaRtWord word, char *buffer, size_t buffer_size);
CalyndaRtWord rt_make_object_word(void *pointer);
CalyndaRtString *rt_new_string_object(const char *bytes, size_t length);
CalyndaRtArray *rt_new_array_object(size_t element_count, const CalyndaRtWord *elements);
CalyndaRtClosure *rt_new_closure_object(CalyndaRtClosureEntry code_ptr,
                                        size_t capture_count,
                                        const CalyndaRtWord *captures);

/* runtime_format.c — formatting utilities */
bool rt_format_word_internal(CalyndaRtWord word, char *buffer, size_t buffer_size);
bool rt_format_hetero_array_text(const CalyndaRtHeteroArray *array,
                                 char *buffer,
                                 size_t buffer_size);
CalyndaRtWord rt_word_from_signed(long long value);
long long rt_signed_from_word(CalyndaRtWord value);
bool rt_is_runtime_string_word(CalyndaRtWord word);
CalyndaRtThread *rt_new_thread_object(void);
CalyndaRtMutex *rt_new_mutex_object(void);
CalyndaRtFuture *rt_new_future_object(CalyndaRtWord callable);
CalyndaRtAtomic *rt_new_atomic_object(void);
CalyndaRtWord rt_dispatch_extern_callable(const CalyndaRtExternCallable *callable,
                                          size_t argument_count,
                                          const CalyndaRtWord *arguments);

#endif
