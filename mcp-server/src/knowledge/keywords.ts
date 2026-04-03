export const KEYWORDS = [
  'package', 'import', 'public', 'private', 'final', 'var', 'start',
  'return', 'exit', 'throw', 'null', 'true', 'false', 'void',
  'export', 'as', 'internal', 'static', 'union', 'manual', 'arr',
  'malloc', 'calloc', 'realloc', 'free',
] as const;

export const PRIMITIVE_TYPES = [
  'int8', 'int16', 'int32', 'int64',
  'uint8', 'uint16', 'uint32', 'uint64',
  'float32', 'float64',
  'bool', 'char', 'string',
  'byte', 'sbyte', 'short', 'int', 'long', 'ulong', 'uint', 'float', 'double',
] as const;

export const ALL_RESERVED = [...KEYWORDS, ...PRIMITIVE_TYPES] as const;

export type Keyword = typeof KEYWORDS[number];
export type PrimitiveType = typeof PRIMITIVE_TYPES[number];
