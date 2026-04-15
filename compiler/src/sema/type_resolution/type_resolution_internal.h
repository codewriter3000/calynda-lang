#ifndef TYPE_RESOLUTION_INTERNAL_H
#define TYPE_RESOLUTION_INTERNAL_H

#include "type_resolution.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* type_resolution_helpers.c */
const char *tr_primitive_type_name(AstPrimitiveType primitive);
ResolvedType tr_resolved_type_invalid(void);
ResolvedType tr_resolved_type_void(void);
ResolvedType tr_resolved_type_value(AstPrimitiveType primitive, size_t array_depth);
ResolvedType tr_resolved_type_named(const char *name, size_t generic_arg_count,
                                    size_t array_depth);
bool tr_source_span_is_valid(AstSourceSpan span);
void tr_set_error(TypeResolver *resolver, const char *format, ...);
void tr_set_error_at(TypeResolver *resolver,
                     AstSourceSpan primary_span,
                     const AstSourceSpan *related_span,
                     const char *format,
                     ...);
bool tr_append_type_entry(TypeResolver *resolver,
                          const AstType *type_ref,
                          ResolvedType type);
bool tr_append_cast_entry(TypeResolver *resolver,
                          const AstExpression *cast_expression,
                          ResolvedType target_type);

/* type_resolution_resolve.c */
bool tr_resolve_declared_type(TypeResolver *resolver,
                              const AstType *type,
                              AstSourceSpan primary_span,
                              const char *subject_kind,
                              const char *subject_name,
                              bool allow_void);
bool tr_resolve_binding_decl(TypeResolver *resolver, const AstBindingDecl *binding_decl);
bool tr_resolve_start_decl(TypeResolver *resolver, const AstStartDecl *start_decl);
bool tr_resolve_parameter(TypeResolver *resolver, const AstParameter *parameter);
bool tr_resolve_block(TypeResolver *resolver, const AstBlock *block);

/* type_resolution_expr.c */
bool tr_resolve_expression(TypeResolver *resolver, const AstExpression *expression);

#endif
