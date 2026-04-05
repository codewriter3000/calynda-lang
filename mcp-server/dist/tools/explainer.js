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
        native: `**Native Backend**\n\n${architecture_1.NATIVE_PIPELINE}\n\nTarget: x86_64 SysV ELF\nRegister allocation: rax (return), rdi/rsi/rdx/rcx/r8/r9 (SysV args), r10/r11/r12/r13 (allocatable vregs), r15 (closure environment)`,
    };
    for (const [key, explanation] of Object.entries(architectureExplanations)) {
        if (topic.includes(key)) {
            return { explanation };
        }
    }
    // Language feature explanations
    const keywordExplanations = {
        lambda: 'Calynda uses lambdas for all functions. Syntax: `(type param) -> expr` or `(type param) -> { ... }`. Lambda types collapse to their return type. Closures capture outer locals/parameters by symbol identity.',
        start: 'The `start` function is the program entry point. Syntax: `start(string[] args) -> { ... };`. It implicitly returns int32 (the exit code). The compiler enforces exactly one `start` with `start(string[] args)` semantics.',
        var: '`var` enables type inference. Example: `var x = 42;` infers x as int32.',
        throw: '`throw` is a keyword that throws an error. It lowers to an explicit MIR_TERM_THROW terminator and a BC_THROW bytecode op. Example: `throw "error message";`',
        exit: '`exit;` is sugar for `return null;` in void-typed lambdas. HIR normalizes it into `return;`.',
        template: 'Template literals use backticks with ${} interpolation. Example: `\\`Hello ${name}\\``. Zero-argument callable interpolations are auto-called during MIR lowering.',
        array: 'Arrays use T[] syntax for homogeneous arrays and `arr<T>` for heterogeneous arrays. Example: `int32[] nums = [1, 2, 3];`. Access with `nums[0]`, length with `nums.length`.',
        cast: 'Type casts use function-call syntax with a type name. Example: `int32(myFloat)`, `float64(myInt)`. Only primitive type casts are supported.',
        ternary: 'Ternary expressions: `condition ? consequent : alternate`. MIR lowers them into explicit branch/join blocks with synthetic merge locals.',
        modifier: 'Modifiers: `export`, `public`, `private`, `final`, `static`, `internal`. The `internal` modifier on local bindings restricts access to nested lambda scopes only.',
        import: 'Import styles: `import io.stdlib;` (plain), `import io.stdlib as std;` (alias), `import io.stdlib.*;` (wildcard), `import io.stdlib.{print, readLine};` (selective).',
        package: 'Declare the package: `package myapp.utils;`.',
        return: '`return expr;` returns a value from a lambda. `return;` for void lambdas. The type checker enforces contextual return errors for non-void lambdas and start.',
        null: '`null` is the null value. void functions implicitly return null.',
        union: 'Tagged unions with optional generic parameters. Syntax: `union Option<T> { Some(T), None };`. Unions support variant construction: `Option.Some(42)`, `Option.None`.',
        generic: 'Reified generics use `<T>` syntax. Named types, union declarations, and `arr<T>` support type parameters. Parser handles `>>` splitting for nested generics.',
        closure: 'Closures are created via lambda expressions and capture outer locals/parameters. MIR lowers them to MIR_UNIT_LAMBDA + MIR_INSTR_CLOSURE. The capture environment is passed in r15 on x86_64.',
        increment: 'Pre/post increment/decrement: `++x`, `x++`, `--x`, `x--`. MIR uses a read-modify-write pattern for post-ops. Machine layer emits x86 `inc`/`dec` for pre-ops.',
        discard: 'The discard expression `_` can be used as an assignment target to explicitly ignore a value: `_ = computeSomething();`',
        varargs: 'Variadic parameters use `Type... name` syntax. Must be the last parameter.',
        internal: 'The `internal` modifier on local bindings restricts access to nested lambda scopes only; the type checker enforces visibility by walking scope boundaries.',
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
        explanation: `No specific documentation found for "${input.topic}". Calynda is a compiled functional systems programming language with ~200 C source files across 12 directories.\n\nLanguage topics: types, lambdas, start, unions, generics, closures, templates, arrays, casts, throw, exit, modifiers, imports.\n\nCompiler topics: pipeline, architecture, tokenizer, parser, AST, symbol table, type checker, HIR, MIR, LIR, codegen, machine, asm, bytecode, runtime, backend, build, source tree, error pattern.`,
    };
}
//# sourceMappingURL=explainer.js.map