"use strict";
/**
 * Calynda compiler architecture knowledge base.
 *
 * Describes the full compilation pipeline, source-tree layout,
 * key data structures per stage, build targets, and CLI tools.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.ERROR_PATTERN = exports.SOURCE_TREE = exports.BACKEND_STRATEGY = exports.BYTECODE_PIPELINE = exports.NATIVE_PIPELINE = exports.BUILD_TARGETS = exports.PIPELINE_STAGES = void 0;
const architecture_stages_1 = require("./architecture-stages");
// ---------------------------------------------------------------------------
// Pipeline overview — backend stages
// ---------------------------------------------------------------------------
const PIPELINE_STAGES_BACKEND = [
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
        description: 'Target-aware code generation via TargetDescriptor abstraction (x86_64 SysV ELF, AArch64 AAPCS64 ELF, and RISC-V 64 LP64D ELF). Classifies LIR ops into direct machine-pattern candidates vs runtime helpers. Uses target-specific register sets for allocation.',
        keyTypes: [
            'CodegenProgram', 'TargetDescriptor', 'CodegenRegister',
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
        description: 'Target-agnostic machine layer consuming LIR + codegen plan. Emits instruction streams, helper-call setup, prologue/epilogue, and conservative register preservation. All operations use TargetDescriptor for register names and ABI conventions. Supports x86_64, AArch64, and RISC-V 64.',
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
        description: 'Lowers MachineProgram into GNU assembler text with concrete stack addresses, rodata literals, global storage, string-object data, runtime helper calls, main entry glue, and .note.GNU-stack. Dispatches to x86_64, AArch64, or RISC-V emitters based on target.',
        keyTypes: [],
        keyFunctions: ['asm_emit_program', 'asm_emit_program_to_string'],
        files: [
            'asm_emit.h', 'asm_emit.c', 'asm_emit_internal.h',
            'asm_emit_helpers.c', 'asm_emit_symbols.c',
            'asm_emit_operand.c', 'asm_emit_operand_ext.c',
            'asm_emit_instr.c', 'asm_emit_instr_aarch64.c',
            'asm_emit_sections.c', 'asm_emit_entry.c', 'asm_emit_entry_aarch64.c',
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
        description: 'Concrete runtime objects and values. Values are raw machine words or registered heap-object handles. Object kinds include strings, arrays, closures, packages, extern callables, template-part packs, unions, threads, futures, mutexes, and atomics. Includes process startup that boxes argv into Calynda string[] and registers static string objects.',
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
        description: 'Command-line tools: the calynda compiler driver (supports --version, --strict-race-check, and --target for x86_64/aarch64/riscv64), AST dumper, semantic dumper, assembly emitter, bytecode emitter, native builder, and CAR archive commands (pack/build/run). Build, run, and asm all accept .car archives, while bytecode remains .cal-only. The native builder resolves the runtime archive relative to the executable directory.',
        keyTypes: [],
        keyFunctions: [],
        files: [
            'calynda.c', 'calynda_internal.h', 'calynda_compile.c', 'calynda_utils.c',
            'calynda_car.c', 'calynda_car_helpers.c',
            'dump_ast.c', 'dump_semantics.c', 'emit_asm.c', 'emit_bytecode.c',
            'build_native.c', 'build_native_utils.c',
        ],
    },
];
exports.PIPELINE_STAGES = [...architecture_stages_1.PIPELINE_STAGES_FRONTEND, ...PIPELINE_STAGES_BACKEND];
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
        'test_runtime', 'test_asm_emit', 'test_build_native', 'test_calynda_cli', 'test_car',
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

Primary backend — native (x86_64 SysV ELF, AArch64 AAPCS64 ELF, and RISC-V 64 LP64D ELF):
  ${exports.NATIVE_PIPELINE}
  Pass --target aarch64 or --target riscv64 for cross-target output (default: x86_64).

Secondary backend — portable-v1 bytecode:
  ${exports.BYTECODE_PIPELINE}

MIR is the split point because it already has explicit control flow, closures, globals, calls, and throw.
The TargetDescriptor abstraction threads target-specific ABI, register, and instruction details through codegen, machine, and asm_emit.

The runtime contract is shared backend infrastructure:
  - Native assembly calls runtime helpers directly and emits startup wrappers.
  - Bytecode targets the same language-level operations.
  - Both backends share the same semantic boundary for dynamic features.`;
// ---------------------------------------------------------------------------
// Source tree summary
// ---------------------------------------------------------------------------
exports.SOURCE_TREE = `compiler/
  calynda.ebnf          — Canonical EBNF grammar
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
    codegen/            — Target-aware register allocation + instruction selection (8 files)
    backend/            — Machine emission, asm emission (x86_64 + AArch64 + RISC-V), runtime ABI, target descriptors
    runtime/            — Runtime objects + helpers, including threading/future/atomic support
    car/                — CAR source archive format (read, write, directory listing)
    cli/                — Driver + diagnostic tools + CAR commands (12 files)
  tests/                — 19 test suites (~1405 tests total)
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