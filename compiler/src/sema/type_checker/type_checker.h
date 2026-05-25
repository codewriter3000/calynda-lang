#ifndef CALYNDA_TYPE_CHECKER_H
#define CALYNDA_TYPE_CHECKER_H

#include "ast.h"
#include "symbol_table.h"
#include "type_resolution.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CHECKED_TYPE_INVALID = 0,
    CHECKED_TYPE_VOID,
    CHECKED_TYPE_NULL,
    CHECKED_TYPE_VALUE,
    CHECKED_TYPE_EXTERNAL,
    CHECKED_TYPE_NAMED,
    CHECKED_TYPE_TYPE_PARAM,
    CHECKED_TYPE_FUNCTION
} CheckedTypeKind;

typedef struct {
    CheckedTypeKind  kind;
    AstPrimitiveType primitive;
    size_t           array_depth;
    const ArrayExtent *array_extents;
    const char      *name;             /* for CHECKED_TYPE_NAMED / TYPE_PARAM */
    size_t           generic_arg_count; /* for CHECKED_TYPE_NAMED */
    bool             is_bounds_checked; /* for ptr<T, checked> */
} CheckedType;

typedef struct {
    CheckedType             type;
    bool                    is_callable;
    CheckedType             callable_return_type;
    const AstParameterList *parameters;
    const AstType          *array_shape_type;
    size_t                  array_shape_consumed_dimensions;
    const AstType          *callable_return_array_shape_type;
    size_t                  callable_return_array_shape_consumed_dimensions;
    bool                    has_first_generic_arg;
    CheckedType             first_generic_arg_type;
} TypeCheckInfo;

typedef struct {
    const AstExpression *expression;
    TypeCheckInfo        info;
} TypeCheckExpressionEntry;

typedef struct {
    const Symbol *symbol;
    TypeCheckInfo info;
    bool          is_resolved;
    bool          is_resolving;
} TypeCheckSymbolEntry;

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} TypeCheckError;

typedef struct {
    TypeCheckError error;
    bool           is_performance;
} TypeCheckNotice;

typedef struct {
    const AstProgram           *program;
    const SymbolTable          *symbols;
    TypeResolver                resolver;
    TypeCheckExpressionEntry  **expression_entries;
    size_t                      expression_count;
    size_t                      expression_capacity;
    TypeCheckSymbolEntry      **symbol_entries;
    size_t                      symbol_count;
    size_t                      symbol_capacity;
    ArrayExtent               **owned_array_extent_blocks;
    size_t                      owned_array_extent_block_count;
    size_t                      owned_array_extent_block_capacity;
    TypeCheckNotice            *warnings;
    size_t                      warning_count;
    size_t                      warning_capacity;
    TypeCheckNotice            *advisories;
    size_t                      advisory_count;
    size_t                      advisory_capacity;
    TypeCheckError              error;
    bool                        has_error;
    /* Non-local return context: the expected return type of the innermost enclosing
       lambda/start whose return type is known; used to type-check |var arguments. */
    CheckedType                 outer_return_type;
    bool                        has_outer_return_type;
    const AstType              *outer_return_ast_type;
    bool                        current_boot_context;
    size_t                      manual_context_depth;
} TypeChecker;

void type_checker_init(TypeChecker *checker);
void type_checker_free(TypeChecker *checker);
bool type_checker_check_program(TypeChecker *checker,
                                const AstProgram *program,
                                const SymbolTable *symbols);

const TypeCheckError *type_checker_get_error(const TypeChecker *checker);
const TypeCheckError *type_checker_get_warning(const TypeChecker *checker);
const TypeCheckError *type_checker_get_warning_at(const TypeChecker *checker,
                                                  size_t index);
size_t type_checker_warning_count(const TypeChecker *checker);
bool type_checker_warning_is_performance(const TypeChecker *checker,
                                         size_t index);
const TypeCheckError *type_checker_get_advisory(const TypeChecker *checker);
const TypeCheckError *type_checker_get_advisory_at(const TypeChecker *checker,
                                                   size_t index);
size_t type_checker_advisory_count(const TypeChecker *checker);
bool type_checker_advisory_is_performance(const TypeChecker *checker,
                                          size_t index);
bool type_checker_format_error(const TypeCheckError *error,
                               char *buffer,
                               size_t buffer_size);
void type_checker_set_global_strict_race_check(bool enabled);
bool type_checker_get_global_strict_race_check(void);
void type_checker_set_global_performance_warnings(bool enabled);
bool type_checker_get_global_performance_warnings(void);
void type_checker_set_global_performance_advisories(bool enabled);
bool type_checker_get_global_performance_advisories(void);
void type_checker_set_global_size_focus(bool enabled);
bool type_checker_get_global_size_focus(void);

const TypeCheckInfo *type_checker_get_expression_info(const TypeChecker *checker,
                                                      const AstExpression *expression);
const TypeCheckInfo *type_checker_get_symbol_info(const TypeChecker *checker,
                                                  const Symbol *symbol);

bool checked_type_to_string(CheckedType type, char *buffer, size_t buffer_size);

#endif /* CALYNDA_TYPE_CHECKER_H */
