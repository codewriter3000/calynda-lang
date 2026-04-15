#include "type_checker_internal.h"

bool tc_validate_program_start_decls(TypeChecker *checker,
                                     const AstProgram *program) {
    const AstStartDecl *first_start = NULL;
    const AstStartDecl *first_boot = NULL;
    const AstStartDecl *duplicate = NULL;
    size_t i;

    if (!checker || !program) {
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl || decl->kind != AST_TOP_LEVEL_START) {
            continue;
        }

        if (decl->as.start_decl.is_boot) {
            if (!first_boot) {
                first_boot = &decl->as.start_decl;
            } else {
                duplicate = &decl->as.start_decl;
                break;
            }
        } else {
            if (!first_start) {
                first_start = &decl->as.start_decl;
            } else {
                duplicate = &decl->as.start_decl;
                break;
            }
        }
    }

    if (!first_start && !first_boot) {
        tc_set_error(checker,
                     "Program must declare exactly one start or boot entry point.");
        return false;
    }

    if (first_start && first_boot) {
        tc_set_error_at(checker,
                        first_boot->start_span,
                        &first_start->start_span,
                        "Program cannot declare both start and boot entry points.");
        return false;
    }

    if (duplicate) {
        const AstStartDecl *first = first_start ? first_start : first_boot;
        const char *kind = first_start ? "start" : "boot";
        tc_set_error_at(checker,
                        duplicate->start_span,
                        &first->start_span,
                        "Program cannot declare multiple %s entry points.",
                        kind);
        return false;
    }

    return true;
}

