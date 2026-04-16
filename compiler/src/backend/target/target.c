#include "target.h"

#include <string.h>

/* Forward declarations for per-target descriptor accessors */
const TargetDescriptor *target_get_x86_64_descriptor(void);
const TargetDescriptor *target_get_aarch64_descriptor(void);
const TargetDescriptor *target_get_riscv64_descriptor(void);

const TargetDescriptor *target_get_descriptor(TargetKind kind) {
    switch (kind) {
    case TARGET_KIND_X86_64_SYSV_ELF:
        return target_get_x86_64_descriptor();
    case TARGET_KIND_AARCH64_AAPCS_ELF:
        return target_get_aarch64_descriptor();
    case TARGET_KIND_RISCV64_LP64D_ELF:
        return target_get_riscv64_descriptor();
    }
    return target_get_x86_64_descriptor();
}

const char *target_kind_name(TargetKind kind) {
    const TargetDescriptor *desc = target_get_descriptor(kind);
    return desc ? desc->name : "unknown";
}

const TargetDescriptor *target_get_default(void) {
    return target_get_descriptor(TARGET_KIND_X86_64_SYSV_ELF);
}

TargetKind target_kind_from_name(const char *name, bool *found) {
    if (!name || !found) {
        if (found) {
            *found = false;
        }
        return TARGET_KIND_X86_64_SYSV_ELF;
    }

    if (strcmp(name, "x86_64") == 0 ||
        strcmp(name, "x86_64_sysv_elf") == 0) {
        *found = true;
        return TARGET_KIND_X86_64_SYSV_ELF;
    }

    if (strcmp(name, "aarch64") == 0 ||
        strcmp(name, "arm64") == 0 ||
        strcmp(name, "aarch64_aapcs_elf") == 0) {
        *found = true;
        return TARGET_KIND_AARCH64_AAPCS_ELF;
    }

    if (strcmp(name, "riscv64") == 0 ||
        strcmp(name, "rv64") == 0 ||
        strcmp(name, "riscv64_lp64d_elf") == 0) {
        *found = true;
        return TARGET_KIND_RISCV64_LP64D_ELF;
    }

    *found = false;
    return TARGET_KIND_X86_64_SYSV_ELF;
}

const char *target_register_name(const TargetDescriptor *target, int register_id) {
    size_t i;

    if (!target) {
        return "?";
    }

    for (i = 0; i < target->register_count; i++) {
        if (target->registers[i].id == register_id) {
            return target->registers[i].name;
        }
    }

    return "?";
}

int target_register_id_by_name(const TargetDescriptor *target, const char *name) {
    size_t i;

    if (!target || !name) {
        return -1;
    }

    for (i = 0; i < target->register_count; i++) {
        if (strcmp(target->registers[i].name, name) == 0) {
            return target->registers[i].id;
        }
    }

    return -1;
}

bool target_register_is_callee_saved(const TargetDescriptor *target, int register_id) {
    size_t i;

    if (!target) {
        return false;
    }

    for (i = 0; i < target->callee_saved_register_count; i++) {
        if (target->callee_saved_registers[i].id == register_id) {
            return true;
        }
    }

    return false;
}
