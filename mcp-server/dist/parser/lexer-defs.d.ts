export type TokenType = 'keyword' | 'type' | 'identifier' | 'integer' | 'float' | 'bool' | 'char' | 'string' | 'null' | 'template_literal' | 'arrow' | 'plus' | 'minus' | 'star' | 'slash' | 'percent' | 'plusplus' | 'minusminus' | 'ellipsis' | 'underscore' | 'amp' | 'pipe' | 'caret' | 'tilde' | 'bang' | 'question' | 'colon' | 'lt' | 'gt' | 'lteq' | 'gteq' | 'eqeq' | 'bangeq' | 'ampamp' | 'pipepipe' | 'tildeamp' | 'tildecaret' | 'ltlt' | 'gtgt' | 'swap' | 'eq' | 'pluseq' | 'minuseq' | 'stareq' | 'slasheq' | 'percenteq' | 'ampeq' | 'pipeeq' | 'careteq' | 'ltlteq' | 'gtgteq' | 'lparen' | 'rparen' | 'lbrace' | 'rbrace' | 'lbracket' | 'rbracket' | 'semicolon' | 'comma' | 'dot' | 'eof';
export interface Token {
    type: TokenType;
    value: string;
    line: number;
    column: number;
    offset: number;
}
export declare class LexError extends Error {
    line: number;
    column: number;
    constructor(message: string, line: number, column: number);
}
export declare const KEYWORDS: Set<string>;
export declare const PRIMITIVE_TYPES: Set<string>;
export interface LexerContext {
    peek(offset?: number): string;
    advance(): string;
    makeToken(type: TokenType, value: string, line: number, column: number, offset: number): Token;
    errors: LexError[];
    pos: number;
    readonly source: string;
}
export declare function readOperatorToken(ctx: LexerContext, line: number, col: number, offset: number): Token | null;
