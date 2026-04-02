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
    CHECKED_TYPE_EXTERNAL
} CheckedTypeKind;

typedef struct {
    CheckedTypeKind  kind;
    AstPrimitiveType primitive;
    size_t           array_depth;
} CheckedType;

typedef struct {
    CheckedType             type;
    bool                    is_callable;
    CheckedType             callable_return_type;
    const AstParameterList *parameters;
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
    const AstProgram           *program;
    const SymbolTable          *symbols;
    TypeResolver                resolver;
    TypeCheckExpressionEntry   *expression_entries;
    size_t                      expression_count;
    size_t                      expression_capacity;
    TypeCheckSymbolEntry       *symbol_entries;
    size_t                      symbol_count;
    size_t                      symbol_capacity;
    TypeCheckError              error;
    bool                        has_error;
} TypeChecker;

void type_checker_init(TypeChecker *checker);
void type_checker_free(TypeChecker *checker);
bool type_checker_check_program(TypeChecker *checker,
                                const AstProgram *program,
                                const SymbolTable *symbols);

const TypeCheckError *type_checker_get_error(const TypeChecker *checker);
bool type_checker_format_error(const TypeCheckError *error,
                               char *buffer,
                               size_t buffer_size);

const TypeCheckInfo *type_checker_get_expression_info(const TypeChecker *checker,
                                                      const AstExpression *expression);
const TypeCheckInfo *type_checker_get_symbol_info(const TypeChecker *checker,
                                                  const Symbol *symbol);

bool checked_type_to_string(CheckedType type, char *buffer, size_t buffer_size);

#endif /* CALYNDA_TYPE_CHECKER_H */