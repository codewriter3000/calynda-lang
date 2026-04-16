#include "runtime_abi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const CodegenRegister runtime_abi_helper_arg_regs[] = {
    CODEGEN_REG_RDI,
    CODEGEN_REG_RSI,
    CODEGEN_REG_RDX,
    CODEGEN_REG_RCX,
    CODEGEN_REG_R8,
    CODEGEN_REG_R9
};

const size_t runtime_abi_helper_arg_reg_count =
    sizeof(runtime_abi_helper_arg_regs) / sizeof(runtime_abi_helper_arg_regs[0]);

const RuntimeAbiHelperSignature runtime_abi_helpers[] = {
    {
        CODEGEN_RUNTIME_CLOSURE_NEW,
        "__calynda_rt_closure_new",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_UNIT_LABEL, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_CAPTURE_COUNT, CODEGEN_REG_RSI },
            { RUNTIME_ABI_ARG_CAPTURE_PACK, CODEGEN_REG_RDX }
        },
        3,
        RUNTIME_ABI_PACK_VALUE_WORDS
    },
    {
        CODEGEN_RUNTIME_CALL_CALLABLE,
        "__calynda_rt_call_callable",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_CALLABLE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_ARGUMENT_COUNT, CODEGEN_REG_RSI },
            { RUNTIME_ABI_ARG_ARGUMENT_PACK, CODEGEN_REG_RDX }
        },
        3,
        RUNTIME_ABI_PACK_VALUE_WORDS
    },
    {
        CODEGEN_RUNTIME_MEMBER_LOAD,
        "__calynda_rt_member_load",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TARGET_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_MEMBER_SYMBOL, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_INDEX_LOAD,
        "__calynda_rt_index_load",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TARGET_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_INDEX_VALUE, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ARRAY_LITERAL,
        "__calynda_rt_array_literal",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_ELEMENT_COUNT, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_ELEMENT_PACK, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_VALUE_WORDS
    },
    {
        CODEGEN_RUNTIME_TEMPLATE_BUILD,
        "__calynda_rt_template_build",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TEMPLATE_PART_COUNT, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TEMPLATE_PART_PACK, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_TEMPLATE_PARTS
    },
    {
        CODEGEN_RUNTIME_STORE_INDEX,
        "__calynda_rt_store_index",
        RUNTIME_ABI_RETURN_VOID,
        {
            { RUNTIME_ABI_ARG_TARGET_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_INDEX_VALUE, CODEGEN_REG_RSI },
            { RUNTIME_ABI_ARG_STORE_VALUE, CODEGEN_REG_RDX }
        },
        3,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_STORE_MEMBER,
        "__calynda_rt_store_member",
        RUNTIME_ABI_RETURN_VOID,
        {
            { RUNTIME_ABI_ARG_TARGET_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_MEMBER_SYMBOL, CODEGEN_REG_RSI },
            { RUNTIME_ABI_ARG_STORE_VALUE, CODEGEN_REG_RDX }
        },
        3,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_THROW,
        "__calynda_rt_throw",
        RUNTIME_ABI_RETURN_NORETURN,
        {
            { RUNTIME_ABI_ARG_THROW_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_CAST_VALUE,
        "__calynda_rt_cast_value",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_SOURCE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TARGET_TYPE_TAG, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_UNION_NEW,
        "__calynda_rt_union_new",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TYPE_DESCRIPTOR, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_VARIANT_TAG, CODEGEN_REG_RSI },
            { RUNTIME_ABI_ARG_PAYLOAD_VALUE, CODEGEN_REG_RDX }
        },
        3,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_UNION_GET_TAG,
        "__calynda_rt_union_get_tag",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TARGET_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_UNION_GET_PAYLOAD,
        "__calynda_rt_union_get_payload",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TARGET_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_HETERO_ARRAY_NEW,
        "__calynda_rt_hetero_array_new",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TYPE_DESCRIPTOR, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_ELEMENT_COUNT, CODEGEN_REG_RSI },
            { RUNTIME_ABI_ARG_ELEMENT_PACK, CODEGEN_REG_RDX }
        },
        3,
        RUNTIME_ABI_PACK_VALUE_WORDS
    },
    {
        CODEGEN_RUNTIME_HETERO_ARRAY_GET_TAG,
        "__calynda_rt_hetero_array_get_tag",
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_TARGET_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_INDEX_VALUE, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    }
};

const size_t runtime_abi_helper_count =
    sizeof(runtime_abi_helpers) / sizeof(runtime_abi_helpers[0]);

const RuntimeAbiHelperSignature *runtime_abi_get_helper_signature(CodegenTargetKind target,
                                                                  CodegenRuntimeHelper helper) {
    size_t i;

    (void)target;

    for (i = 0; i < runtime_abi_helper_count; i++) {
        if (runtime_abi_helpers[i].helper == helper) {
            return &runtime_abi_helpers[i];
        }
    }

    return NULL;
}

CodegenRegister runtime_abi_get_helper_argument_register(CodegenTargetKind target,
                                                         size_t argument_index) {
    const TargetDescriptor *td = target_get_descriptor(target);

    if (!td || argument_index >= td->arg_register_count) {
        return td ? td->return_register.id : 0;
    }

    return td->arg_registers[argument_index].id;
}