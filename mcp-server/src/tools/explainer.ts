import { TYPE_DOCS } from '../knowledge/types';
import { EXAMPLES } from '../knowledge/examples';
import {
  PIPELINE_STAGES, NATIVE_PIPELINE, BYTECODE_PIPELINE,
  BACKEND_STRATEGY, SOURCE_TREE, ERROR_PATTERN, BUILD_TARGETS,
} from '../knowledge/architecture';
import { BYTECODE_ISA } from '../knowledge/bytecode';

export interface ExplainInput {
  topic: string;
}

export interface ExplainResult {
  explanation: string;
  examples?: string[];
}

export function explainTopic(input: ExplainInput): ExplainResult {
  const topic = input.topic.toLowerCase().trim();

  // Type documentation
  if (TYPE_DOCS[topic]) {
    const info = TYPE_DOCS[topic];
    let explanation = `**${info.name}**: ${info.description}`;
    if (info.size) explanation += `\n- Size: ${info.size}`;
    if (info.range) explanation += `\n- Range: ${info.range}`;
    return { explanation, examples: info.examples };
  }

  // Compiler pipeline stages
  for (const stage of PIPELINE_STAGES) {
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
  const architectureExplanations: Record<string, string> = {
    pipeline: `**Compiler Pipeline**\n\nNative path:\n  ${NATIVE_PIPELINE}\n\nBytecode path:\n  ${BYTECODE_PIPELINE}`,
    architecture: `**Compiler Architecture**\n\n${SOURCE_TREE}`,
    backend: BACKEND_STRATEGY,
    bytecode: BYTECODE_ISA,
    'source tree': SOURCE_TREE,
    'build': `**Build System**\n\nBuild command: \`${BUILD_TARGETS.buildCommand}\`\nTest runner: \`${BUILD_TARGETS.testRunner}\`\n\n**Main binaries:** ${BUILD_TARGETS.mainBinaries.map(b => `${b.name} — ${b.description}`).join('; ')}\n\n**Debug tools:** ${BUILD_TARGETS.debugTools.map(b => `${b.name} — ${b.description}`).join('; ')}\n\n**Test suites (${BUILD_TARGETS.testSuites.length}):** ${BUILD_TARGETS.testSuites.join(', ')}`,
    'error pattern': ERROR_PATTERN,
    native: `**Native Backend**\n\n${NATIVE_PIPELINE}\n\nTargets: x86_64 SysV ELF (default) and AArch64 AAPCS64 ELF (--target aarch64-linux).\nRegister allocation uses TargetDescriptor for target-specific register sets and ABI conventions.`,
    target: `**Target Abstraction**\n\nThe TargetDescriptor abstraction threads target-specific ABI, register, and instruction details through codegen, machine, and asm_emit.\n\nSupported targets:\n- x86_64 (default): x86_64 SysV ELF — rax (return), rdi/rsi/rdx/rcx/r8/r9 (args), r15 (closure env)\n- aarch64-linux: AArch64 AAPCS64 ELF\n- riscv64-linux: RISC-V 64 LP64D ELF\n\nPass --target aarch64-linux or --target riscv64-linux to calynda build for cross-target output.`,
    car: `**CAR Source Archives**\n\nThe CAR (Calynda Archive) format bundles multiple .cal source files into a single portable archive.\n\nCLI commands:\n- \`calynda pack src/ -o archive.car\` — create an archive from a directory\n- \`calynda build project.car\` — build directly from an archive\n- \`calynda run project.car\` — build and run from an archive\n\nIntra-archive imports are stripped; external imports are preserved.`,
  };

  for (const [key, explanation] of Object.entries(architectureExplanations)) {
    if (topic.includes(key)) {
      return { explanation };
    }
  }

  // Language feature explanations
  const keywordExplanations: Record<string, string> = {
    lambda: 'Calynda uses lambdas for all functions. Syntax: `(type param) -> expr` or `(type param) -> { ... }`. Lambda types collapse to their return type. Closures capture outer locals/parameters by symbol identity.',
    start: 'The `start` function is the program entry point. Syntax: `start(string[] args) -> { ... };`. It implicitly returns int32 (the exit code). The compiler enforces exactly one `start` with `start(string[] args)` semantics.',
    var: '`var` enables type inference. Example: `var x = 42;` infers x as int32.',
    throw: '`throw` is a keyword that throws an error. It lowers to an explicit MIR_TERM_THROW terminator and a BC_THROW bytecode op. Example: `throw "error message";`',
    exit: '`exit;` is sugar for `return null;` in void-typed lambdas. HIR normalizes it into `return;`.',
    template: 'Template literals use backticks with ${} interpolation. Example: `\\`Hello ${name}\\``. Zero-argument callable interpolations are auto-called during MIR lowering.',
    array: 'Arrays use T[] syntax for homogeneous arrays and `arr<?>` for heterogeneous arrays. Example: `int32[] nums = [1, 2, 3];` or `arr<?> mixed = [1, true, "hello"];`. Heterogeneous arrays are shape-locked after literal construction; indexed reads produce external values that usually need explicit casts.',
    cast: 'Type casts use function-call syntax with a type name. Example: `int32(myFloat)`, `float64(myInt)`. Only primitive type casts are supported.',
    ternary: 'Ternary expressions: `condition ? consequent : alternate`. MIR lowers them into explicit branch/join blocks with synthetic merge locals.',
    modifier: 'Modifiers: `export`, `public`, `private`, `final`, `static`, `internal`. The `internal` modifier on local bindings restricts access to nested lambda scopes only.',
    import: 'Import styles: `import io.stdlib;` (plain), `import io.stdlib as std;` (alias), `import io.stdlib.*;` (wildcard), `import io.stdlib.{print, readLine};` (selective).',
    package: 'Declare the package: `package myapp.utils;`.',
    return: '`return expr;` returns a value from a lambda. `return;` for void lambdas. The type checker enforces contextual return errors for non-void lambdas and start.',
    null: '`null` is the null value. void functions implicitly return null.',
    union: 'Tagged unions with optional generic parameters. Syntax: `union Option<T> { Some(T), None };`. Unions support variant construction (`Option.Some(42)`, `Option.None`) plus read-only `.tag` and `.payload` access on union values.',
    generic: 'Reified generics use `<T>` syntax. Named types, union declarations, `arr<?>`, and `ptr<T>` support type parameters. Wildcard `?` survives into the shared runtime descriptor model for 0.4.0 metadata.',
    closure: 'Closures are created via lambda expressions and capture outer locals/parameters. MIR lowers them to MIR_UNIT_LAMBDA + MIR_INSTR_CLOSURE. The capture environment is passed in r15 on x86_64.',
    increment: 'Pre/post increment/decrement: `++x`, `x++`, `--x`, `x--`. MIR uses a read-modify-write pattern for post-ops. Machine layer emits x86 `inc`/`dec` for pre-ops.',
    discard: 'The discard expression `_` can be used as an assignment target to explicitly ignore a value: `_ = computeSomething();`',
    varargs: 'Variadic parameters use `Type... name` syntax. Must be the last parameter.',
    internal: 'The `internal` modifier on local bindings restricts access to nested lambda scopes only; the type checker enforces visibility by walking scope boundaries.',
    asm: '`asm()` is an inline assembly declaration. Syntax: `int32 name = asm(int32 a, int32 b) -> { mov eax, edi; add eax, esi; ret };`. The assembly body is passed through to the assembler unchanged. Parameters follow normal Calynda type syntax and map to ABI registers. The `export` and `static` modifiers are supported.',
    boot: '`boot()` is a bare-metal entry point that bypasses the Calynda runtime entirely and emits a raw `_start` symbol. Intended for freestanding and embedded targets. Syntax: `boot() -> 0;` or `boot() -> { ... };`. `boot()` and `start()` cannot coexist in the same compilation unit.',
    manual: '`manual { ... };` and `manual checked { ... };` are the stable unsafe 0.4.0 manual-memory surface. Built-ins include malloc/calloc/realloc/free, cleanup(value, fn), stackalloc(size), deref, store, offset, and addr. `ptr<T>` adds typed sizing for deref/store/offset, `manual checked` routes through growable bounds tracking, and `stackalloc(size)` means manual-scope scratch storage reclaimed at scope exit.',
    layout: '`layout Name { int32 x; int32 y; };` declares a struct-like type for use with `ptr<T>`, `offset`, `deref`, and `store`. In 0.4.0, layout fields are intentionally restricted to scalar primitive types.',
    arm64: 'ARM64 / AArch64 is a supported compilation target. Pass `--target aarch64-linux` to `calynda build` to produce AArch64 assembly. The default target remains Linux x86_64 SysV.',
  };

  for (const [key, explanation] of Object.entries(keywordExplanations)) {
    if (topic.includes(key)) {
      const relevantExamples = EXAMPLES
        .filter(e => e.tags.some(t => t.includes(key) || key.includes(t)))
        .map(e => e.code);
      return { explanation, examples: relevantExamples.slice(0, 2) };
    }
  }

  return {
    explanation: `No specific documentation found for "${input.topic}". Calynda is a compiled functional systems programming language with ~200 C source files across 12 directories.\n\nLanguage topics: types, lambdas, start, unions, generics, closures, templates, arrays, casts, throw, exit, modifiers, imports.\n\nCompiler topics: pipeline, architecture, tokenizer, parser, AST, symbol table, type checker, HIR, MIR, LIR, codegen, machine, asm, bytecode, runtime, backend, build, source tree, error pattern.`,
  };
}
