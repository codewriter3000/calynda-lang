"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.explainTopic = explainTopic;
const types_1 = require("../knowledge/types");
const examples_1 = require("../knowledge/examples");
const architecture_1 = require("../knowledge/architecture");
const bytecode_1 = require("../knowledge/bytecode");
function explainTopic(input) {
    const topic = input.topic.toLowerCase().trim();
    // Type documentation
    if (types_1.TYPE_DOCS[topic]) {
        const info = types_1.TYPE_DOCS[topic];
        let explanation = `**${info.name}**: ${info.description}`;
        if (info.size)
            explanation += `\n- Size: ${info.size}`;
        if (info.range)
            explanation += `\n- Range: ${info.range}`;
        return { explanation, examples: info.examples };
    }
    // Compiler pipeline stages
    for (const stage of architecture_1.PIPELINE_STAGES) {
        if (topic.includes(stage.name.toLowerCase())) {
            let explanation = `**${stage.name}** (${stage.dir})\n\n${stage.description}`;
            if (stage.keyTypes.length > 0)
                explanation += `\n\n**Key types:** ${stage.keyTypes.join(', ')}`;
            if (stage.keyFunctions.length > 0)
                explanation += `\n\n**Key functions:** ${stage.keyFunctions.join(', ')}`;
            explanation += `\n\n**Files (${stage.files.length}):** ${stage.files.join(', ')}`;
            return { explanation };
        }
    }
    // Architecture / pipeline topics
    const architectureExplanations = {
        pipeline: `**Compiler Pipeline**\n\nNative path:\n  ${architecture_1.NATIVE_PIPELINE}\n\nBytecode path:\n  ${architecture_1.BYTECODE_PIPELINE}`,
        architecture: `**Compiler Architecture**\n\n${architecture_1.SOURCE_TREE}`,
        backend: architecture_1.BACKEND_STRATEGY,
        bytecode: bytecode_1.BYTECODE_ISA,
        'source tree': architecture_1.SOURCE_TREE,
        'build': `**Build System**\n\nBuild command: \`${architecture_1.BUILD_TARGETS.buildCommand}\`\nTest runner: \`${architecture_1.BUILD_TARGETS.testRunner}\`\n\n**Main binaries:** ${architecture_1.BUILD_TARGETS.mainBinaries.map(b => `${b.name} — ${b.description}`).join('; ')}\n\n**Debug tools:** ${architecture_1.BUILD_TARGETS.debugTools.map(b => `${b.name} — ${b.description}`).join('; ')}\n\n**Test suites (${architecture_1.BUILD_TARGETS.testSuites.length}):** ${architecture_1.BUILD_TARGETS.testSuites.join(', ')}`,
        'error pattern': architecture_1.ERROR_PATTERN,
        native: `**Native Backend**\n\n${architecture_1.NATIVE_PIPELINE}\n\nTargets: x86_64 SysV ELF (default), AArch64 AAPCS64 ELF, and RISC-V 64 LP64D ELF. Use \`--target aarch64\` or \`--target riscv64\` for cross-target output. Register allocation uses TargetDescriptor for target-specific register sets and ABI conventions.`,
        target: `**Target Abstraction**\n\nThe TargetDescriptor abstraction threads target-specific ABI, register, and instruction details through codegen, machine, and asm_emit.\n\nSupported targets:\n- x86_64 (default): x86_64 SysV ELF — rax (return), rdi/rsi/rdx/rcx/r8/r9 (args), r15 (closure env)\n- aarch64: AArch64 AAPCS64 ELF\n- riscv64: RISC-V 64 LP64D ELF\n\nPass \`--target aarch64\` or \`--target riscv64\` to calynda asm/build/run for cross-target output.`,
        car: `**CAR Source Archives**\n\nThe CAR (Calynda Archive) format bundles multiple .cal source files into a single portable archive.\n\nCLI commands:\n- \`calynda pack src/ -o archive.car\` — create an archive from a directory\n- \`calynda asm project.car\` — emit assembly directly from an archive\n- \`calynda build project.car\` — build directly from an archive\n- \`calynda run project.car\` — build and run from an archive\n\nIntra-archive imports are stripped; external imports are preserved.`,
    };
    for (const [key, explanation] of Object.entries(architectureExplanations)) {
        if (topic.includes(key)) {
            return { explanation };
        }
    }
    // Language feature explanations
    const keywordExplanations = {
        lambda: 'Calynda uses lambdas for all functions. Syntax: `(type param) -> expr` or `(type param) -> { ... }`. Whole-function manual shorthand is also supported for block-bodied lambdas: `manual(type param) -> { ... }`. Explicitly typed top-level lambda bindings are recursive within their own body; this also applies to whole-function manual shorthand, but not to inferred, local, or non-lambda bindings. Lambda types collapse to their return type. Closures capture outer locals/parameters by symbol identity.',
        start: 'The `start` function is the program entry point. Syntax: `start(string[] args) -> { ... };`. It implicitly returns int32 (the exit code). The compiler enforces exactly one `start` with `start(string[] args)` semantics.',
        var: '`var` enables type inference. Example: `var x = 42;` infers x as int32.',
        throw: '`throw` is a keyword that throws an error. It lowers to an explicit MIR_TERM_THROW terminator and a BC_THROW bytecode op. Example: `throw "error message";`',
        exit: '`exit;` is sugar for `return null;` in void-typed lambdas. HIR normalizes it into `return;`.',
        template: 'Template literals use backticks with ${} interpolation. Example: `\\`Hello ${name}\\``. Zero-argument callable interpolations are auto-called during MIR lowering.',
        array: 'Arrays use T[] syntax for homogeneous arrays and `arr<?>` for heterogeneous arrays. Example: `int32[] nums = [1, 2, 3];` or `arr<?> mixed = [1, true, "hello"];`. Heterogeneous arrays are shape-locked after literal construction; indexed reads produce external values that usually need explicit casts.',
        cast: 'Type casts use function-call syntax with a type name. Example: `int32(myFloat)`, `float64(myInt)`. Only primitive type casts are supported.',
        ternary: 'Ternary expressions: `condition ? consequent : alternate`. MIR lowers them into explicit branch/join blocks with synthetic merge locals.',
        modifier: 'Modifiers: `export`, `public`, `private`, `final`, `static`, `internal`, `thread_local`. The `internal` modifier on local bindings restricts access to nested lambda scopes only; `thread_local` is an alpha.2 storage modifier with a deliberately narrow contract.',
        import: 'Import styles: `import io.stdlib;` (plain), `import io.stdlib as std;` (alias), `import io.stdlib.*;` (wildcard), `import io.stdlib.{print, readLine};` (selective).',
        package: 'Declare the package: `package myapp.utils;`.',
        return: '`return expr;` returns a value from a lambda. `return;` for void lambdas. The type checker enforces contextual return errors for non-void lambdas and start.',
        null: '`null` is the null value. void functions implicitly return null.',
        union: 'Tagged unions with optional generic parameters. Syntax: `union Option<T> { Some(T), None };`. Unions support variant construction (`Option.Some(42)`, `Option.None`) plus read-only `.tag` and `.payload` access on union values.',
        generic: 'Reified generics use `<T>` syntax. Named types, union declarations, `arr<?>`, `ptr<T>`, `Future<T>`, and `Atomic<T>` support type parameters. Wildcard `?` survives into the shared runtime descriptor model.',
        closure: 'Closures are created via lambda expressions and capture outer locals/parameters. MIR lowers them to MIR_UNIT_LAMBDA + MIR_INSTR_CLOSURE. The capture environment is passed in r15 on x86_64.',
        increment: 'Pre/post increment/decrement: `++x`, `x++`, `--x`, `x--`. MIR uses a read-modify-write pattern for post-ops. Machine layer emits x86 `inc`/`dec` for pre-ops.',
        discard: 'The discard expression `_` can be used as an assignment target to explicitly ignore a value: `_ = computeSomething();`',
        varargs: 'Variadic parameters use `Type... name` syntax. Must be the last parameter.',
        internal: 'The `internal` modifier on local bindings restricts access to nested lambda scopes only; the type checker enforces visibility by walking scope boundaries.',
        asm: '`asm()` is an inline assembly declaration. Syntax: `int32 name = asm(int32 a, int32 b) -> { mov eax, edi; add eax, esi; ret };`. The assembly body is passed through to the assembler unchanged. Parameters follow normal Calynda type syntax and map to ABI registers. The `export` and `static` modifiers are supported.',
        boot: '`boot()` is a bare-metal entry point that bypasses the Calynda runtime and emits a freestanding `_start` symbol. Intended for freestanding and embedded targets. Syntax: `boot() -> 0;` or `boot() -> { ... };`. `boot()` and `start()` cannot coexist, and the alpha.2 contract no longer advertises Linux-only exit behaviour for bare-metal RV64 entry glue.',
        manual: '`manual { ... };` and `manual checked { ... };` are the stable unsafe manual-memory surface. Whole-function manual shorthand is also supported for block-bodied lambdas: `manual(type param) -> { ... }`, which desugars to a normal lambda whose body is wrapped in an existing manual block. When that shorthand is used in an explicitly typed top-level binding, the binding is recursive within its own body. Built-ins include malloc/calloc/realloc/free, cleanup(value, fn), stackalloc(size), deref, store, offset, and addr. `ptr<T>` adds typed sizing for deref/store/offset, `manual checked` routes through growable bounds tracking, and `stackalloc(size)` means manual-scope scratch storage reclaimed at scope exit. Forced pthread-style cancellation does not yet guarantee Calynda-level cleanup execution for manual scopes.',
        recurs: 'Calynda supports recursion for explicitly typed top-level lambda bindings, including whole-function manual shorthand. This is a semantic rule rather than a distinct grammar production. Inferred, local, and non-lambda bindings do not gain recursive self-reference.',
        layout: '`layout Name { int32 x; int32 y; };` declares a struct-like type for use with `ptr<T>`, `offset`, `deref`, and `store`. Layout fields remain intentionally restricted to scalar primitive types in alpha.2.',
        spawn: '`spawn expr` launches a zero-argument callable. If the callable returns `void`, the result is `Thread`; otherwise the result is `Future<T>`. Supported alpha.2 members are `Thread.join()`, `Thread.cancel()`, `Future<T>.get()`, and `Future<T>.cancel()`.',
        future: '`Future<T>` is produced by spawning a zero-argument non-void callable. Use `.get()` to join and read the result, or `.cancel()` for pthread-style forced cancellation.',
        atomic: '`Atomic<T>` is the alpha.2 atomic cell type for first-class single-word runtime values. Construct with `Atomic.new(value)` and use `.load()`, `.store(value)`, or `.exchange(value)`.',
        'thread_local': '`thread_local` declares thread-local storage. In alpha.2 it is intentionally limited to storage with cross-thread identity, not ordinary stack locals.',
        version: 'Use `calynda --version` to print the CLI version metadata.',
        'race check': 'The strict race-checker flag name is `--strict-race-check`.',
        'type alias': '`type Name = ExistingType;` creates a top-level type alias. Type aliases may carry normal declaration modifiers before `type`.',
        'numeric literal': 'Numeric literal separators use `_`. The tokenizer strips underscores before parsing, so forms like `1_000`, `0xFF_FF`, and `3.141_592` follow the existing literal grammar.',
        numeric: 'Numeric literal separators use `_`. The tokenizer strips underscores before parsing, so forms like `1_000`, `0xFF_FF`, and `3.141_592` follow the existing literal grammar.',
        arm64: 'ARM64 / AArch64 is a supported compilation target. Pass `--target aarch64` to `calynda asm`, `calynda build`, or `calynda run` to produce AArch64 output. The default target remains x86_64 SysV.',
    };
    for (const [key, explanation] of Object.entries(keywordExplanations)) {
        if (topic.includes(key)) {
            const relevantExamples = examples_1.EXAMPLES
                .filter(e => e.tags.some(t => t.includes(key) || key.includes(t)))
                .map(e => e.code);
            return { explanation, examples: relevantExamples.slice(0, 2) };
        }
    }
    return {
        explanation: `No specific documentation found for "${input.topic}". Calynda is a compiled functional systems programming language with ~200 C source files across 12 directories.\n\nLanguage topics: types, thread, future, atomic, mutex, thread_local, spawn, recursion, type alias, numeric literals, lambdas, manual shorthand, start, boot, unions, generics, closures, templates, arrays, casts, throw, exit, modifiers, imports.\n\nCompiler topics: pipeline, architecture, tokenizer, parser, AST, symbol table, type checker, HIR, MIR, LIR, codegen, machine, asm, bytecode, runtime, backend, build, source tree, error pattern, CAR archives.`,
    };
}
//# sourceMappingURL=explainer.js.map