#ifndef CALYNDA_MIR_INTERNAL_H
#define CALYNDA_MIR_INTERNAL_H
#include "mir.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    MirProgram       *program;
    const HirProgram *hir_program;
    size_t            next_lambda_unit_index;
    bool              global_bounds_check;
} MirBuildContext;

typedef struct {
    MirValue      argument;
    MirValue      callee;
    AstSourceSpan source_span;
} MirDeferredCleanup;

typedef struct {
    MirBuildContext *build;
    MirUnit         *unit;
    size_t           current_block_index;
    size_t           next_synthetic_local_index;
    bool             in_checked_manual;
    bool             is_nlr_block;  /* returns in this unit invoke NLR helpers */
    const Symbol   **escape_set;        /* symbols that must be heap-cell'd (escape analysis) */
    size_t           escape_set_count;
    MirDeferredCleanup *deferred_cleanups;
    size_t              deferred_cleanup_count;
    size_t              deferred_cleanup_capacity;
    size_t             *manual_cleanup_markers;
    size_t              manual_cleanup_depth;
    size_t              manual_cleanup_capacity;
} MirUnitBuildContext;

typedef enum {
    MIR_LVALUE_LOCAL = 0,
    MIR_LVALUE_GLOBAL,
    MIR_LVALUE_INDEX,
    MIR_LVALUE_MEMBER
} MirLValueKind;

typedef struct {
    MirLValueKind kind;
    CheckedType   type;
    bool          is_cell; /* if true and kind==MIR_LVALUE_LOCAL, r/w via cell helpers */
    union {
        size_t local_index;
        char  *global_name;
        struct {
            MirValue target;
            MirValue index;
        } index;
        struct {
            MirValue target;
            char    *member;
        } member;
    } as;
} MirLValue;

typedef struct {
    const Symbol *symbol;
    CheckedType   type;
    bool          is_cell;  /* captured as a cell reference */
} MirCaptureSpec;

typedef struct {
    MirCaptureSpec *items;
    size_t          count;
    size_t          capacity;
} MirCaptureList;

typedef struct {
    const Symbol **items;
    size_t         count;
    size_t         capacity;
} MirBoundSymbolSet;

static inline const char *mr_hir_symbol_global_name(const HirSymbolRef *symbol) {
    if (!symbol) {
        return NULL;
    }

    if (symbol->symbol &&
        (symbol->symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING ||
         symbol->symbol->kind == SYMBOL_KIND_ASM_BINDING) &&
        symbol->symbol->qualified_name) {
        return symbol->symbol->qualified_name;
    }

    return symbol->name;
}

static inline const char *mr_named_symbol_global_name(const char *fallback_name,
                                                      const Symbol *symbol) {
    if (symbol &&
        (symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING ||
         symbol->kind == SYMBOL_KIND_ASM_BINDING) &&
        symbol->qualified_name) {
        return symbol->qualified_name;
    }

    return fallback_name;
}

extern const char *MIR_MODULE_INIT_NAME;

/* mir_union.c */
bool mr_is_union_variant_call(const HirExpression *expression,
                              const char **out_union_name,
                              const char **out_variant_name);
bool mr_is_union_variant_member(const HirExpression *expression,
                                const char **out_union_name,
                                const char **out_variant_name);
const HirUnionDecl *mr_find_hir_union_decl(const MirBuildContext *build,
                                           const char *union_name);
bool mr_find_hir_union_variant(const MirBuildContext *build,
                               const char *union_name,
                               const char *variant_name,
                               size_t *out_variant_index,
                               size_t *out_variant_count);
CalyndaRtTypeTag mr_checked_type_to_runtime_tag(CheckedType type);
bool mr_init_union_new_instruction(MirBuildContext *build,
                                   MirInstruction *instruction,
                                   const char *union_name,
                                   AstSourceSpan source_span);

/* mir_capture.c */
void mr_capture_list_free(MirCaptureList *captures);
bool mr_capture_list_add(MirBuildContext *context,
                         MirCaptureList *captures,
                         const Symbol *symbol,
                         CheckedType type,
                         AstSourceSpan source_span);
void mr_bound_symbol_set_free(MirBoundSymbolSet *set);
bool mr_bound_symbol_set_contains(const MirBoundSymbolSet *set,
                                  const Symbol *symbol);
bool mr_bound_symbol_set_add(MirBuildContext *context,
                             MirBoundSymbolSet *set,
                             const Symbol *symbol,
                             AstSourceSpan source_span);
bool mr_bound_symbol_set_clone(MirBuildContext *context,
                               const MirBoundSymbolSet *source,
                               MirBoundSymbolSet *clone,
                               AstSourceSpan source_span);

/* mir_capture_analysis.c */
bool mr_analyze_lambda_captures(MirBuildContext *context,
                                const HirLambdaExpression *lambda,
                                AstSourceSpan source_span,
                                MirCaptureList *captures);
bool mr_compute_block_escape_set(MirBuildContext *context,
                                 const HirBlock *block,
                                 const Symbol ***escape_out,
                                 size_t *escape_count_out);
bool mr_symbol_in_escape_set(const Symbol * const *escape_set,
                             size_t escape_count,
                             const Symbol *symbol);

/* mir_value.c */
MirValue mr_invalid_value(void);
bool mr_value_clone(MirBuildContext *context,
                    const MirValue *source, MirValue *clone);
void mr_value_free(MirValue *value);
void mr_template_part_free(MirTemplatePart *part);
void mr_lvalue_free(MirLValue *lvalue);
bool mr_value_from_literal(MirBuildContext *context,
                           const HirLiteral *literal,
                           CheckedType type, MirValue *value);
bool mr_value_from_global(MirBuildContext *context,
                          const char *name,
                          CheckedType type, MirValue *value);

/* mir_cleanup.c */
void mr_instruction_free(MirInstruction *instruction);
void mr_basic_block_free(MirBasicBlock *block);
void mr_unit_free(MirUnit *unit);

/* mir_builders.c */
bool mr_reserve_items(void **items, size_t *capacity,
                      size_t needed, size_t item_size);
bool mr_source_span_is_valid(AstSourceSpan span);
CheckedType mr_checked_type_void_value(void);
void mr_set_error(MirBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format, ...);
bool mr_append_unit(MirProgram *program, MirUnit unit);
bool mr_append_local(MirUnit *unit, MirLocal local);
bool mr_append_block(MirUnit *unit, MirBasicBlock block);
bool mr_append_instruction(MirBasicBlock *block, MirInstruction instruction);
MirBasicBlock *mr_current_block(MirUnitBuildContext *context);
bool mr_create_block(MirUnitBuildContext *context, size_t *block_index);
size_t mr_find_local_index(const MirUnit *unit, const Symbol *symbol);

/* mir_store.c */
bool mr_append_call_global_instruction(MirUnitBuildContext *context,
                                       const char *global_name,
                                       CheckedType return_type,
                                       AstSourceSpan source_span);
bool mr_append_store_local_instruction(MirUnitBuildContext *context,
                                       size_t local_index,
                                       MirValue value,
                                       AstSourceSpan source_span);
bool mr_append_store_global_instruction(MirUnitBuildContext *context,
                                        const char *global_name,
                                        MirValue value,
                                        AstSourceSpan source_span);
bool mr_append_store_index_instruction(MirUnitBuildContext *context,
                                       const MirValue *target,
                                       const MirValue *index,
                                       MirValue value,
                                       AstSourceSpan source_span);
bool mr_append_store_member_instruction(MirUnitBuildContext *context,
                                        const MirValue *target,
                                        const char *member,
                                        MirValue value,
                                        AstSourceSpan source_span);
bool mr_append_synthetic_local(MirUnitBuildContext *context,
                               const char *name_prefix,
                               CheckedType type,
                               AstSourceSpan source_span,
                               size_t *local_index);
void mr_set_goto_terminator(MirUnitBuildContext *context,
                            size_t target_block_index);
/* Cell helpers: emit call to __calynda_rt_cell_alloc/read/write */
bool mr_emit_cell_alloc(MirUnitBuildContext *context,
                        MirValue initial_value,
                        AstSourceSpan source_span,
                        size_t cell_local_index);
bool mr_emit_cell_read(MirUnitBuildContext *context,
                       size_t cell_local_index,
                       CheckedType value_type,
                       AstSourceSpan source_span,
                       MirValue *out);
bool mr_emit_cell_write(MirUnitBuildContext *context,
                        size_t cell_local_index,
                        MirValue value,
                        AstSourceSpan source_span);

#include "mir_internal_p2.inc"
#endif /* CALYNDA_MIR_INTERNAL_H */
