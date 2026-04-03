export type TokenType = 'keyword' | 'type' | 'identifier' | 'integer' | 'float' | 'bool' | 'char' | 'string' | 'null' | 'template_literal' | 'arrow' | 'plus' | 'minus' | 'star' | 'slash' | 'percent' | 'plusplus' | 'minusminus' | 'ellipsis' | 'underscore' | 'amp' | 'pipe' | 'caret' | 'tilde' | 'bang' | 'question' | 'colon' | 'lt' | 'gt' | 'lteq' | 'gteq' | 'eqeq' | 'bangeq' | 'ampamp' | 'pipepipe' | 'tildeamp' | 'tildecaret' | 'ltlt' | 'gtgt' | 'eq' | 'pluseq' | 'minuseq' | 'stareq' | 'slasheq' | 'percenteq' | 'ampeq' | 'pipeeq' | 'careteq' | 'ltlteq' | 'gtgteq' | 'lparen' | 'rparen' | 'lbrace' | 'rbrace' | 'lbracket' | 'rbracket' | 'semicolon' | 'comma' | 'dot' | 'eof';
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
export declare class Lexer {
    private source;
    private pos;
    private line;
    private column;
    private errors;
    constructor(source: string);
    get lexErrors(): LexError[];
    tokenize(): Token[];
    private peek;
    private advance;
    private makeToken;
    private skipWhitespaceAndComments;
    private nextToken;
    private readIdentOrKeyword;
    private readNumber;
    private readString;
    private readChar;
    private readTemplateLiteral;
    private readOperator;
}
