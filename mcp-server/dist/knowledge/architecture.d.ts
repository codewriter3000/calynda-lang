/**
 * Calynda compiler architecture knowledge base.
 *
 * Describes the full compilation pipeline, source-tree layout,
 * key data structures per stage, build targets, and CLI tools.
 */
export declare const PIPELINE_STAGES: readonly [{
    readonly name: "Tokenizer";
    readonly dir: "compiler/src/tokenizer/";
    readonly description: "Lexes source text into a flat token stream. Tracks nested braces inside template interpolations so expressions like lambdas with block bodies parse correctly inside templates.";
    readonly keyTypes: readonly ["Token", "Tokenizer"];
    readonly keyFunctions: readonly ["tokenizer_init", "tokenizer_next_token", "tokenizer_current_token"];
    readonly files: readonly ["tokenizer.h", "tokenizer.c", "tokenizer_internal.h", "tokenizer_keywords.c", "tokenizer_punctuation.c", "tokenizer_literals.c"];
}, {
    readonly name: "Parser";
    readonly dir: "compiler/src/parser/";
    readonly description: "Recursive-descent parser with token pre-loading for lookahead. Parses the full EBNF grammar into an AST. Handles named types, generic arguments, >> splitting for nested generics, and all V2 surface features.";
    readonly keyTypes: readonly ["Parser", "AstProgram"];
    readonly keyFunctions: readonly ["parser_parse_program", "parser_parse_expression"];
    readonly files: readonly ["parser.h", "parser.c", "parser_internal.h", "parser_utils.c", "parser_types.c", "parser_decl.c", "parser_union.c", "parser_stmt.c", "parser_expr.c", "parser_binary.c", "parser_postfix.c", "parser_literals.c", "parser_lookahead.c"];
}, {
    readonly name: "AST";
    readonly dir: "compiler/src/ast/";
    readonly description: "Abstract syntax tree node definitions and creation helpers. Declarations, expressions, statements, and types all carry source spans for diagnostic reporting. AST dump utilities provide both FILE-based and string-returning APIs.";
    readonly keyTypes: readonly ["AstProgram", "AstTopLevelDecl", "AstExpression", "AstStatement", "AstType", "AstStartDecl", "AstBindingDecl", "AstUnionDecl"];
    readonly keyFunctions: readonly ["ast_dump_program", "ast_dump_program_to_string", "ast_dump_expression_to_string"];
    readonly files: readonly ["ast.h", "ast.c", "ast_types.h", "ast_decl_types.h", "ast_internal.h", "ast_expressions.c", "ast_statements.c", "ast_declarations.c", "ast_dump.h", "ast_dump.c", "ast_dump_internal.h", "ast_dump_names.c", "ast_dump_types.c", "ast_dump_expr.c", "ast_dump_stmt.c", "ast_dump_decl.c"];
}, {
    readonly name: "Symbol Table";
    readonly dir: "compiler/src/sema/";
    readonly description: "Builds a program scope plus nested start/lambda/block scopes. Tracks package/import/top-level/parameter/local symbols, records identifier resolutions, and reports unresolved names. Symbols carry is_exported, is_static, is_internal flags. Symbol kinds include SYMBOL_KIND_UNION and SYMBOL_KIND_TYPE_PARAMETER.";
    readonly keyTypes: readonly ["SymbolTable", "Symbol", "Scope", "SymbolResolution"];
    readonly keyFunctions: readonly ["symbol_table_build", "symbol_table_find_import", "symbol_table_root_scope", "symbol_table_find_resolution"];
    readonly files: readonly ["symbol_table.h", "symbol_table.c", "symbol_table_internal.h", "symbol_table_query.c", "symbol_table_core.c", "symbol_table_registry.c", "symbol_table_imports.c", "symbol_table_predecl.c", "symbol_table_analyze.c", "symbol_table_analyze_expr.c"];
}, {
    readonly name: "Type Resolution";
    readonly dir: "compiler/src/sema/";
    readonly description: "Resolves declared source types for bindings, parameters, locals, and casts. Validates array dimensions and rejects invalid void usage before type checking.";
    readonly keyTypes: readonly ["TypeResolution"];
    readonly keyFunctions: readonly ["type_resolution_resolve"];
    readonly files: readonly ["type_resolution.h", "type_resolution.c", "type_resolution_internal.h", "type_resolution_helpers.c", "type_resolution_resolve.c", "type_resolution_expr.c"];
}, {
    readonly name: "Type Checker";
    readonly dir: "compiler/src/sema/";
    readonly description: "Infers expression and symbol types. Preserves callable metadata for lambdas/bindings. Validates operators, calls, assignments, arrays, casts, and start return typing. Enforces exactly one start, start(string[] args) signature, exit-in-void-lambda, contextual return errors, internal visibility, and l-value assignment rules.";
    readonly keyTypes: readonly ["TypeChecker", "CheckedType", "TypeCheckInfo"];
    readonly keyFunctions: readonly ["type_checker_check_program", "type_checker_get_expression_info", "checked_type_to_string"];
    readonly files: readonly ["type_checker.h", "type_checker.c", "type_checker_internal.h", "type_checker_check.c", "type_checker_names.c", "type_checker_types.c", "type_checker_convert.c", "type_checker_util.c", "type_checker_resolve.c", "type_checker_lambda.c", "type_checker_block.c", "type_checker_ops.c", "type_checker_expr.c", "type_checker_expr_ext.c", "type_checker_expr_more.c"];
}, {
    readonly name: "Semantic Dump";
    readonly dir: "compiler/src/sema/";
    readonly description: "Renders package/imports, hierarchical scopes, declared and inferred types, identifier resolution edges, unresolved names, and explicit shadowing notes.";
    readonly keyTypes: readonly [];
    readonly keyFunctions: readonly ["semantic_dump_program"];
    readonly files: readonly ["semantic_dump.h", "semantic_dump.c", "semantic_dump_internal.h", "semantic_dump_builder.c", "semantic_dump_types.c", "semantic_dump_scope.c", "semantic_dump_symbols.c"];
}, {
    readonly name: "HIR (High-level IR)";
    readonly dir: "compiler/src/hir/";
    readonly description: "Lowers typed AST + semantic state into an owned, typed IR. Normalizes expression-bodied lambdas and start bodies into block form, erases grouping expressions, normalizes exit; into return;, and owns callable signatures. Local bindings preserve callable metadata.";
    readonly keyTypes: readonly ["HirProgram", "HirExpression", "HirStatement", "HirBindingDecl"];
    readonly keyFunctions: readonly ["hir_build_program", "hir_dump_program"];
    readonly files: readonly ["hir.h", "hir_types.h", "hir_expr_types.h", "hir_internal.h", "hir.c", "hir_memory.c", "hir_helpers.c", "hir_lower.c", "hir_lower_stmt.c", "hir_lower_expr.c", "hir_lower_expr_ext.c", "hir_lower_decls.c", "hir_dump.h", "hir_dump.c", "hir_dump_internal.h", "hir_dump_helpers.c", "hir_dump_expr.c", "hir_dump_expr_ext.c"];
}, {
    readonly name: "MIR (Mid-level IR)";
    readonly dir: "compiler/src/mir/";
    readonly description: "Machine-independent IR with explicit callable units, locals, temporaries, basic blocks, branch/goto/return/throw terminators. Lowers short-circuit &&/|| into branch/join blocks, ternary into merge locals, closures into MIR_UNIT_LAMBDA + MIR_INSTR_CLOSURE, and top-level bindings into __mir_module_init.";
    readonly keyTypes: readonly ["MirProgram", "MirUnit", "MirBasicBlock", "MirInstruction", "MirValue", "MirTerminator"];
    readonly keyFunctions: readonly ["mir_build_program", "mir_dump_program", "mir_dump_program_to_string"];
    readonly files: readonly ["mir.h", "mir_instr_types.h", "mir_internal.h", "mir.c", "mir_lower.c", "mir_expr.c", "mir_union.c", "mir_capture.c", "mir_capture_analysis.c", "mir_value.c", "mir_store.c", "mir_lvalue.c", "mir_assign.c", "mir_lambda.c", "mir_control.c", "mir_cleanup.c", "mir_expr_helpers.c", "mir_builders.c", "mir_dump.c", "mir_dump_helpers.c", "mir_dump_instr.c"];
}, {
    readonly name: "LIR (Low-level IR)";
    readonly dir: "compiler/src/lir/";
    readonly description: "Target-aware IR with stack slots, virtual registers, explicit incoming capture/argument moves, explicit outgoing call-argument setup, and block terminators.";
    readonly keyTypes: readonly ["LirProgram", "LirUnit", "LirInstruction", "LirOperand", "LirSlot"];
    readonly keyFunctions: readonly ["lir_build_program", "lir_dump_program"];
    readonly files: readonly ["lir.h", "lir_instr_types.h", "lir_internal.h", "lir.c", "lir_memory.c", "lir_helpers.c", "lir_lower.c", "lir_lower_instr.c", "lir_lower_instr_ext.c", "lir_lower_instr_stores.c", "lir_dump.c", "lir_dump_helpers.c", "lir_dump_instr.c"];
}, {
    readonly name: "Codegen Planning";
    readonly dir: "compiler/src/codegen/";
    readonly description: "x86_64 SysV ELF target with register allocation. Classifies LIR ops into direct machine-pattern candidates vs runtime helpers. Allocatable vreg set: r10/r11/r12/r13 before spilling.";
    readonly keyTypes: readonly ["CodegenProgram", "CodegenTargetKind", "CodegenRegister", "CodegenVRegAllocation", "CodegenDirectPattern"];
    readonly keyFunctions: readonly ["codegen_build_program"];
    readonly files: readonly ["codegen.h", "codegen_internal.h", "codegen.c", "codegen_names.c", "codegen_helpers.c", "codegen_infer.c", "codegen_build.c", "codegen_dump.c", "codegen_dump_helpers.c"];
}, {
    readonly name: "Machine Emission";
    readonly dir: "compiler/src/backend/";
    readonly description: "Consumes LIR + codegen plan to emit x86_64 SysV instruction streams. Handles helper-call setup, prologue/epilogue, and conservative register preservation around calls. Emits inc/dec for pre-increment/decrement.";
    readonly keyTypes: readonly ["MachineProgram", "MachineUnit", "MachineInstruction", "MachineBlock"];
    readonly keyFunctions: readonly ["machine_build_program", "machine_dump_program"];
    readonly files: readonly ["machine.h", "machine.c", "machine_internal.h", "machine_dump.c", "machine_helpers.c", "machine_layout.c", "machine_operand.c", "machine_emit.c", "machine_ops.c", "machine_instr.c", "machine_instr_rt.c", "machine_instr_rt_ext.c", "machine_build.c"];
}, {
    readonly name: "Assembly Emission";
    readonly dir: "compiler/src/backend/";
    readonly description: "Lowers MachineProgram into GNU assembler x86_64 text with concrete stack addresses, rodata literals, global storage, string-object data, runtime helper calls, main entry glue, and .note.GNU-stack.";
    readonly keyTypes: readonly [];
    readonly keyFunctions: readonly ["asm_emit_program", "asm_emit_program_to_string"];
    readonly files: readonly ["asm_emit.h", "asm_emit.c", "asm_emit_internal.h", "asm_emit_helpers.c", "asm_emit_symbols.c", "asm_emit_operand.c", "asm_emit_operand_ext.c", "asm_emit_instr.c", "asm_emit_sections.c", "asm_emit_entry.c"];
}, {
    readonly name: "Runtime ABI";
    readonly dir: "compiler/src/backend/";
    readonly description: "Defines the x86_64 SysV helper-call surface for closures, callable dispatch, member/index access, arrays, templates, dynamic casts, throw, and capture-environment contract in r15.";
    readonly keyTypes: readonly ["RuntimeAbiHelperSignature"];
    readonly keyFunctions: readonly ["runtime_abi_get_helper_signature", "runtime_abi_get_helper_argument_register"];
    readonly files: readonly ["runtime_abi.h", "runtime_abi.c", "runtime_abi_names.c", "runtime_abi_dump.c"];
}, {
    readonly name: "Runtime";
    readonly dir: "compiler/src/runtime/";
    readonly description: "Concrete runtime objects and values. Values are raw machine words or registered heap-object handles. Object kinds: strings, arrays, closures, packages, extern callables, template-part packs, and unions. Includes process startup that boxes argv into Calynda string[] and registers static string objects.";
    readonly keyTypes: readonly ["CalyndaRtWord", "CalyndaRtString", "CalyndaRtArray", "CalyndaRtClosure", "CalyndaRtTypeTag", "CalyndaRtTypeDescriptor"];
    readonly keyFunctions: readonly ["calynda_rt_start_process"];
    readonly files: readonly ["runtime.h", "runtime.c", "runtime_internal.h", "runtime_names.c", "runtime_format.c", "runtime_ops.c", "runtime_union.c"];
}, {
    readonly name: "Bytecode Backend";
    readonly dir: "compiler/src/bytecode/";
    readonly description: "Portable-v1 bytecode backend. Lowers MIR directly into BytecodeProgram structures with a constant pool, bytecode units, basic blocks, and explicit instructions + terminators. Mirrors the MIR opcode surface.";
    readonly keyTypes: readonly ["BytecodeProgram", "BytecodeUnit", "BytecodeInstruction", "BytecodeValue", "BytecodeConstant"];
    readonly keyFunctions: readonly ["bytecode_build_program", "bytecode_dump_program", "bytecode_target_name"];
    readonly files: readonly ["bytecode.h", "bytecode_instr_types.h", "bytecode_internal.h", "bytecode.c", "bytecode_memory.c", "bytecode_helpers.c", "bytecode_constants.c", "bytecode_lower.c", "bytecode_lower_instr.c", "bytecode_dump.c", "bytecode_dump_helpers.c", "bytecode_dump_instr.c", "bytecode_dump_names.c"];
}, {
    readonly name: "CLI";
    readonly dir: "compiler/src/cli/";
    readonly description: "Command-line tools: the calynda compiler driver, AST dumper, semantic dumper, assembly emitter, bytecode emitter, and native builder. The native builder resolves runtime.o relative to the executable directory.";
    readonly keyTypes: readonly [];
    readonly keyFunctions: readonly [];
    readonly files: readonly ["calynda.c", "calynda_internal.h", "calynda_compile.c", "calynda_utils.c", "dump_ast.c", "dump_semantics.c", "emit_asm.c", "emit_bytecode.c", "build_native.c", "build_native_utils.c"];
}];
export declare const BUILD_TARGETS: {
    mainBinaries: {
        name: string;
        description: string;
    }[];
    debugTools: {
        name: string;
        description: string;
    }[];
    testSuites: string[];
    buildCommand: string;
    testRunner: string;
};
export declare const NATIVE_PIPELINE = "Source \u2192 Tokenizer \u2192 Parser \u2192 AST \u2192 SymbolTable \u2192 TypeResolution \u2192 TypeChecker \u2192 HIR \u2192 MIR \u2192 LIR \u2192 Codegen \u2192 Machine \u2192 AsmEmit \u2192 gcc link \u2192 executable";
export declare const BYTECODE_PIPELINE = "Source \u2192 Tokenizer \u2192 Parser \u2192 AST \u2192 SymbolTable \u2192 TypeResolution \u2192 TypeChecker \u2192 HIR \u2192 MIR \u2192 BytecodeProgram (portable-v1)";
export declare const BACKEND_STRATEGY = "Calynda has two compiler backends and rejects any interpreter path:\n\nPrimary backend \u2014 native x86_64 SysV ELF:\n  Source \u2192 Tokenizer \u2192 Parser \u2192 AST \u2192 SymbolTable \u2192 TypeResolution \u2192 TypeChecker \u2192 HIR \u2192 MIR \u2192 LIR \u2192 Codegen \u2192 Machine \u2192 AsmEmit \u2192 gcc link \u2192 executable\n\nSecondary backend \u2014 portable-v1 bytecode:\n  Source \u2192 Tokenizer \u2192 Parser \u2192 AST \u2192 SymbolTable \u2192 TypeResolution \u2192 TypeChecker \u2192 HIR \u2192 MIR \u2192 BytecodeProgram (portable-v1)\n\nMIR is the split point because it already has explicit control flow, closures, globals, calls, and throw.\n\nThe runtime contract is shared backend infrastructure:\n  - Native assembly calls runtime helpers directly and emits startup wrappers.\n  - Bytecode targets the same language-level operations.\n  - Both backends share the same semantic boundary for dynamic features.";
export declare const SOURCE_TREE = "compiler/\n  calynda.ebnf          \u2014 Canonical V1 EBNF grammar\n  calynda_v2.ebnf       \u2014 V2 grammar sandbox (superset of V1)\n  Makefile              \u2014 Build system\n  run_tests.sh          \u2014 Test runner (make clean && make test)\n  src/\n    tokenizer/          \u2014 Lexer (6 files)\n    parser/             \u2014 Recursive-descent parser (12 files)\n    ast/                \u2014 AST nodes + dump (16 files)\n    sema/               \u2014 Symbol table, type resolution, type checker, semantic dump (27 files)\n    hir/                \u2014 High-level IR (14 files)\n    mir/                \u2014 Mid-level IR (22 files)\n    lir/                \u2014 Low-level IR (11 files)\n    bytecode/           \u2014 Portable-v1 bytecode backend (13 files)\n    codegen/            \u2014 x86_64 register allocation + instruction selection (8 files)\n    backend/            \u2014 Machine emission, asm emission, runtime ABI (34 files)\n    runtime/            \u2014 Runtime objects + helpers (6 files)\n    cli/                \u2014 Driver + diagnostic tools (10 files)\n  tests/                \u2014 18 test suites (~1147 tests total)\n  build/                \u2014 Build output directory";
export declare const ERROR_PATTERN = "Each compiler stage uses the same error struct pattern:\n\n  struct XyzBuildError {\n      AstSourceSpan primary_span;\n      AstSourceSpan related_span;\n      bool has_related_span;\n      char message[256];\n  };\n\n  struct XyzProgram {\n      /* ... members ... */\n      XyzBuildError error;\n      bool has_error;\n  };\n\nSource spans carry line/column for precise diagnostic reporting.";
