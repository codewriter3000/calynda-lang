import { TokenType, Token, LexError, LexerContext } from './lexer-defs';
export { TokenType, Token, LexError } from './lexer-defs';
export declare class Lexer implements LexerContext {
    source: string;
    pos: number;
    line: number;
    column: number;
    errors: LexError[];
    constructor(source: string);
    get lexErrors(): LexError[];
    tokenize(): Token[];
    peek(offset?: number): string;
    advance(): string;
    makeToken(type: TokenType, value: string, line: number, column: number, offset: number): Token;
    private skipWhitespaceAndComments;
    private nextToken;
    private readIdentOrKeyword;
    private readNumber;
    private readString;
    private readChar;
    private readTemplateLiteral;
}
