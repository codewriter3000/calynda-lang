#ifndef CALYNDA_TYPE_RESOLUTION_H
#define CALYNDA_TYPE_RESOLUTION_H

#include "ast.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    RESOLVED_TYPE_INVALID = 0,
    RESOLVED_TYPE_VOID,
    RESOLVED_TYPE_VALUE,
    RESOLVED_TYPE_NAMED
} ResolvedTypeKind;

typedef struct {
    ResolvedTypeKind kind;
    AstPrimitiveType primitive;
    size_t           array_depth;
    const char      *name;             /* for RESOLVED_TYPE_NAMED */
    size_t           generic_arg_count; /* for RESOLVED_TYPE_NAMED */
} ResolvedType;

typedef struct {
    const AstType *type_ref;
    ResolvedType   type;
} ResolvedTypeEntry;

typedef struct {
    const AstExpression *cast_expression;
    ResolvedType         target_type;
} ResolvedCastTypeEntry;

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} TypeResolutionError;

typedef struct {
    const AstProgram      *program;
    ResolvedTypeEntry     *type_entries;
    size_t                 type_count;
    size_t                 type_capacity;
    ResolvedCastTypeEntry *cast_entries;
    size_t                 cast_count;
    size_t                 cast_capacity;
    TypeResolutionError    error;
    bool                   has_error;
} TypeResolver;

void type_resolver_init(TypeResolver *resolver);
void type_resolver_free(TypeResolver *resolver);
bool type_resolver_resolve_program(TypeResolver *resolver, const AstProgram *program);

const TypeResolutionError *type_resolver_get_error(const TypeResolver *resolver);
bool type_resolver_format_error(const TypeResolutionError *error,
                                char *buffer,
                                size_t buffer_size);

const ResolvedType *type_resolver_get_type(const TypeResolver *resolver,
                                           const AstType *type_ref);
const ResolvedType *type_resolver_get_cast_target_type(const TypeResolver *resolver,
                                                       const AstExpression *cast_expression);
bool resolved_type_to_string(ResolvedType type, char *buffer, size_t buffer_size);

#endif /* CALYNDA_TYPE_RESOLUTION_H */