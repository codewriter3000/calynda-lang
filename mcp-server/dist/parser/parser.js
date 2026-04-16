"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.LexError = exports.Lexer = exports.ParserState = exports.ASSIGNMENT_OPS = exports.MODIFIERS = exports.PRIMITIVE_TYPES = exports.ParseError = void 0;
exports.parse = parse;
class ParseError extends Error {
    line;
    column;
    constructor(message, line, column) {
        super(`${line}:${column}: ${message}`);
        this.line = line;
        this.column = column;
        this.name = 'ParseError';
    }
}
exports.ParseError = ParseError;
exports.PRIMITIVE_TYPES = new Set([
    'int8', 'int16', 'int32', 'int64',
    'uint8', 'uint16', 'uint32', 'uint64',
    'float32', 'float64',
    'bool', 'char', 'string',
]);
exports.MODIFIERS = new Set(['public', 'private', 'final', 'export', 'static', 'internal']);
exports.ASSIGNMENT_OPS = new Set(['=', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<<=', '>>=']);
class ParserState {
    tokens;
    pos = 0;
    errors = [];
    constructor(tokens) {
        this.tokens = tokens;
    }
    get parseErrors() { return this.errors; }
    peek(offset = 0) {
        const idx = this.pos + offset;
        return this.tokens[idx] ?? { type: 'eof', value: '', line: 0, column: 0, offset: 0 };
    }
    advance() {
        const tok = this.tokens[this.pos];
        if (this.pos < this.tokens.length - 1)
            this.pos++;
        return tok;
    }
    check(type, value) {
        const tok = this.peek();
        if (tok.type !== type)
            return false;
        if (value !== undefined && tok.value !== value)
            return false;
        return true;
    }
    eat(type, value) {
        if (this.check(type, value))
            return this.advance();
        const tok = this.peek();
        const expected = value ? `'${value}'` : type;
        this.errors.push(new ParseError(`Expected ${expected}, got '${tok.value}'`, tok.line, tok.column));
        return { type, value: value ?? '', line: tok.line, column: tok.column, offset: tok.offset };
    }
    position() {
        const tok = this.peek();
        return { line: tok.line, column: tok.column, offset: tok.offset };
    }
    tokenToPosition(tok) {
        return { line: tok.line, column: tok.column, offset: tok.offset };
    }
}
exports.ParserState = ParserState;
var lexer_1 = require("./lexer");
Object.defineProperty(exports, "Lexer", { enumerable: true, get: function () { return lexer_1.Lexer; } });
Object.defineProperty(exports, "LexError", { enumerable: true, get: function () { return lexer_1.LexError; } });
const parser_declarations_1 = require("./parser-declarations");
function parse(tokens) {
    const state = new ParserState(tokens);
    try {
        const ast = (0, parser_declarations_1.parseProgram)(state);
        return { ast, errors: state.parseErrors };
    }
    catch (e) {
        if (e instanceof ParseError) {
            return { ast: null, errors: [e, ...state.parseErrors] };
        }
        throw e;
    }
}
//# sourceMappingURL=parser.js.map