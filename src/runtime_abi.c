#include "runtime_abi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const CodegenRegister X86_64_SYSV_HELPER_ARG_REGS[] = {
    CODEGEN_REG_RDI,
    CODEGEN_REG_RSI,
    CODEGEN_REG_RDX,
    CODEGEN_REG_RCX,
    CODEGEN_REG_R8,
    CODEGEN_REG_R9
};

static const RuntimeAbiHelperSignature X86_64_SYSV_HELPERS[] = {
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
    }
};

static bool write_helper_signature(FILE *out, const RuntimeAbiHelperSignature *signature);

const RuntimeAbiHelperSignature *runtime_abi_get_helper_signature(CodegenTargetKind target,
                                                                  CodegenRuntimeHelper helper) {
    size_t i;

    if (target != CODEGEN_TARGET_X86_64_SYSV_ELF) {
        return NULL;
    }

    for (i = 0; i < sizeof(X86_64_SYSV_HELPERS) / sizeof(X86_64_SYSV_HELPERS[0]); i++) {
        if (X86_64_SYSV_HELPERS[i].helper == helper) {
            return &X86_64_SYSV_HELPERS[i];
        }
    }

    return NULL;
}

CodegenRegister runtime_abi_get_helper_argument_register(CodegenTargetKind target,
                                                         size_t argument_index) {
    if (target != CODEGEN_TARGET_X86_64_SYSV_ELF ||
        argument_index >= sizeof(X86_64_SYSV_HELPER_ARG_REGS) / sizeof(X86_64_SYSV_HELPER_ARG_REGS[0])) {
        return CODEGEN_REG_RAX;
    }

    return X86_64_SYSV_HELPER_ARG_REGS[argument_index];
}

const char *runtime_abi_argument_role_name(RuntimeAbiArgumentRole role) {
    switch (role) {
    case RUNTIME_ABI_ARG_UNIT_LABEL:
        return "unit_label";
    case RUNTIME_ABI_ARG_CAPTURE_COUNT:
        return "capture_count";
    case RUNTIME_ABI_ARG_CAPTURE_PACK:
        return "capture_pack";
    case RUNTIME_ABI_ARG_CALLABLE_VALUE:
        return "callable";
    case RUNTIME_ABI_ARG_ARGUMENT_COUNT:
        return "argument_count";
    case RUNTIME_ABI_ARG_ARGUMENT_PACK:
        return "argument_pack";
    case RUNTIME_ABI_ARG_TARGET_VALUE:
        return "target_value";
    case RUNTIME_ABI_ARG_MEMBER_SYMBOL:
        return "member_symbol";
    case RUNTIME_ABI_ARG_INDEX_VALUE:
        return "index_value";
    case RUNTIME_ABI_ARG_ELEMENT_COUNT:
        return "element_count";
    case RUNTIME_ABI_ARG_ELEMENT_PACK:
        return "element_pack";
    case RUNTIME_ABI_ARG_TEMPLATE_PART_COUNT:
        return "part_count";
    case RUNTIME_ABI_ARG_TEMPLATE_PART_PACK:
        return "part_pack";
    case RUNTIME_ABI_ARG_STORE_VALUE:
        return "store_value";
    case RUNTIME_ABI_ARG_THROW_VALUE:
        return "throw_value";
    case RUNTIME_ABI_ARG_SOURCE_VALUE:
        return "source_value";
    case RUNTIME_ABI_ARG_TARGET_TYPE_TAG:
        return "target_type_tag";
    }

    return "unknown";
}

const char *runtime_abi_pack_kind_name(RuntimeAbiPackKind kind) {
    switch (kind) {
    case RUNTIME_ABI_PACK_NONE:
        return "none";
    case RUNTIME_ABI_PACK_VALUE_WORDS:
        return "value-word";
    case RUNTIME_ABI_PACK_TEMPLATE_PARTS:
        return "template-part(tag,payload)";
    }

    return "unknown";
}

const char *runtime_abi_return_kind_name(RuntimeAbiReturnKind kind) {
    switch (kind) {
    case RUNTIME_ABI_RETURN_VALUE:
        return codegen_register_name(CODEGEN_REG_RAX);
    case RUNTIME_ABI_RETURN_VOID:
        return "void";
    case RUNTIME_ABI_RETURN_NORETURN:
        return "noreturn";
    }

    return "unknown";
}

bool runtime_abi_format_type_tag(CheckedType type, char *buffer, size_t buffer_size) {
    char type_name[64];

    if (!buffer || buffer_size == 0 || !checked_type_to_string(type, type_name, sizeof(type_name))) {
        return false;
    }

    return snprintf(buffer, buffer_size, "type(%s)", type_name) >= 0;
}

bool runtime_abi_dump_surface(FILE *out, CodegenTargetKind target) {
    size_t i;

    if (!out || target != CODEGEN_TARGET_X86_64_SYSV_ELF) {
        return false;
    }

    fprintf(out,
            "RuntimeAbi target=%s env=%s helper_args=[",
            codegen_target_name(target),
            codegen_register_name(CODEGEN_REG_R15));
    for (i = 0; i < sizeof(X86_64_SYSV_HELPER_ARG_REGS) / sizeof(X86_64_SYSV_HELPER_ARG_REGS[0]); i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        if (fputs(codegen_register_name(X86_64_SYSV_HELPER_ARG_REGS[i]), out) == EOF) {
            return false;
        }
    }
    if (fputs("]\n", out) == EOF) {
        return false;
    }
    if (fputs("  entry captures=r15[value-word]\n", out) == EOF) {
        return false;
    }

    for (i = 0; i < sizeof(X86_64_SYSV_HELPERS) / sizeof(X86_64_SYSV_HELPERS[0]); i++) {
        if (!write_helper_signature(out, &X86_64_SYSV_HELPERS[i])) {
            return false;
        }
    }

    return !ferror(out);
}

char *runtime_abi_dump_surface_to_string(CodegenTargetKind target) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!runtime_abi_dump_surface(stream, target) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static bool write_helper_signature(FILE *out, const RuntimeAbiHelperSignature *signature) {
    size_t i;

    if (!out || !signature) {
        return false;
    }

    fprintf(out,
            "  helper %s return=%s args=[",
            signature->name,
            runtime_abi_return_kind_name(signature->return_kind));
    for (i = 0; i < signature->argument_count; i++) {
        if (i > 0 && fputs(", ", out) == EOF) {
            return false;
        }
        fprintf(out,
                "%s=%s",
                codegen_register_name(signature->arguments[i].reg),
                runtime_abi_argument_role_name(signature->arguments[i].role));
    }
    if (fputs("]", out) == EOF) {
        return false;
    }
    if (signature->pack_kind != RUNTIME_ABI_PACK_NONE) {
        fprintf(out, " pack=%s", runtime_abi_pack_kind_name(signature->pack_kind));
    }
    if (fputc('\n', out) == EOF) {
        return false;
    }

    return true;
}