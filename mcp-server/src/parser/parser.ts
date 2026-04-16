import { Token, TokenType } from './lexer-defs';
import * as AST from './ast';

export class ParseError extends Error {
  constructor(message: string, public line: number, public column: number) {
    super(`${line}:${column}: ${message}`);
    this.name = 'ParseError';
  }
}

export interface ParseResult {
  ast: AST.Program | null;
  errors: ParseError[];
}

export const PRIMITIVE_TYPES = new Set([
  'int8', 'int16', 'int32', 'int64',
  'uint8', 'uint16', 'uint32', 'uint64',
  'float32', 'float64',
  'bool', 'char', 'string',
]);

export const MODIFIERS = new Set(['public', 'private', 'final', 'export', 'static', 'internal', 'thread_local']);

export const ASSIGNMENT_OPS = new Set(['=', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<<=', '>>=']);

export class ParserState {
  tokens: Token[];
  pos: number = 0;
  errors: ParseError[] = [];

  constructor(tokens: Token[]) {
    this.tokens = tokens;
  }

  get parseErrors(): ParseError[] { return this.errors; }

  peek(offset = 0): Token {
    const idx = this.pos + offset;
    return this.tokens[idx] ?? { type: 'eof', value: '', line: 0, column: 0, offset: 0 };
  }

  advance(): Token {
    const tok = this.tokens[this.pos];
    if (this.pos < this.tokens.length - 1) this.pos++;
    return tok;
  }

  check(type: TokenType, value?: string): boolean {
    const tok = this.peek();
    if (tok.type !== type) return false;
    if (value !== undefined && tok.value !== value) return false;
    return true;
  }

  eat(type: TokenType, value?: string): Token {
    if (this.check(type, value)) return this.advance();
    const tok = this.peek();
    const expected = value ? `'${value}'` : type;
    this.errors.push(new ParseError(`Expected ${expected}, got '${tok.value}'`, tok.line, tok.column));
    return { type, value: value ?? '', line: tok.line, column: tok.column, offset: tok.offset };
  }

  position(): AST.Position {
    const tok = this.peek();
    return { line: tok.line, column: tok.column, offset: tok.offset };
  }

  tokenToPosition(tok: Token): AST.Position {
    return { line: tok.line, column: tok.column, offset: tok.offset };
  }
}

export { Token, TokenType } from './lexer-defs';
export { Lexer, LexError } from './lexer';

import { parseProgram } from './parser-declarations';

export function parse(tokens: Token[]): ParseResult {
  const state = new ParserState(tokens);
  try {
    const ast = parseProgram(state);
    return { ast, errors: state.parseErrors };
  } catch (e) {
    if (e instanceof ParseError) {
      return { ast: null, errors: [e, ...state.parseErrors] };
    }
    throw e;
  }
}
