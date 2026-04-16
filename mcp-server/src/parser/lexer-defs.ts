export type TokenType =
  | 'keyword' | 'type' | 'identifier'
  | 'integer' | 'float' | 'bool' | 'char' | 'string' | 'null'
  | 'template_literal'
  | 'arrow'
  | 'plus' | 'minus' | 'star' | 'slash' | 'percent'
  | 'plusplus' | 'minusminus'
  | 'ellipsis' | 'underscore'
  | 'amp' | 'pipe' | 'caret' | 'tilde'
  | 'bang' | 'question' | 'colon'
  | 'lt' | 'gt' | 'lteq' | 'gteq' | 'eqeq' | 'bangeq'
  | 'ampamp' | 'pipepipe'
  | 'tildeamp' | 'tildecaret'
  | 'ltlt' | 'gtgt'
  | 'eq' | 'pluseq' | 'minuseq' | 'stareq' | 'slasheq' | 'percenteq'
  | 'ampeq' | 'pipeeq' | 'careteq' | 'ltlteq' | 'gtgteq'
  | 'lparen' | 'rparen' | 'lbrace' | 'rbrace' | 'lbracket' | 'rbracket'
  | 'semicolon' | 'comma' | 'dot'
  | 'eof';

export interface Token {
  type: TokenType;
  value: string;
  line: number;
  column: number;
  offset: number;
}

export class LexError extends Error {
  constructor(message: string, public line: number, public column: number) {
    super(message);
  }
}

export const KEYWORDS = new Set([
  'package', 'import', 'public', 'private', 'final', 'var', 'start', 'boot',
  'return', 'exit', 'throw', 'null', 'true', 'false', 'void',
  'export', 'as', 'internal', 'static', 'thread_local', 'type', 'union', 'manual', 'arr', 'ptr', 'layout',
  'spawn', 'checked', 'asm', 'malloc', 'calloc', 'realloc', 'free', 'deref', 'store',
  'offset', 'addr', 'cleanup', 'stackalloc',
]);

export const PRIMITIVE_TYPES = new Set([
  'int8', 'int16', 'int32', 'int64',
  'uint8', 'uint16', 'uint32', 'uint64',
  'float32', 'float64',
  'bool', 'char', 'string',
  'byte', 'sbyte', 'short', 'int', 'long', 'ulong', 'uint', 'float', 'double',
]);

export interface LexerContext {
  peek(offset?: number): string;
  advance(): string;
  makeToken(type: TokenType, value: string, line: number, column: number, offset: number): Token;
  errors: LexError[];
  pos: number;
  readonly source: string;
}

export function readOperatorToken(ctx: LexerContext, line: number, col: number, offset: number): Token | null {
  const ch = ctx.advance();
  const next = ctx.peek();

  switch (ch) {
    case '-':
      if (next === '>') { ctx.advance(); return ctx.makeToken('arrow', '->', line, col, offset); }
      if (next === '-') { ctx.advance(); return ctx.makeToken('minusminus', '--', line, col, offset); }
      if (next === '=') { ctx.advance(); return ctx.makeToken('minuseq', '-=', line, col, offset); }
      return ctx.makeToken('minus', '-', line, col, offset);
    case '+':
      if (next === '+') { ctx.advance(); return ctx.makeToken('plusplus', '++', line, col, offset); }
      if (next === '=') { ctx.advance(); return ctx.makeToken('pluseq', '+=', line, col, offset); }
      return ctx.makeToken('plus', '+', line, col, offset);
    case '*':
      if (next === '=') { ctx.advance(); return ctx.makeToken('stareq', '*=', line, col, offset); }
      return ctx.makeToken('star', '*', line, col, offset);
    case '/':
      if (next === '=') { ctx.advance(); return ctx.makeToken('slasheq', '/=', line, col, offset); }
      return ctx.makeToken('slash', '/', line, col, offset);
    case '%':
      if (next === '=') { ctx.advance(); return ctx.makeToken('percenteq', '%=', line, col, offset); }
      return ctx.makeToken('percent', '%', line, col, offset);
    case '&':
      if (next === '&') { ctx.advance(); return ctx.makeToken('ampamp', '&&', line, col, offset); }
      if (next === '=') { ctx.advance(); return ctx.makeToken('ampeq', '&=', line, col, offset); }
      return ctx.makeToken('amp', '&', line, col, offset);
    case '|':
      if (next === '|') { ctx.advance(); return ctx.makeToken('pipepipe', '||', line, col, offset); }
      if (next === '=') { ctx.advance(); return ctx.makeToken('pipeeq', '|=', line, col, offset); }
      return ctx.makeToken('pipe', '|', line, col, offset);
    case '^':
      if (next === '=') { ctx.advance(); return ctx.makeToken('careteq', '^=', line, col, offset); }
      return ctx.makeToken('caret', '^', line, col, offset);
    case '~':
      if (next === '&') { ctx.advance(); return ctx.makeToken('tildeamp', '~&', line, col, offset); }
      if (next === '^') { ctx.advance(); return ctx.makeToken('tildecaret', '~^', line, col, offset); }
      return ctx.makeToken('tilde', '~', line, col, offset);
    case '!':
      if (next === '=') { ctx.advance(); return ctx.makeToken('bangeq', '!=', line, col, offset); }
      return ctx.makeToken('bang', '!', line, col, offset);
    case '<':
      if (next === '<') {
        ctx.advance();
        if (ctx.peek() === '=') { ctx.advance(); return ctx.makeToken('ltlteq', '<<=', line, col, offset); }
        return ctx.makeToken('ltlt', '<<', line, col, offset);
      }
      if (next === '=') { ctx.advance(); return ctx.makeToken('lteq', '<=', line, col, offset); }
      return ctx.makeToken('lt', '<', line, col, offset);
    case '>':
      if (next === '>') {
        ctx.advance();
        if (ctx.peek() === '=') { ctx.advance(); return ctx.makeToken('gtgteq', '>>=', line, col, offset); }
        return ctx.makeToken('gtgt', '>>', line, col, offset);
      }
      if (next === '=') { ctx.advance(); return ctx.makeToken('gteq', '>=', line, col, offset); }
      return ctx.makeToken('gt', '>', line, col, offset);
    case '=':
      if (next === '=') { ctx.advance(); return ctx.makeToken('eqeq', '==', line, col, offset); }
      return ctx.makeToken('eq', '=', line, col, offset);
    case '?': return ctx.makeToken('question', '?', line, col, offset);
    case ':': return ctx.makeToken('colon', ':', line, col, offset);
    case '(': return ctx.makeToken('lparen', '(', line, col, offset);
    case ')': return ctx.makeToken('rparen', ')', line, col, offset);
    case '{': return ctx.makeToken('lbrace', '{', line, col, offset);
    case '}': return ctx.makeToken('rbrace', '}', line, col, offset);
    case '[': return ctx.makeToken('lbracket', '[', line, col, offset);
    case ']': return ctx.makeToken('rbracket', ']', line, col, offset);
    case ';': return ctx.makeToken('semicolon', ';', line, col, offset);
    case ',': return ctx.makeToken('comma', ',', line, col, offset);
    case '.':
      if (ctx.peek() === '.' && ctx.peek(1) === '.') { ctx.advance(); ctx.advance(); return ctx.makeToken('ellipsis', '...', line, col, offset); }
      return ctx.makeToken('dot', '.', line, col, offset);
    default:
      ctx.errors.push(new LexError(`Unexpected character: ${ch}`, line, col));
      return null;
  }
}
