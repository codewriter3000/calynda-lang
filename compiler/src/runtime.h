#ifndef CALYNDA_RUNTIME_H
#define CALYNDA_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint64_t CalyndaRtWord;

typedef CalyndaRtWord (*CalyndaRtProgramStartEntry)(CalyndaRtWord arguments);

#define CALYNDA_RT_OBJECT_MAGIC 0x434C5944u

typedef enum {
    CALYNDA_RT_OBJECT_STRING = 0,
    CALYNDA_RT_OBJECT_ARRAY,
    CALYNDA_RT_OBJECT_CLOSURE,
    CALYNDA_RT_OBJECT_PACKAGE,
    CALYNDA_RT_OBJECT_EXTERN_CALLABLE
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
    CALYNDA_RT_TYPE_RAW_WORD
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
CalyndaRtWord __calynda_rt_call_callable(CalyndaRtWord callable,
                                         size_t argument_count,
                                         const CalyndaRtWord *arguments);
CalyndaRtWord __calynda_rt_member_load(CalyndaRtWord target, const char *member);
CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index);
CalyndaRtWord __calynda_rt_array_literal(size_t element_count,
                                         const CalyndaRtWord *elements);
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

#endif /* CALYNDA_RUNTIME_H */