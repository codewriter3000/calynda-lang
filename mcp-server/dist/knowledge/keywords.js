"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.NUMERIC_LITERAL_NOTES = exports.DECLARATION_DOCS = exports.KEYWORD_DOCS = exports.ALL_RESERVED = exports.BUILTIN_TYPES = exports.PRIMITIVE_TYPES = exports.KEYWORDS = void 0;
exports.KEYWORDS = [
    'package', 'import', 'public', 'private', 'final', 'var', 'start', 'boot',
    'return', 'exit', 'throw', 'null', 'true', 'false', 'void',
    'export', 'as', 'internal', 'static', 'thread_local', 'type', 'union', 'manual', 'arr', 'ptr', 'layout',
    'spawn', 'checked', 'asm', 'malloc', 'calloc', 'realloc', 'free', 'deref', 'store',
    'offset', 'addr', 'cleanup', 'stackalloc',
];
exports.PRIMITIVE_TYPES = [
    'int8', 'int16', 'int32', 'int64',
    'uint8', 'uint16', 'uint32', 'uint64',
    'float32', 'float64',
    'bool', 'char', 'string',
    'byte', 'sbyte', 'short', 'int', 'long', 'ulong', 'uint', 'float', 'double',
];
exports.BUILTIN_TYPES = ['Thread', 'Future<T>', 'Mutex', 'Atomic<T>'];
exports.ALL_RESERVED = [...exports.KEYWORDS, ...exports.PRIMITIVE_TYPES];
exports.KEYWORD_DOCS = {
    spawn: 'Spawns a zero-argument callable. Void callables return Thread; non-void callables return Future<T>.',
    thread_local: 'Declares thread-local storage. In alpha.2 this is limited to storage with cross-thread identity, not ordinary stack locals.',
    type: 'Declares a type alias. Syntax: `type Name = ExistingType;`',
};
exports.DECLARATION_DOCS = [
    {
        name: 'Type alias',
        syntax: 'type Name = ExistingType;',
        description: 'Creates a named alias for an existing type. Type aliases are top-level declarations and may carry normal declaration modifiers.',
    },
    {
        name: 'Thread-local storage',
        syntax: 'thread_local Type name = value;',
        description: 'Declares thread-local storage. The alpha.2 contract is intentionally narrow and does not turn ordinary stack locals into thread-local variables.',
    },
];
exports.NUMERIC_LITERAL_NOTES = [
    'Numeric literal separators (`_`) are stripped by the tokenizer before parsing, so integer and float grammar stay unchanged. Examples: `1_000`, `0xFF_FF`, `3.141_592`.',
];
//# sourceMappingURL=keywords.js.map