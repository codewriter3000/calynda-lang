import { Token, TokenType } from './lexer-defs';
import * as AST from './ast';
export declare class ParseError extends Error {
    line: number;
    column: number;
    constructor(message: string, line: number, column: number);
}
export interface ParseResult {
    ast: AST.Program | null;
    errors: ParseError[];
}
export declare const PRIMITIVE_TYPES: Set<string>;
export declare const MODIFIERS: Set<string>;
export declare const ASSIGNMENT_OPS: Set<string>;
export declare class ParserState {
    tokens: Token[];
    pos: number;
    errors: ParseError[];
    constructor(tokens: Token[]);
    get parseErrors(): ParseError[];
    peek(offset?: number): Token;
    advance(): Token;
    check(type: TokenType, value?: string): boolean;
    eat(type: TokenType, value?: string): Token;
    position(): AST.Position;
    tokenToPosition(tok: Token): AST.Position;
}
export { Token, TokenType } from './lexer-defs';
export { Lexer, LexError } from './lexer';
export declare function parse(tokens: Token[]): ParseResult;
