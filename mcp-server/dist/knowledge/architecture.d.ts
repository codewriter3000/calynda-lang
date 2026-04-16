/**
 * Calynda compiler architecture knowledge base.
 *
 * Describes the full compilation pipeline, source-tree layout,
 * key data structures per stage, build targets, and CLI tools.
 */
export declare const PIPELINE_STAGES: readonly ({
    name: string;
    dir: string;
    description: string;
    keyTypes: string[];
    keyFunctions: string[];
    files: string[];
} | {
    name: string;
    dir: string;
    description: string;
    keyTypes: string[];
    keyFunctions: string[];
    files: string[];
})[];
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
export declare const BACKEND_STRATEGY = "Calynda has two compiler backends and rejects any interpreter path:\n\nPrimary backend \u2014 native (x86_64 SysV ELF, AArch64 AAPCS64 ELF, and RISC-V 64 LP64D ELF):\n  Source \u2192 Tokenizer \u2192 Parser \u2192 AST \u2192 SymbolTable \u2192 TypeResolution \u2192 TypeChecker \u2192 HIR \u2192 MIR \u2192 LIR \u2192 Codegen \u2192 Machine \u2192 AsmEmit \u2192 gcc link \u2192 executable\n  Pass --target aarch64-linux or --target riscv64-linux for cross-target output (default: x86_64).\n\nSecondary backend \u2014 portable-v1 bytecode:\n  Source \u2192 Tokenizer \u2192 Parser \u2192 AST \u2192 SymbolTable \u2192 TypeResolution \u2192 TypeChecker \u2192 HIR \u2192 MIR \u2192 BytecodeProgram (portable-v1)\n\nMIR is the split point because it already has explicit control flow, closures, globals, calls, and throw.\nThe TargetDescriptor abstraction threads target-specific ABI, register, and instruction details through codegen, machine, and asm_emit.\n\nThe runtime contract is shared backend infrastructure:\n  - Native assembly calls runtime helpers directly and emits startup wrappers.\n  - Bytecode targets the same language-level operations.\n  - Both backends share the same semantic boundary for dynamic features.";
export declare const SOURCE_TREE = "compiler/\n  calynda.ebnf          \u2014 Canonical EBNF grammar\n  Makefile              \u2014 Build system\n  run_tests.sh          \u2014 Test runner (make clean && make test)\n  src/\n    tokenizer/          \u2014 Lexer (6 files)\n    parser/             \u2014 Recursive-descent parser (12 files)\n    ast/                \u2014 AST nodes + dump (16 files)\n    sema/               \u2014 Symbol table, type resolution, type checker, semantic dump (27 files)\n    hir/                \u2014 High-level IR (14 files)\n    mir/                \u2014 Mid-level IR (22 files)\n    lir/                \u2014 Low-level IR (11 files)\n    bytecode/           \u2014 Portable-v1 bytecode backend (13 files)\n    codegen/            \u2014 Target-aware register allocation + instruction selection (8 files)\n    backend/            \u2014 Machine emission, asm emission (x86_64 + AArch64 + RISC-V), runtime ABI, target descriptors\n    runtime/            \u2014 Runtime objects + helpers, including threading/future/atomic support\n    car/                \u2014 CAR source archive format (read, write, directory listing)\n    cli/                \u2014 Driver + diagnostic tools + CAR commands (12 files)\n  tests/                \u2014 19 test suites (~1405 tests total)\n  build/                \u2014 Build output directory";
export declare const ERROR_PATTERN = "Each compiler stage uses the same error struct pattern:\n\n  struct XyzBuildError {\n      AstSourceSpan primary_span;\n      AstSourceSpan related_span;\n      bool has_related_span;\n      char message[256];\n  };\n\n  struct XyzProgram {\n      /* ... members ... */\n      XyzBuildError error;\n      bool has_error;\n  };\n\nSource spans carry line/column for precise diagnostic reporting.";
