"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PROMPTS = void 0;
exports.getPromptMessages = getPromptMessages;
exports.PROMPTS = [
    {
        name: 'write-calynda-function',
        description: 'Template for writing Calynda functions',
        arguments: [
            { name: 'task', description: 'Description of what the function should do', required: true },
            { name: 'returnType', description: 'The return type of the function (e.g., int32, string)', required: false },
        ],
    },
    {
        name: 'debug-calynda-code',
        description: 'Template for debugging Calynda code',
        arguments: [
            { name: 'code', description: 'The Calynda code to debug', required: true },
            { name: 'problem', description: 'Description of the problem or error', required: false },
        ],
    },
    {
        name: 'convert-to-calynda',
        description: 'Template for converting code from other languages to Calynda',
        arguments: [
            { name: 'code', description: 'The source code to convert', required: true },
            { name: 'sourceLanguage', description: 'The source programming language (e.g., Python, JavaScript, C)', required: false },
        ],
    },
    {
        name: 'explain-compiler-stage',
        description: 'Explain a specific stage of the Calynda compiler pipeline',
        arguments: [
            { name: 'stage', description: 'The pipeline stage to explain (e.g., tokenizer, parser, HIR, MIR, LIR, codegen, bytecode)', required: true },
        ],
    },
];
const CALYNDA_LANGUAGE_FACTS = `Calynda key facts:
- Compiled functional systems programming language with ~200 C source files
- Two backends: native x86_64 SysV ELF / AArch64 AAPCS64 ELF / RISC-V 64 LP64D ELF and portable-v1 bytecode (no interpreter)
- CLI version metadata is available via calynda --version
- Strict race-checker flag spelling: --strict-race-check
- All functions are lambdas: (type param) -> expr or (type param) -> { ... }
- Block-bodied lambdas also support whole-function manual shorthand: manual(type param) -> { ... }
- Explicitly typed top-level lambda bindings are recursive within their own body; this also applies to whole-function manual shorthand, but not to inferred, local, or non-lambda bindings
- Entry point: start(string[] args) -> { ... }; or bare form start -> { ... }; returns int32 (exit code)
- Bare-metal entry point: boot -> expr; bypasses runtime, emits freestanding _start, and cannot coexist with start
- Inline assembly: int32 name = asm(int32 a) -> { ... }; passed through to assembler unchanged
- Stable unsafe manual memory: manual { ... }; / manual checked { ... }; with malloc/calloc/realloc/free, cleanup(value, fn), stackalloc(size), deref/store/offset/addr, and ptr<T>
- Types: int8/16/32/64, uint8/16/32/64, float32/64, bool, char, string, T[], arr<?>, ptr<T>, layout types, Thread, Future<T>, Mutex, Atomic<T>, void
- Java-style aliases: byte, sbyte, short, int, long, ulong, uint, float, double
- Type aliases: type Name = ExistingType;
- Threading: spawn launches a zero-arg callable; void returns Thread, non-void returns Future<T>; Thread has .join()/.cancel(); Future<T> has .get()/.cancel(); Mutex.new() creates a mutex with .lock()/.unlock()
- thread_local exists for storage with cross-thread identity, not ordinary stack locals
- Atomic<T> is alpha.2-only and limited to first-class single-word runtime values
- Tagged unions with reified generics: union Option<T> { Some(T), None }; union values expose read-only .tag and .payload
- Heterogeneous arrays: arr<?> mixed = [1, "hello", true]; indexed reads produce external values and indexed writes remain rejected
- Layout declarations exist for ptr<T> use; layout fields are limited to scalar primitive types
- Numeric literals support underscore separators; the tokenizer strips underscores before parsing
- Template literals with \${} interpolation (backtick strings)
- No built-in if/else or loops — use ternary for conditionals, library functions for iteration
- throw keyword for errors, exit; is sugar for return; in void lambdas
- var keyword for type inference
- Modifiers: export, public, private, final, static, internal, thread_local
- Import styles: plain, alias ("as"), wildcard (".*"), selective (".{a, b}")
- Closures capture outer locals/parameters; ++ and -- prefix/postfix operators
- Discard expression: _ = expr; to explicitly ignore values
- Varargs: Type... name in parameter lists; optional parameter defaults: Type name = defaultExpr after required parameters
- Swap statement: a >< b; atomically exchanges two same-type mutable l-values in one statement
- CAR archives bundle .cal files: calynda pack src/ -o archive.car, then calynda asm/build/run project.car`;
function getPromptMessages(name, args) {
    switch (name) {
        case 'write-calynda-function':
            return [{
                    role: 'user',
                    content: `Write a Calynda function to: ${args['task']}${args['returnType'] ? `\nThe function should return type: ${args['returnType']}` : ''}\n\n${CALYNDA_LANGUAGE_FACTS}`,
                }];
        case 'debug-calynda-code':
            return [{
                    role: 'user',
                    content: `Debug this Calynda code${args['problem'] ? ` (Problem: ${args['problem']})` : ''}:\n\n\`\`\`cal\n${args['code']}\n\`\`\`\n\n${CALYNDA_LANGUAGE_FACTS}\n\nCheck for:\n- Syntax errors (missing semicolons, incorrect -> arrow syntax)\n- Type mismatches and invalid casts\n- Missing or incorrect start entry point (start -> ... or start(string[] args) -> ...; must be exactly one)\n- boot and start cannot coexist in the same compilation unit\n- Swap statement: a >< b; requires same type mutable l-values\n- Undefined variables or out-of-scope references\n- Incorrect lambda parameter types\n- Template literal interpolation issues (zero-arg callables are auto-called)\n- Incorrect use of throw, exit, return (exit only in void lambdas)\n- internal visibility violations (nested-lambda-only access)\n- Assignment to non-l-values (imports, packages, final bindings, temporaries)\n- manual { } blocks: malloc/calloc/realloc/free/cleanup/stackalloc and pointer ops only valid inside manual scope`,
                }];
        case 'convert-to-calynda':
            return [{
                    role: 'user',
                    content: `Convert this ${args['sourceLanguage'] || 'code'} to Calynda:\n\n\`\`\`${args['sourceLanguage'] ? args['sourceLanguage'].toLowerCase() : ''}\n${args['code']}\n\`\`\`\n\n${CALYNDA_LANGUAGE_FACTS}`,
                }];
        case 'explain-compiler-stage':
            return [{
                    role: 'user',
                    content: `Explain the ${args['stage']} stage of the Calynda compiler.\n\nThe Calynda compiler pipeline:\n- Native path: Source → Tokenizer → Parser → AST → SymbolTable → TypeResolution → TypeChecker → HIR → MIR → LIR → Codegen → Machine → AsmEmit → gcc link → executable\n- Bytecode path: Source → Tokenizer → Parser → AST → SymbolTable → TypeResolution → TypeChecker → HIR → MIR → BytecodeProgram (portable-v1)\n\nMIR is the split point. The runtime is shared infrastructure (strings, arrays, closures, unions, packages), not an interpreter surface.\n\nPlease explain what the ${args['stage']} stage does, its key data structures, and how it fits into the pipeline.`,
                }];
        default:
            return [{ role: 'user', content: `Unknown prompt: ${name}` }];
    }
}
//# sourceMappingURL=index.js.map