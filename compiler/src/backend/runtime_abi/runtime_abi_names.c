#include "runtime_abi.h"

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
    case RUNTIME_ABI_ARG_TYPE_DESCRIPTOR:
        return "type_descriptor";
    case RUNTIME_ABI_ARG_VARIANT_TAG:
        return "variant_tag";
    case RUNTIME_ABI_ARG_PAYLOAD_VALUE:
        return "payload_value";
    case RUNTIME_ABI_ARG_EXPECTED_TAG:
        return "expected_tag";
    case RUNTIME_ABI_ARG_ELEMENT_TAG_PACK:
        return "element_tag_pack";
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
