export declare const KEYWORDS: readonly ["package", "import", "public", "private", "final", "var", "start", "boot", "return", "exit", "throw", "null", "true", "false", "void", "export", "as", "internal", "static", "type", "union", "manual", "arr", "ptr", "layout", "spawn", "checked", "asm", "malloc", "calloc", "realloc", "free", "deref", "store", "offset", "addr", "cleanup", "stackalloc"];
export declare const PRIMITIVE_TYPES: readonly ["int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "float32", "float64", "bool", "char", "string", "byte", "sbyte", "short", "int", "long", "ulong", "uint", "float", "double"];
export declare const BUILTIN_TYPES: readonly ["Thread", "Mutex"];
export declare const ALL_RESERVED: readonly ["package", "import", "public", "private", "final", "var", "start", "boot", "return", "exit", "throw", "null", "true", "false", "void", "export", "as", "internal", "static", "type", "union", "manual", "arr", "ptr", "layout", "spawn", "checked", "asm", "malloc", "calloc", "realloc", "free", "deref", "store", "offset", "addr", "cleanup", "stackalloc", "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64", "float32", "float64", "bool", "char", "string", "byte", "sbyte", "short", "int", "long", "ulong", "uint", "float", "double"];
export type Keyword = typeof KEYWORDS[number];
export type PrimitiveType = typeof PRIMITIVE_TYPES[number];
export declare const KEYWORD_DOCS: {
    readonly spawn: "Spawns a new thread; right-hand side must be a zero-argument void callable. Returns a Thread handle.";
    readonly type: "Declares a type alias. Syntax: `type Name = ExistingType;`";
};
export declare const DECLARATION_DOCS: readonly [{
    readonly name: "Type alias";
    readonly syntax: "type Name = ExistingType;";
    readonly description: "Creates a named alias for an existing type. Type aliases are top-level declarations and may carry normal declaration modifiers.";
}];
export declare const NUMERIC_LITERAL_NOTES: readonly ["Numeric literal separators (`_`) are stripped by the tokenizer before parsing, so integer and float grammar stay unchanged. Examples: `1_000`, `0xFF_FF`, `3.141_592`."];
