"use strict";
/**
 * Calynda compiler pipeline stages — frontend and middle-end.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.PIPELINE_STAGES_FRONTEND = void 0;
exports.PIPELINE_STAGES_FRONTEND = [
    {
        name: 'Tokenizer',
        dir: 'compiler/src/tokenizer/',
        description: 'Lexes source text into a flat token stream. Tracks nested braces inside template interpolations so expressions like lambdas with block bodies parse correctly inside templates.',
        keyTypes: ['Token', 'Tokenizer'],
        keyFunctions: ['tokenizer_init', 'tokenizer_next_token', 'tokenizer_current_token'],
        files: [
            'tokenizer.h', 'tokenizer.c', 'tokenizer_internal.h',
            'tokenizer_keywords.c', 'tokenizer_punctuation.c', 'tokenizer_literals.c',
        ],
    },
    {
        name: 'Parser',
        dir: 'compiler/src/parser/',
        description: 'Recursive-descent parser with token pre-loading for lookahead. Parses the full EBNF grammar into an AST. Handles named types, generic arguments, >> splitting for nested generics, and all V2 surface features.',
        keyTypes: ['Parser', 'AstProgram'],
        keyFunctions: ['parser_parse_program', 'parser_parse_expression'],
        files: [
            'parser.h', 'parser.c', 'parser_internal.h',
            'parser_utils.c', 'parser_types.c', 'parser_decl.c', 'parser_union.c',
            'parser_stmt.c', 'parser_expr.c', 'parser_binary.c', 'parser_postfix.c',
            'parser_literals.c', 'parser_lookahead.c',
        ],
    },
    {
        name: 'AST',
        dir: 'compiler/src/ast/',
        description: 'Abstract syntax tree node definitions and creation helpers. Declarations, expressions, statements, and types all carry source spans for diagnostic reporting. AST dump utilities provide both FILE-based and string-returning APIs.',
        keyTypes: [
            'AstProgram', 'AstTopLevelDecl', 'AstExpression', 'AstStatement', 'AstType',
            'AstStartDecl', 'AstBindingDecl', 'AstUnionDecl',
        ],
        keyFunctions: ['ast_dump_program', 'ast_dump_program_to_string', 'ast_dump_expression_to_string'],
        files: [
            'ast.h', 'ast.c', 'ast_types.h', 'ast_decl_types.h', 'ast_internal.h',
            'ast_expressions.c', 'ast_statements.c', 'ast_declarations.c',
            'ast_dump.h', 'ast_dump.c', 'ast_dump_internal.h',
            'ast_dump_names.c', 'ast_dump_types.c', 'ast_dump_expr.c',
            'ast_dump_stmt.c', 'ast_dump_decl.c',
        ],
    },
    {
        name: 'Symbol Table',
        dir: 'compiler/src/sema/',
        description: 'Builds a program scope plus nested start/lambda/block scopes. Tracks package/import/top-level/parameter/local symbols, records identifier resolutions, and reports unresolved names. Symbols carry is_exported, is_static, is_internal flags. Symbol kinds include SYMBOL_KIND_UNION and SYMBOL_KIND_TYPE_PARAMETER.',
        keyTypes: ['SymbolTable', 'Symbol', 'Scope', 'SymbolResolution'],
        keyFunctions: [
            'symbol_table_build', 'symbol_table_find_import',
            'symbol_table_root_scope', 'symbol_table_find_resolution',
        ],
        files: [
            'symbol_table.h', 'symbol_table.c', 'symbol_table_internal.h',
            'symbol_table_query.c', 'symbol_table_core.c', 'symbol_table_registry.c',
            'symbol_table_imports.c', 'symbol_table_predecl.c',
            'symbol_table_analyze.c', 'symbol_table_analyze_expr.c',
        ],
    },
    {
        name: 'Type Resolution',
        dir: 'compiler/src/sema/',
        description: 'Resolves declared source types for bindings, parameters, locals, and casts. Validates array dimensions and rejects invalid void usage before type checking.',
        keyTypes: ['TypeResolution'],
        keyFunctions: ['type_resolution_resolve'],
        files: [
            'type_resolution.h', 'type_resolution.c', 'type_resolution_internal.h',
            'type_resolution_helpers.c', 'type_resolution_resolve.c', 'type_resolution_expr.c',
        ],
    },
    {
        name: 'Type Checker',
        dir: 'compiler/src/sema/',
        description: 'Infers expression and symbol types. Preserves callable metadata for lambdas/bindings. Validates operators, calls, assignments, arrays, casts, and start return typing. Enforces exactly one start, start(string[] args) signature, exit-in-void-lambda, contextual return errors, internal visibility, and l-value assignment rules.',
        keyTypes: ['TypeChecker', 'CheckedType', 'TypeCheckInfo'],
        keyFunctions: [
            'type_checker_check_program', 'type_checker_get_expression_info',
            'checked_type_to_string',
        ],
        files: [
            'type_checker.h', 'type_checker.c', 'type_checker_internal.h',
            'type_checker_check.c', 'type_checker_names.c', 'type_checker_types.c',
            'type_checker_convert.c', 'type_checker_util.c', 'type_checker_resolve.c',
            'type_checker_lambda.c', 'type_checker_block.c', 'type_checker_ops.c',
            'type_checker_expr.c', 'type_checker_expr_ext.c', 'type_checker_expr_more.c',
        ],
    },
    {
        name: 'Semantic Dump',
        dir: 'compiler/src/sema/',
        description: 'Renders package/imports, hierarchical scopes, declared and inferred types, identifier resolution edges, unresolved names, and explicit shadowing notes.',
        keyTypes: [],
        keyFunctions: ['semantic_dump_program'],
        files: [
            'semantic_dump.h', 'semantic_dump.c', 'semantic_dump_internal.h',
            'semantic_dump_builder.c', 'semantic_dump_types.c',
            'semantic_dump_scope.c', 'semantic_dump_symbols.c',
        ],
    },
    {
        name: 'HIR (High-level IR)',
        dir: 'compiler/src/hir/',
        description: 'Lowers typed AST + semantic state into an owned, typed IR. Normalizes expression-bodied lambdas and start bodies into block form, erases grouping expressions, normalizes exit; into return;, and owns callable signatures. Local bindings preserve callable metadata.',
        keyTypes: ['HirProgram', 'HirExpression', 'HirStatement', 'HirBindingDecl'],
        keyFunctions: ['hir_build_program', 'hir_dump_program'],
        files: [
            'hir.h', 'hir_types.h', 'hir_expr_types.h', 'hir_internal.h',
            'hir.c', 'hir_memory.c', 'hir_helpers.c',
            'hir_lower.c', 'hir_lower_stmt.c', 'hir_lower_expr.c',
            'hir_lower_expr_ext.c', 'hir_lower_decls.c',
            'hir_dump.h', 'hir_dump.c', 'hir_dump_internal.h',
            'hir_dump_helpers.c', 'hir_dump_expr.c', 'hir_dump_expr_ext.c',
        ],
    },
    {
        name: 'MIR (Mid-level IR)',
        dir: 'compiler/src/mir/',
        description: 'Machine-independent IR with explicit callable units, locals, temporaries, basic blocks, branch/goto/return/throw terminators. Lowers short-circuit &&/|| into branch/join blocks, ternary into merge locals, closures into MIR_UNIT_LAMBDA + MIR_INSTR_CLOSURE, and top-level bindings into __mir_module_init.',
        keyTypes: [
            'MirProgram', 'MirUnit', 'MirBasicBlock', 'MirInstruction',
            'MirValue', 'MirTerminator',
        ],
        keyFunctions: ['mir_build_program', 'mir_dump_program', 'mir_dump_program_to_string'],
        files: [
            'mir.h', 'mir_instr_types.h', 'mir_internal.h',
            'mir.c', 'mir_build.c', 'mir_lower.c', 'mir_start.c', 'mir_expr.c',
            'mir_expr_call.c', 'mir_expr_helpers.c', 'mir_union.c',
            'mir_capture.c', 'mir_capture_analysis.c', 'mir_capture_analysis_expr.c',
            'mir_value.c', 'mir_store.c', 'mir_lvalue.c', 'mir_assign.c',
            'mir_lambda.c', 'mir_control.c', 'mir_cleanup.c', 'mir_builders.c',
            'mir_dump.c', 'mir_dump_helpers.c', 'mir_dump_instr.c',
        ],
    },
];
//# sourceMappingURL=architecture-stages.js.map