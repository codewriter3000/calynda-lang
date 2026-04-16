export const KEYWORDS = [
  'package', 'import', 'public', 'private', 'final', 'var', 'start', 'boot',
  'return', 'exit', 'throw', 'null', 'true', 'false', 'void',
  'export', 'as', 'internal', 'static', 'type', 'union', 'manual', 'arr', 'ptr', 'layout',
  'spawn', 'checked', 'asm', 'malloc', 'calloc', 'realloc', 'free', 'deref', 'store',
  'offset', 'addr', 'cleanup', 'stackalloc',
] as const;

export const PRIMITIVE_TYPES = [
  'int8', 'int16', 'int32', 'int64',
  'uint8', 'uint16', 'uint32', 'uint64',
  'float32', 'float64',
  'bool', 'char', 'string',
  'byte', 'sbyte', 'short', 'int', 'long', 'ulong', 'uint', 'float', 'double',
] as const;

export const BUILTIN_TYPES = ['Thread', 'Mutex'] as const;

export const ALL_RESERVED = [...KEYWORDS, ...PRIMITIVE_TYPES] as const;

export type Keyword = typeof KEYWORDS[number];
export type PrimitiveType = typeof PRIMITIVE_TYPES[number];

export const KEYWORD_DOCS = {
  spawn: 'Spawns a new thread; right-hand side must be a zero-argument void callable. Returns a Thread handle.',
  type: 'Declares a type alias. Syntax: `type Name = ExistingType;`',
} as const;

export const DECLARATION_DOCS = [
  {
    name: 'Type alias',
    syntax: 'type Name = ExistingType;',
    description: 'Creates a named alias for an existing type. Type aliases are top-level declarations and may carry normal declaration modifiers.',
  },
] as const;

export const NUMERIC_LITERAL_NOTES = [
  'Numeric literal separators (`_`) are stripped by the tokenizer before parsing, so integer and float grammar stay unchanged. Examples: `1_000`, `0xFF_FF`, `3.141_592`.',
] as const;
