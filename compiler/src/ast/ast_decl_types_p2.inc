struct AstTopLevelDecl {
    AstTopLevelDeclKind kind;
    union {
        AstStartDecl   start_decl;
        AstBindingDecl binding_decl;
        AstUnionDecl   union_decl;
        AstAsmDecl     asm_decl;
        AstLayoutDecl  layout_decl;
        AstTypeAliasDecl type_alias_decl;
    } as;
};

typedef enum {
    AST_IMPORT_PLAIN = 0,
    AST_IMPORT_ALIAS,
    AST_IMPORT_WILDCARD,
    AST_IMPORT_SELECTIVE
} AstImportKind;

typedef struct {
    AstImportKind    kind;
    AstQualifiedName module_name;
    char            *alias;           /* for AST_IMPORT_ALIAS */
    char           **selected_names;  /* for AST_IMPORT_SELECTIVE */
    size_t           selected_count;
    size_t           selected_capacity;
} AstImportDecl;

typedef struct {
    bool             has_package;
    AstQualifiedName package_name;
    AstImportDecl   *imports;
    size_t           import_count;
    size_t           import_capacity;
    AstTopLevelDecl **top_level_decls;
    size_t           top_level_count;
    size_t           top_level_capacity;
} AstProgram;

