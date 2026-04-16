#include "codegen_internal.h"
#include "runtime.h"

const char *codegen_target_name(CodegenTargetKind target) {
    return target_kind_name(target);
}

const char *codegen_register_name(CodegenRegister reg) {
    const TargetDescriptor *target = target_get_default();
    return target_register_name(target, reg);
}

const char *codegen_direct_pattern_name(CodegenDirectPattern pattern) {
    switch (pattern) {
    case CODEGEN_DIRECT_ABI_ARG_MOVE:
        return "abi-arg-move";
    case CODEGEN_DIRECT_ABI_CAPTURE_LOAD:
        return "abi-capture-load";
    case CODEGEN_DIRECT_ABI_OUTGOING_ARG:
        return "abi-outgoing-arg";
    case CODEGEN_DIRECT_SCALAR_BINARY:
        return "scalar-binary";
    case CODEGEN_DIRECT_SCALAR_UNARY:
        return "scalar-unary";
    case CODEGEN_DIRECT_SCALAR_CAST:
        return "scalar-cast";
    case CODEGEN_DIRECT_CALL_GLOBAL:
        return "call-global";
    case CODEGEN_DIRECT_STORE_SLOT:
        return "store-slot";
    case CODEGEN_DIRECT_STORE_GLOBAL:
        return "store-global";
    case CODEGEN_DIRECT_RETURN:
        return "return";
    case CODEGEN_DIRECT_JUMP:
        return "jump";
    case CODEGEN_DIRECT_BRANCH:
        return "branch";
    }

    return "unknown-direct";
}

const char *codegen_runtime_helper_name(CodegenRuntimeHelper helper) {
    switch (helper) {
    case CODEGEN_RUNTIME_CLOSURE_NEW:
        return "__calynda_rt_closure_new";
    case CODEGEN_RUNTIME_CALL_CALLABLE:
        return "__calynda_rt_call_callable";
    case CODEGEN_RUNTIME_MEMBER_LOAD:
        return "__calynda_rt_member_load";
    case CODEGEN_RUNTIME_INDEX_LOAD:
        return "__calynda_rt_index_load";
    case CODEGEN_RUNTIME_ARRAY_LITERAL:
        return "__calynda_rt_array_literal";
    case CODEGEN_RUNTIME_TEMPLATE_BUILD:
        return "__calynda_rt_template_build";
    case CODEGEN_RUNTIME_STORE_INDEX:
        return "__calynda_rt_store_index";
    case CODEGEN_RUNTIME_STORE_MEMBER:
        return "__calynda_rt_store_member";
    case CODEGEN_RUNTIME_THROW:
        return "__calynda_rt_throw";
    case CODEGEN_RUNTIME_CAST_VALUE:
        return "__calynda_rt_cast_value";
    case CODEGEN_RUNTIME_UNION_NEW:
        return "__calynda_rt_union_new";
    case CODEGEN_RUNTIME_UNION_GET_TAG:
        return "__calynda_rt_union_get_tag";
    case CODEGEN_RUNTIME_UNION_GET_PAYLOAD:
        return "__calynda_rt_union_get_payload";
    case CODEGEN_RUNTIME_HETERO_ARRAY_NEW:
        return "__calynda_rt_hetero_array_new";
    case CODEGEN_RUNTIME_HETERO_ARRAY_GET_TAG:
        return "__calynda_rt_hetero_array_get_tag";
    case CODEGEN_RUNTIME_THREAD_SPAWN:
        return CALYNDA_RT_THREAD_SPAWN;
    case CODEGEN_RUNTIME_THREAD_JOIN:
        return CALYNDA_RT_THREAD_JOIN;
    case CODEGEN_RUNTIME_THREAD_CANCEL:
        return CALYNDA_RT_THREAD_CANCEL;
    case CODEGEN_RUNTIME_MUTEX_NEW:
        return CALYNDA_RT_MUTEX_NEW;
    case CODEGEN_RUNTIME_MUTEX_LOCK:
        return CALYNDA_RT_MUTEX_LOCK;
    case CODEGEN_RUNTIME_MUTEX_UNLOCK:
        return CALYNDA_RT_MUTEX_UNLOCK;
    case CODEGEN_RUNTIME_FUTURE_SPAWN:
        return CALYNDA_RT_FUTURE_SPAWN;
    case CODEGEN_RUNTIME_FUTURE_GET:
        return CALYNDA_RT_FUTURE_GET;
    case CODEGEN_RUNTIME_FUTURE_CANCEL:
        return CALYNDA_RT_FUTURE_CANCEL;
    case CODEGEN_RUNTIME_ATOMIC_NEW:
        return CALYNDA_RT_ATOMIC_NEW;
    case CODEGEN_RUNTIME_ATOMIC_LOAD:
        return CALYNDA_RT_ATOMIC_LOAD;
    case CODEGEN_RUNTIME_ATOMIC_STORE:
        return CALYNDA_RT_ATOMIC_STORE;
    case CODEGEN_RUNTIME_ATOMIC_EXCHANGE:
        return CALYNDA_RT_ATOMIC_EXCHANGE;
    }

    return "__calynda_rt_unknown";
}
