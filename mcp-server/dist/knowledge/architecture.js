"use strict";
/**
 * Calynda compiler architecture knowledge base.
 *
 * Describes the full compilation pipeline, source-tree layout,
 * key data structures per stage, build targets, and CLI tools.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.ERROR_PATTERN = exports.SOURCE_TREE = exports.BACKEND_STRATEGY = exports.BYTECODE_PIPELINE = exports.NATIVE_PIPELINE = exports.BUILD_TARGETS = exports.PIPELINE_STAGES = void 0;
// ---------------------------------------------------------------------------
// Pipeline overview
// ---------------------------------------------------------------------------
exports.PIPELINE_STAGES = [
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
            'mir.c', 'mir_lower.c', 'mir_expr.c', 'mir_union.c',
            'mir_capture.c', 'mir_capture_analysis.c', 'mir_value.c',
            'mir_store.c', 'mir_lvalue.c', 'mir_assign.c', 'mir_lambda.c',
            'mir_control.c', 'mir_cleanup.c', 'mir_expr_helpers.c', 'mir_builders.c',
            'mir_dump.c', 'mir_dump_helpers.c', 'mir_dump_instr.c',
        ],
    },
    {
        name: 'LIR (Low-level IR)',
        dir: 'compiler/src/lir/',
        description: 'Target-aware IR with stack slots, virtual registers, explicit incoming capture/argument moves, explicit outgoing call-argument setup, and block terminators.',
        keyTypes: ['LirProgram', 'LirUnit', 'LirInstruction', 'LirOperand', 'LirSlot'],
        keyFunctions: ['lir_build_program', 'lir_dump_program'],
        files: [
            'lir.h', 'lir_instr_types.h', 'lir_internal.h',
            'lir.c', 'lir_memory.c', 'lir_helpers.c',
            'lir_lower.c', 'lir_lower_instr.c', 'lir_lower_instr_ext.c', 'lir_lower_instr_stores.c',
            'lir_dump.c', 'lir_dump_helpers.c', 'lir_dump_instr.c',
        ],
    },
    {
        name: 'Codegen Planning',
        dir: 'compiler/src/codegen/',
        description: 'x86_64 SysV ELF target with register allocation. Classifies LIR ops into direct machine-pattern candidates vs runtime helpers. Allocatable vreg set: r10/r11/r12/r13 before spilling.',
        keyTypes: [
            'CodegenProgram', 'CodegenTargetKind', 'CodegenRegister',
            'CodegenVRegAllocation', 'CodegenDirectPattern',
        ],
        keyFunctions: ['codegen_build_program'],
        files: [
            'codegen.h', 'codegen_internal.h',
            'codegen.c', 'codegen_names.c', 'codegen_helpers.c',
            'codegen_infer.c', 'codegen_build.c',
            'codegen_dump.c', 'codegen_dump_helpers.c',
        ],
    },
    {
        name: 'Machine Emission',
        dir: 'compiler/src/backend/',
        description: 'Consumes LIR + codegen plan to emit x86_64 SysV instruction streams. Handles helper-call setup, prologue/epilogue, and conservative register preservation around calls. Emits inc/dec for pre-increment/decrement.',
        keyTypes: ['MachineProgram', 'MachineUnit', 'MachineInstruction', 'MachineBlock'],
        keyFunctions: ['machine_build_program', 'machine_dump_program'],
        files: [
            'machine.h', 'machine.c', 'machine_internal.h',
            'machine_dump.c', 'machine_helpers.c', 'machine_layout.c',
            'machine_operand.c', 'machine_emit.c', 'machine_ops.c',
            'machine_instr.c', 'machine_instr_rt.c', 'machine_instr_rt_ext.c', 'machine_build.c',
        ],
    },
    {
        name: 'Assembly Emission',
        dir: 'compiler/src/backend/',
        description: 'Lowers MachineProgram into GNU assembler x86_64 text with concrete stack addresses, rodata literals, global storage, string-object data, runtime helper calls, main entry glue, and .note.GNU-stack.',
        keyTypes: [],
        keyFunctions: ['asm_emit_program', 'asm_emit_program_to_string'],
        files: [
            'asm_emit.h', 'asm_emit.c', 'asm_emit_internal.h',
            'asm_emit_helpers.c', 'asm_emit_symbols.c',
            'asm_emit_operand.c', 'asm_emit_operand_ext.c',
            'asm_emit_instr.c', 'asm_emit_sections.c', 'asm_emit_entry.c',
        ],
    },
    {
        name: 'Runtime ABI',
        dir: 'compiler/src/backend/',
        description: 'Defines the x86_64 SysV helper-call surface for closures, callable dispatch, member/index access, arrays, templates, dynamic casts, throw, and capture-environment contract in r15.',
        keyTypes: ['RuntimeAbiHelperSignature'],
        keyFunctions: ['runtime_abi_get_helper_signature', 'runtime_abi_get_helper_argument_register'],
        files: ['runtime_abi.h', 'runtime_abi.c', 'runtime_abi_names.c', 'runtime_abi_dump.c'],
    },
    {
        name: 'Runtime',
        dir: 'compiler/src/runtime/',
        description: 'Concrete runtime objects and values. Values are raw machine words or registered heap-object handles. Object kinds: strings, arrays, closures, packages, extern callables, template-part packs, and unions. Includes process startup that boxes argv into Calynda string[] and registers static string objects.',
        keyTypes: [
            'CalyndaRtWord', 'CalyndaRtString', 'CalyndaRtArray',
            'CalyndaRtClosure', 'CalyndaRtTypeTag', 'CalyndaRtTypeDescriptor',
        ],
        keyFunctions: ['calynda_rt_start_process'],
        files: [
            'runtime.h', 'runtime.c', 'runtime_internal.h',
            'runtime_names.c', 'runtime_format.c', 'runtime_ops.c', 'runtime_union.c',
        ],
    },
    {
        name: 'Bytecode Backend',
        dir: 'compiler/src/bytecode/',
        description: 'Portable-v1 bytecode backend. Lowers MIR directly into BytecodeProgram structures with a constant pool, bytecode units, basic blocks, and explicit instructions + terminators. Mirrors the MIR opcode surface.',
        keyTypes: [
            'BytecodeProgram', 'BytecodeUnit', 'BytecodeInstruction',
            'BytecodeValue', 'BytecodeConstant',
        ],
        keyFunctions: ['bytecode_build_program', 'bytecode_dump_program', 'bytecode_target_name'],
        files: [
            'bytecode.h', 'bytecode_instr_types.h', 'bytecode_internal.h',
            'bytecode.c', 'bytecode_memory.c', 'bytecode_helpers.c', 'bytecode_constants.c',
            'bytecode_lower.c', 'bytecode_lower_instr.c',
            'bytecode_dump.c', 'bytecode_dump_helpers.c', 'bytecode_dump_instr.c', 'bytecode_dump_names.c',
        ],
    },
    {
        name: 'CLI',
        dir: 'compiler/src/cli/',
        description: 'Command-line tools: the calynda compiler driver, AST dumper, semantic dumper, assembly emitter, bytecode emitter, and native builder. The native builder resolves runtime.o relative to the executable directory.',
        keyTypes: [],
        keyFunctions: [],
        files: [
            'calynda.c', 'calynda_internal.h', 'calynda_compile.c', 'calynda_utils.c',
            'dump_ast.c', 'dump_semantics.c', 'emit_asm.c', 'emit_bytecode.c',
            'build_native.c', 'build_native_utils.c',
        ],
    },
];
// ---------------------------------------------------------------------------
// Build targets
// ---------------------------------------------------------------------------
exports.BUILD_TARGETS = {
    mainBinaries: [
        { name: 'calynda', description: 'Main compiler/driver CLI' },
        { name: 'build_native', description: 'Assembles + links a native executable' },
    ],
    debugTools: [
        { name: 'dump_ast', description: 'AST pretty-printer; supports --expr mode' },
        { name: 'dump_semantics', description: 'Semantic state inspector (scopes, types, resolutions)' },
        { name: 'emit_asm', description: 'Assembly text emitter' },
        { name: 'emit_bytecode', description: 'Bytecode emitter' },
    ],
    testSuites: [
        'test_tokenizer', 'test_ast', 'test_parser', 'test_ast_dump',
        'test_symbol_table', 'test_type_resolution', 'test_type_checker', 'test_semantic_dump',
        'test_hir_dump', 'test_mir_dump', 'test_bytecode_dump', 'test_lir_dump',
        'test_codegen_dump', 'test_machine_dump',
        'test_runtime', 'test_asm_emit', 'test_build_native', 'test_calynda_cli',
    ],
    buildCommand: 'cd compiler && make test',
    testRunner: 'cd compiler && ./run_tests.sh',
};
// ---------------------------------------------------------------------------
// Compiler pipeline paths (text descriptions)
// ---------------------------------------------------------------------------
exports.NATIVE_PIPELINE = 'Source → Tokenizer → Parser → AST → SymbolTable → TypeResolution → TypeChecker → HIR → MIR → LIR → Codegen → Machine → AsmEmit → gcc link → executable';
exports.BYTECODE_PIPELINE = 'Source → Tokenizer → Parser → AST → SymbolTable → TypeResolution → TypeChecker → HIR → MIR → BytecodeProgram (portable-v1)';
exports.BACKEND_STRATEGY = `Calynda has two compiler backends and rejects any interpreter path:

Primary backend — native x86_64 SysV ELF:
  ${exports.NATIVE_PIPELINE}

Secondary backend — portable-v1 bytecode:
  ${exports.BYTECODE_PIPELINE}

MIR is the split point because it already has explicit control flow, closures, globals, calls, and throw.

The runtime contract is shared backend infrastructure:
  - Native assembly calls runtime helpers directly and emits startup wrappers.
  - Bytecode targets the same language-level operations.
  - Both backends share the same semantic boundary for dynamic features.`;
// ---------------------------------------------------------------------------
// Source tree summary
// ---------------------------------------------------------------------------
exports.SOURCE_TREE = `compiler/
  calynda.ebnf          — Canonical V1 EBNF grammar
  calynda_v2.ebnf       — V2 grammar sandbox (superset of V1)
  Makefile              — Build system
  run_tests.sh          — Test runner (make clean && make test)
  src/
    tokenizer/          — Lexer (6 files)
    parser/             — Recursive-descent parser (12 files)
    ast/                — AST nodes + dump (16 files)
    sema/               — Symbol table, type resolution, type checker, semantic dump (27 files)
    hir/                — High-level IR (14 files)
    mir/                — Mid-level IR (22 files)
    lir/                — Low-level IR (11 files)
    bytecode/           — Portable-v1 bytecode backend (13 files)
    codegen/            — x86_64 register allocation + instruction selection (8 files)
    backend/            — Machine emission, asm emission, runtime ABI (34 files)
    runtime/            — Runtime objects + helpers (6 files)
    cli/                — Driver + diagnostic tools (10 files)
  tests/                — 18 test suites (~1147 tests total)
  build/                — Build output directory`;
// ---------------------------------------------------------------------------
// Error handling pattern
// ---------------------------------------------------------------------------
exports.ERROR_PATTERN = `Each compiler stage uses the same error struct pattern:

  struct XyzBuildError {
      AstSourceSpan primary_span;
      AstSourceSpan related_span;
      bool has_related_span;
      char message[256];
  };

  struct XyzProgram {
      /* ... members ... */
      XyzBuildError error;
      bool has_error;
  };

Source spans carry line/column for precise diagnostic reporting.`;
//# sourceMappingURL=architecture.js.map