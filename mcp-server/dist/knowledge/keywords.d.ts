export declare const KEYWORDS: readonly ["package", "import", "public", "private", "final", "var", "start", "boot", "return", "exit", "throw", "null", "true", "false", "void", "export", "as", "internal", "static", "thread_local", "type", "union", "manual", "arr", "ptr", "layout", "spawn", "checked", "asm", "malloc", "calloc", "realloc", "free", "deref", "store", "offset", "addr", "cleanup", "stackalloc"];
export declare const PRIMITIVE_TYPES: readonly ["int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "float32", "float64", "bool", "char", "string", "byte", "sbyte", "short", "int", "long", "ulong", "uint", "float", "double", "num"];
export declare const BUILTIN_TYPES: readonly ["Thread", "Future<T>", "Mutex", "Atomic<T>"];
export declare const ALL_RESERVED: readonly ["package", "import", "public", "private", "final", "var", "start", "boot", "return", "exit", "throw", "null", "true", "false", "void", "export", "as", "internal", "static", "thread_local", "type", "union", "manual", "arr", "ptr", "layout", "spawn", "checked", "asm", "malloc", "calloc", "realloc", "free", "deref", "store", "offset", "addr", "cleanup", "stackalloc", "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "float32", "float64", "bool", "char", "string", "byte", "sbyte", "short", "int", "long", "ulong", "uint", "float", "double", "num"];
export type Keyword = typeof KEYWORDS[number];
export type PrimitiveType = typeof PRIMITIVE_TYPES[number];
export declare const KEYWORD_DOCS: {
    readonly spawn: "Spawns a zero-argument callable. Void callables return Thread; non-void callables return Future<T>.";
    readonly thread_local: "Declares thread-local storage. In alpha.2 this is limited to storage with cross-thread identity, not ordinary stack locals.";
    readonly type: "Declares a type alias. Syntax: `type Name = ExistingType;`";
    readonly var: "Two roles: (1) inferred local binding `var x = expr;`; (2) **alpha.6** untyped parameter `(var name) -> ...` whose value is opaque at compile time and inspected at run time via `typeof`, `isint`, `isstring`, etc. Untyped parameters cannot be varargs and must follow any typed parameters.";
    readonly num: "Built-in generic numeric primitive (alpha.6). A binding written against `num` resolves to whichever numeric primitive (int8…int64, uint8…uint64, float32, float64) the call site requires. Participates in numeric widening.";
};
export declare const ALPHA6_INTRINSICS: {
    readonly typeof: "`typeof(value)` returns a `string` naming the runtime type. Dispatched through the runtime, not constant-folded.";
    readonly isint: "`isint(value)` returns `bool`. Companion to `isfloat`, `isbool`, `isstring`, `isarray`.";
    readonly issametype: "`issametype(x, y)` returns `bool` — true iff x and y share a runtime type tag.";
    readonly car: "`car(arr)` returns the first element of a non-empty array. **alpha.6** also accepts `string`, returning the first byte as `char`. Aborts at runtime on empty input.";
    readonly cdr: "`cdr(arr)` returns a new array containing every element except the first. **alpha.6** also accepts `string`, returning a new `string` with the first byte removed. Aborts at runtime on empty input.";
};
export declare const ALPHA6_PARAM_FORMS: readonly [{
    readonly syntax: "(Type name) -> ...";
    readonly description: "Standard typed parameter.";
}, {
    readonly syntax: "(Type... name) -> ...";
    readonly description: "Varargs typed parameter (must be last).";
}, {
    readonly syntax: "(Type name = expr) -> ...";
    readonly description: "Default-valued parameter (alpha.5).";
}, {
    readonly syntax: "(var name) -> ...";
    readonly description: "Untyped parameter (alpha.6). Type is opaque; inspect at run time via `typeof`/`is*`.";
}, {
    readonly syntax: "(|var name) -> ...";
    readonly description: "Early-return parameter (alpha.6). Writing through the parameter performs a non-local return out of the enclosing call.";
}];
export declare const DECLARATION_DOCS: readonly [{
    readonly name: "Type alias";
    readonly syntax: "type Name = ExistingType;";
    readonly description: "Creates a named alias for an existing type. Type aliases are top-level declarations and may carry normal declaration modifiers.";
}, {
    readonly name: "Thread-local storage";
    readonly syntax: "thread_local Type name = value;";
    readonly description: "Declares thread-local storage. The alpha.2 contract is intentionally narrow and does not turn ordinary stack locals into thread-local variables.";
}];
export declare const NUMERIC_LITERAL_NOTES: readonly ["Numeric literal separators (`_`) are stripped by the tokenizer before parsing, so integer and float grammar stay unchanged. Examples: `1_000`, `0xFF_FF`, `3.141_592`."];
