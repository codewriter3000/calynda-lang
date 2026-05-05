    /* Future helpers (alpha.2) */
    {
        CODEGEN_RUNTIME_FUTURE_SPAWN,
        CALYNDA_RT_FUTURE_SPAWN,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_CALLABLE_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_FUTURE_GET,
        CALYNDA_RT_FUTURE_GET,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_FUTURE_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_FUTURE_CANCEL,
        CALYNDA_RT_FUTURE_CANCEL,
        RUNTIME_ABI_RETURN_VOID,
        {
            { RUNTIME_ABI_ARG_FUTURE_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    /* Atomic helpers (alpha.2) */
    {
        CODEGEN_RUNTIME_ATOMIC_NEW,
        CALYNDA_RT_ATOMIC_NEW,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_ATOMIC_NEW_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ATOMIC_LOAD,
        CALYNDA_RT_ATOMIC_LOAD,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_ATOMIC_VALUE, CODEGEN_REG_RDI }
        },
        1,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ATOMIC_STORE,
        CALYNDA_RT_ATOMIC_STORE,
        RUNTIME_ABI_RETURN_VOID,
        {
            { RUNTIME_ABI_ARG_ATOMIC_VALUE,     CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_ATOMIC_NEW_VALUE, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ATOMIC_EXCHANGE,
        CALYNDA_RT_ATOMIC_EXCHANGE,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_ATOMIC_VALUE,     CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_ATOMIC_NEW_VALUE, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_TYPEOF,
        CALYNDA_TYPEOF,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_SOURCE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TYPE_TEXT, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ISINT,
        CALYNDA_ISINT,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_SOURCE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TYPE_TEXT, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ISFLOAT,
        CALYNDA_ISFLOAT,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_SOURCE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TYPE_TEXT, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ISBOOL,
        CALYNDA_ISBOOL,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_SOURCE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TYPE_TEXT, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ISSTRING,
        CALYNDA_ISSTRING,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_SOURCE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TYPE_TEXT, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ISARRAY,
        CALYNDA_ISARRAY,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_SOURCE_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_TYPE_TEXT, CODEGEN_REG_RSI }
        },
        2,
        RUNTIME_ABI_PACK_NONE
    },
    {
        CODEGEN_RUNTIME_ISSAMETYPE,
        CALYNDA_ISSAMETYPE,
        RUNTIME_ABI_RETURN_VALUE,
        {
            { RUNTIME_ABI_ARG_LEFT_VALUE, CODEGEN_REG_RDI },
            { RUNTIME_ABI_ARG_LEFT_TYPE_TEXT, CODEGEN_REG_RSI },
            { RUNTIME_ABI_ARG_RIGHT_VALUE, CODEGEN_REG_RDX },
            { RUNTIME_ABI_ARG_RIGHT_TYPE_TEXT, CODEGEN_REG_RCX }
        },
        4,
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
