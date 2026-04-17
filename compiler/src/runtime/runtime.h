#ifndef CALYNDA_RUNTIME_H
#define CALYNDA_RUNTIME_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

typedef uint64_t CalyndaRtWord;

#define CALYNDA_RT_EXIT_RUNTIME_ERROR 70
#define CALYNDA_RT_EXIT_RUNTIME_OOM   71

typedef CalyndaRtWord (*CalyndaRtProgramStartEntry)(CalyndaRtWord arguments);

#define CALYNDA_RT_OBJECT_MAGIC 0x434C5944u

typedef enum {
    CALYNDA_RT_OBJECT_STRING = 0,
    CALYNDA_RT_OBJECT_ARRAY,
    CALYNDA_RT_OBJECT_CLOSURE,
    CALYNDA_RT_OBJECT_PACKAGE,
    CALYNDA_RT_OBJECT_EXTERN_CALLABLE,
    CALYNDA_RT_OBJECT_UNION,
    CALYNDA_RT_OBJECT_HETERO_ARRAY,
    CALYNDA_RT_OBJECT_THREAD,
    CALYNDA_RT_OBJECT_MUTEX,
    CALYNDA_RT_OBJECT_FUTURE,
    CALYNDA_RT_OBJECT_ATOMIC
} CalyndaRtObjectKind;

typedef enum {
    CALYNDA_RT_TEMPLATE_TAG_TEXT = 0,
    CALYNDA_RT_TEMPLATE_TAG_VALUE
} CalyndaRtTemplateTag;

typedef enum {
    CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT = 0
} CalyndaRtExternCallableKind;

typedef enum {
    CALYNDA_RT_TYPE_VOID = 0,
    CALYNDA_RT_TYPE_BOOL,
    CALYNDA_RT_TYPE_INT32,
    CALYNDA_RT_TYPE_INT64,
    CALYNDA_RT_TYPE_STRING,
    CALYNDA_RT_TYPE_ARRAY,
    CALYNDA_RT_TYPE_CLOSURE,
    CALYNDA_RT_TYPE_EXTERNAL,
    CALYNDA_RT_TYPE_RAW_WORD,
    CALYNDA_RT_TYPE_UNION,
    CALYNDA_RT_TYPE_HETERO_ARRAY
} CalyndaRtTypeTag;

typedef struct {
    uint32_t magic;
    uint32_t kind;
} CalyndaRtObjectHeader;

typedef struct {
    CalyndaRtObjectHeader header;
    size_t                length;
    char                 *bytes;
} CalyndaRtString;

typedef struct {
    CalyndaRtObjectHeader header;
    size_t                count;
    CalyndaRtWord        *elements;
} CalyndaRtArray;

typedef CalyndaRtWord (*CalyndaRtClosureEntry)(const CalyndaRtWord *captures,
                                               size_t capture_count,
                                               const CalyndaRtWord *arguments,
                                               size_t argument_count);

typedef struct {
    CalyndaRtObjectHeader header;
    CalyndaRtClosureEntry entry;
    size_t                capture_count;
    CalyndaRtWord        *captures;
} CalyndaRtClosure;

typedef struct {
    CalyndaRtObjectHeader header;
    const char           *name;
} CalyndaRtPackage;

typedef struct {
    CalyndaRtObjectHeader     header;
    CalyndaRtExternCallableKind kind;
    const char               *name;
} CalyndaRtExternCallable;

typedef struct {
    CalyndaRtWord tag;
    CalyndaRtWord payload;
} CalyndaRtTemplatePart;

typedef struct {
    const char       *name;
    size_t            generic_param_count;
    const CalyndaRtTypeTag *generic_param_tags;
    size_t            variant_count;
    const char *const *variant_names;
    const CalyndaRtTypeTag *variant_payload_tags;
} CalyndaRtTypeDescriptor;

typedef struct {
    CalyndaRtObjectHeader        header;
    const CalyndaRtTypeDescriptor *type_desc;
    uint32_t                      tag;
    CalyndaRtWord                 payload;
} CalyndaRtUnion;

typedef struct {
    CalyndaRtObjectHeader  header;
    const CalyndaRtTypeDescriptor *type_desc;
    size_t                 count;
    CalyndaRtWord         *elements;
} CalyndaRtHeteroArray;

typedef struct {
    CalyndaRtObjectHeader header;
    pthread_t             thread;
    bool                  joined;
} CalyndaRtThread;

typedef struct {
    CalyndaRtObjectHeader header;
    pthread_mutex_t       mutex;
} CalyndaRtMutex;

/*
 * Future<T>: spawned from a non-void callable.
 * The thread trampoline writes `result` before exiting; pthread_join()
 * in future_get provides the required happens-before guarantee so no
 * additional mutex is needed for the result field itself.
 */
typedef struct {
    CalyndaRtObjectHeader header;
    pthread_t             thread;
    bool                  joined;
    CalyndaRtWord         result;
    CalyndaRtWord         callable; /* strong ref keeps callable alive */
} CalyndaRtFuture;

/*
 * Atomic<T>: single-word value backed by C11 _Atomic.
 * Supports load, store, and exchange with sequentially-consistent ordering.
 */
typedef struct {
    CalyndaRtObjectHeader   header;
    _Atomic CalyndaRtWord   value;
} CalyndaRtAtomic;

#define CALYNDA_RT_THREAD_SPAWN   "__calynda_rt_thread_spawn"
#define CALYNDA_RT_THREAD_JOIN    "__calynda_rt_thread_join"
#define CALYNDA_RT_THREAD_CANCEL  "__calynda_rt_thread_cancel"
#define CALYNDA_RT_MUTEX_NEW      "__calynda_rt_mutex_new"
#define CALYNDA_RT_MUTEX_LOCK     "__calynda_rt_mutex_lock"
#define CALYNDA_RT_MUTEX_UNLOCK   "__calynda_rt_mutex_unlock"
/* Future helpers (alpha.2) */
#define CALYNDA_RT_FUTURE_SPAWN   "__calynda_rt_future_spawn"
#define CALYNDA_RT_FUTURE_GET     "__calynda_rt_future_get"
#define CALYNDA_RT_FUTURE_CANCEL  "__calynda_rt_future_cancel"
/* Atomic helpers (alpha.2) */
#define CALYNDA_RT_ATOMIC_NEW      "__calynda_rt_atomic_new"
#define CALYNDA_RT_ATOMIC_LOAD     "__calynda_rt_atomic_load"
#define CALYNDA_RT_ATOMIC_STORE    "__calynda_rt_atomic_store"
#define CALYNDA_RT_ATOMIC_EXCHANGE "__calynda_rt_atomic_exchange"
#define CALYNDA_RT_ARRAY_CAR       "__calynda_rt_array_car"
#define CALYNDA_RT_ARRAY_CDR       "__calynda_rt_array_cdr"

extern CalyndaRtPackage __calynda_pkg_stdlib;

bool calynda_rt_is_object(CalyndaRtWord word);
const CalyndaRtObjectHeader *calynda_rt_as_object(CalyndaRtWord word);
const char *calynda_rt_object_kind_name(CalyndaRtObjectKind kind);
const char *calynda_rt_type_tag_name(CalyndaRtTypeTag tag);

bool calynda_rt_dump_layout(FILE *out);
char *calynda_rt_dump_layout_to_string(void);

bool calynda_rt_format_word(CalyndaRtWord word, char *buffer, size_t buffer_size);
const char *calynda_rt_string_bytes(CalyndaRtWord word);
size_t calynda_rt_array_length(CalyndaRtWord word);
int calynda_rt_start_process(CalyndaRtProgramStartEntry entry, int argc, char **argv);
void calynda_rt_register_static_object(const CalyndaRtObjectHeader *object);

CalyndaRtWord calynda_rt_make_string_copy(const char *bytes);

CalyndaRtWord __calynda_rt_closure_new(CalyndaRtClosureEntry code_ptr,
                                       size_t capture_count,
                                       const CalyndaRtWord *captures);
CalyndaRtWord calynda_rt_callable_dispatch(CalyndaRtWord callable,
                                           const CalyndaRtWord *arguments,
                                           size_t argument_count);
CalyndaRtWord __calynda_rt_call_callable(CalyndaRtWord callable,
                                         size_t argument_count,
                                         const CalyndaRtWord *arguments);
CalyndaRtWord __calynda_rt_member_load(CalyndaRtWord target, const char *member);
CalyndaRtWord __calynda_rt_stdlib_print0(void);
CalyndaRtWord __calynda_rt_stdlib_print1(CalyndaRtWord value);
CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index);
CalyndaRtWord __calynda_rt_array_literal(size_t element_count,
                                         const CalyndaRtWord *elements);
CalyndaRtWord __calynda_rt_array_car(CalyndaRtWord target);
CalyndaRtWord __calynda_rt_array_cdr(CalyndaRtWord target);
CalyndaRtWord __calynda_rt_template_build(size_t part_count,
                                          const CalyndaRtTemplatePart *parts);
void __calynda_rt_store_index(CalyndaRtWord target,
                              CalyndaRtWord index,
                              CalyndaRtWord value);
void __calynda_rt_store_member(CalyndaRtWord target,
                               const char *member,
                               CalyndaRtWord value);
void __calynda_rt_throw(CalyndaRtWord value) __attribute__((noreturn));
CalyndaRtWord __calynda_rt_cast_value(CalyndaRtWord source,
                                      CalyndaRtTypeTag target_type_tag);

CalyndaRtWord __calynda_rt_union_new(const CalyndaRtTypeDescriptor *type_desc,
                                     uint32_t variant_tag,
                                     CalyndaRtWord payload);
uint32_t __calynda_rt_union_get_tag(CalyndaRtWord value);
CalyndaRtWord __calynda_rt_union_get_payload(CalyndaRtWord value);
bool __calynda_rt_union_check_tag(CalyndaRtWord value, uint32_t expected_tag);

CalyndaRtWord __calynda_rt_hetero_array_new(const CalyndaRtTypeDescriptor *type_desc,
                                            size_t element_count,
                                            const CalyndaRtWord *elements);
CalyndaRtTypeTag __calynda_rt_hetero_array_get_tag(CalyndaRtWord target, CalyndaRtWord index);

CalyndaRtWord __calynda_rt_thread_spawn(CalyndaRtWord callable);
void          __calynda_rt_thread_join(CalyndaRtWord thread_handle);
void          __calynda_rt_thread_cancel(CalyndaRtWord thread_handle);
CalyndaRtWord __calynda_rt_mutex_new(void);
void          __calynda_rt_mutex_lock(CalyndaRtWord mutex_handle);
void          __calynda_rt_mutex_unlock(CalyndaRtWord mutex_handle);

/* Future helpers (alpha.2): spawn returns Future object, get joins and
   returns the stored result, cancel calls pthread_cancel + join. */
CalyndaRtWord __calynda_rt_future_spawn(CalyndaRtWord callable);
CalyndaRtWord __calynda_rt_future_get(CalyndaRtWord future_handle);
void          __calynda_rt_future_cancel(CalyndaRtWord future_handle);

/* Atomic helpers (alpha.2): single-word SC atomic operations. */
CalyndaRtWord __calynda_rt_atomic_new(CalyndaRtWord initial_value);
CalyndaRtWord __calynda_rt_atomic_load(CalyndaRtWord atomic_handle);
void          __calynda_rt_atomic_store(CalyndaRtWord atomic_handle,
                                        CalyndaRtWord value);
CalyndaRtWord __calynda_rt_atomic_exchange(CalyndaRtWord atomic_handle,
                                           CalyndaRtWord new_value);

/* Manual memory pointer operations */
CalyndaRtWord __calynda_deref(CalyndaRtWord ptr);
CalyndaRtWord __calynda_deref_sized(CalyndaRtWord ptr, CalyndaRtWord size);
CalyndaRtWord __calynda_addr(CalyndaRtWord value);
CalyndaRtWord __calynda_offset(CalyndaRtWord ptr, CalyndaRtWord count);
CalyndaRtWord __calynda_offset_stride(CalyndaRtWord ptr, CalyndaRtWord count,
                                      CalyndaRtWord stride);
void          __calynda_store(CalyndaRtWord ptr, CalyndaRtWord value);
void          __calynda_store_sized(CalyndaRtWord ptr, CalyndaRtWord value,
                                    CalyndaRtWord size);
/* stackalloc() returns scratch storage whose lifetime ends with the enclosing
    manual scope once compiler-inserted cleanup runs. */
CalyndaRtWord __calynda_stackalloc(CalyndaRtWord size);

/* Bounds-checked memory operations (manual checked {} mode). */
CalyndaRtWord __calynda_bc_malloc(CalyndaRtWord size);
CalyndaRtWord __calynda_bc_calloc(CalyndaRtWord n, CalyndaRtWord size);
CalyndaRtWord __calynda_bc_realloc(CalyndaRtWord ptr, CalyndaRtWord new_size);
void          __calynda_bc_free(CalyndaRtWord ptr);
CalyndaRtWord __calynda_bc_stackalloc(CalyndaRtWord size);
CalyndaRtWord __calynda_bc_deref(CalyndaRtWord ptr);
void          __calynda_bc_store(CalyndaRtWord ptr, CalyndaRtWord value);
CalyndaRtWord __calynda_bc_offset(CalyndaRtWord ptr, CalyndaRtWord idx,
                                  CalyndaRtWord elem_size);

#endif /* CALYNDA_RUNTIME_H */
