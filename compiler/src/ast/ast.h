#ifndef CALYNDA_AST_H
#define CALYNDA_AST_H

#include "ast_decl_types.h"

char *ast_copy_text(const char *text);
char *ast_copy_text_n(const char *text, size_t length);

void ast_qualified_name_init(AstQualifiedName *name);
void ast_qualified_name_free(AstQualifiedName *name);
bool ast_qualified_name_append(AstQualifiedName *name, const char *segment);

void ast_type_init_void(AstType *type);
void ast_type_init_primitive(AstType *type, AstPrimitiveType primitive);
void ast_type_init_arr(AstType *type);
void ast_type_init_named(AstType *type, const char *name);
void ast_type_init_thread(AstType *type);
void ast_type_init_mutex(AstType *type);
void ast_type_init_future(AstType *type);
void ast_type_init_atomic(AstType *type);
void ast_type_free(AstType *type);
bool ast_type_add_dimension(AstType *type, bool has_size, const char *size_literal);
bool ast_type_add_generic_arg(AstType *type, AstGenericArg *arg);
void ast_generic_arg_list_free(AstGenericArgList *list);

void ast_parameter_list_init(AstParameterList *list);
void ast_parameter_list_free(AstParameterList *list);
bool ast_parameter_list_append(AstParameterList *list, AstParameter *parameter);

AstBlock *ast_block_new(void);
void ast_block_free(AstBlock *block);
bool ast_block_append_statement(AstBlock *block, AstStatement *statement);

AstExpression *ast_expression_new(AstExpressionKind kind);
void ast_expression_free(AstExpression *expression);
bool ast_expression_list_append(AstExpressionList *list, AstExpression *expression);
bool ast_template_literal_append_text(AstLiteral *literal, const char *text);
bool ast_template_literal_append_expression(AstLiteral *literal,
                                            AstExpression *expression);

AstStatement *ast_statement_new(AstStatementKind kind);
void ast_statement_free(AstStatement *statement);

AstTopLevelDecl *ast_top_level_decl_new(AstTopLevelDeclKind kind);
void ast_top_level_decl_free(AstTopLevelDecl *decl);
bool ast_binding_decl_add_modifier(AstBindingDecl *decl, AstModifier modifier);
bool ast_asm_decl_add_modifier(AstAsmDecl *decl, AstModifier modifier);
bool ast_union_decl_add_modifier(AstUnionDecl *decl, AstModifier modifier);
bool ast_type_alias_decl_add_modifier(AstTypeAliasDecl *decl, AstModifier modifier);
bool ast_decl_has_modifier(const AstModifier *modifiers, size_t count,
                           AstModifier modifier);

void ast_program_init(AstProgram *program);
void ast_program_free(AstProgram *program);
bool ast_program_set_package(AstProgram *program, AstQualifiedName *package_name);
bool ast_program_add_import(AstProgram *program, AstImportDecl *import_decl);
bool ast_program_add_top_level_decl(AstProgram *program, AstTopLevelDecl *decl);

void ast_import_decl_init(AstImportDecl *decl);
void ast_import_decl_free(AstImportDecl *decl);
bool ast_import_decl_add_selected(AstImportDecl *decl, const char *name);

void ast_union_decl_free_fields(AstUnionDecl *decl);
bool ast_union_decl_add_generic_param(AstUnionDecl *decl, const char *param);
bool ast_union_decl_add_variant(AstUnionDecl *decl, AstUnionVariant *variant);

#endif /* CALYNDA_AST_H */
