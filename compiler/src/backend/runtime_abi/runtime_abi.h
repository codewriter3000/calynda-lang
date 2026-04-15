#ifndef CALYNDA_RUNTIME_ABI_H
#define CALYNDA_RUNTIME_ABI_H

#include "codegen.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    RUNTIME_ABI_RETURN_VALUE = 0,
    RUNTIME_ABI_RETURN_VOID,
    RUNTIME_ABI_RETURN_NORETURN
} RuntimeAbiReturnKind;

typedef enum {
    RUNTIME_ABI_ARG_UNIT_LABEL = 0,
    RUNTIME_ABI_ARG_CAPTURE_COUNT,
    RUNTIME_ABI_ARG_CAPTURE_PACK,
    RUNTIME_ABI_ARG_CALLABLE_VALUE,
    RUNTIME_ABI_ARG_ARGUMENT_COUNT,
    RUNTIME_ABI_ARG_ARGUMENT_PACK,
    RUNTIME_ABI_ARG_TARGET_VALUE,
    RUNTIME_ABI_ARG_MEMBER_SYMBOL,
    RUNTIME_ABI_ARG_INDEX_VALUE,
    RUNTIME_ABI_ARG_ELEMENT_COUNT,
    RUNTIME_ABI_ARG_ELEMENT_PACK,
    RUNTIME_ABI_ARG_TEMPLATE_PART_COUNT,
    RUNTIME_ABI_ARG_TEMPLATE_PART_PACK,
    RUNTIME_ABI_ARG_STORE_VALUE,
    RUNTIME_ABI_ARG_THROW_VALUE,
    RUNTIME_ABI_ARG_SOURCE_VALUE,
    RUNTIME_ABI_ARG_TARGET_TYPE_TAG,
    RUNTIME_ABI_ARG_TYPE_DESCRIPTOR,
    RUNTIME_ABI_ARG_VARIANT_TAG,
    RUNTIME_ABI_ARG_PAYLOAD_VALUE,
    RUNTIME_ABI_ARG_EXPECTED_TAG,
    RUNTIME_ABI_ARG_ELEMENT_TAG_PACK
} RuntimeAbiArgumentRole;

typedef enum {
    RUNTIME_ABI_PACK_NONE = 0,
    RUNTIME_ABI_PACK_VALUE_WORDS,
    RUNTIME_ABI_PACK_TEMPLATE_PARTS
} RuntimeAbiPackKind;

typedef struct {
    RuntimeAbiArgumentRole role;
    CodegenRegister        reg;
} RuntimeAbiArgument;

typedef struct {
    CodegenRuntimeHelper helper;
    const char          *name;
    RuntimeAbiReturnKind return_kind;
    RuntimeAbiArgument   arguments[3];
    size_t               argument_count;
    RuntimeAbiPackKind   pack_kind;
} RuntimeAbiHelperSignature;

const RuntimeAbiHelperSignature *runtime_abi_get_helper_signature(CodegenTargetKind target,
                                                                  CodegenRuntimeHelper helper);
CodegenRegister runtime_abi_get_helper_argument_register(CodegenTargetKind target,
                                                         size_t argument_index);
const char *runtime_abi_argument_role_name(RuntimeAbiArgumentRole role);
const char *runtime_abi_pack_kind_name(RuntimeAbiPackKind kind);
const char *runtime_abi_return_kind_name(RuntimeAbiReturnKind kind);

bool runtime_abi_format_type_tag(CheckedType type, char *buffer, size_t buffer_size);

bool runtime_abi_dump_surface(FILE *out, CodegenTargetKind target);
char *runtime_abi_dump_surface_to_string(CodegenTargetKind target);

#endif /* CALYNDA_RUNTIME_ABI_H */