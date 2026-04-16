"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Lexer = exports.LexError = void 0;
const lexer_defs_1 = require("./lexer-defs");
var lexer_defs_2 = require("./lexer-defs");
Object.defineProperty(exports, "LexError", { enumerable: true, get: function () { return lexer_defs_2.LexError; } });
class Lexer {
    source;
    pos = 0;
    line = 1;
    column = 1;
    errors = [];
    constructor(source) {
        this.source = source;
    }
    get lexErrors() { return this.errors; }
    tokenize() {
        const tokens = [];
        while (this.pos < this.source.length) {
            this.skipWhitespaceAndComments();
            if (this.pos >= this.source.length)
                break;
            const tok = this.nextToken();
            if (tok)
                tokens.push(tok);
        }
        tokens.push({ type: 'eof', value: '', line: this.line, column: this.column, offset: this.pos });
        return tokens;
    }
    peek(offset = 0) {
        return this.source[this.pos + offset] ?? '';
    }
    advance() {
        const ch = this.source[this.pos++];
        if (ch === '\n') {
            this.line++;
            this.column = 1;
        }
        else {
            this.column++;
        }
        return ch;
    }
    makeToken(type, value, line, column, offset) {
        return { type, value, line, column, offset };
    }
    skipWhitespaceAndComments() {
        while (this.pos < this.source.length) {
            const ch = this.peek();
            if (ch === ' ' || ch === '\t' || ch === '\r' || ch === '\n') {
                this.advance();
            }
            else if (ch === '/' && this.peek(1) === '/') {
                while (this.pos < this.source.length && this.peek() !== '\n')
                    this.advance();
            }
            else if (ch === '/' && this.peek(1) === '*') {
                this.advance();
                this.advance();
                while (this.pos < this.source.length) {
                    if (this.peek() === '*' && this.peek(1) === '/') {
                        this.advance();
                        this.advance();
                        break;
                    }
                    this.advance();
                }
            }
            else {
                break;
            }
        }
    }
    nextToken() {
        const startLine = this.line;
        const startCol = this.column;
        const startOffset = this.pos;
        const ch = this.peek();
        if (ch === '`')
            return this.readTemplateLiteral(startLine, startCol, startOffset);
        if (ch === '"')
            return this.readString(startLine, startCol, startOffset);
        if (ch === "'")
            return this.readChar(startLine, startCol, startOffset);
        if (ch >= '0' && ch <= '9')
            return this.readNumber(startLine, startCol, startOffset);
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch === '_') {
            return this.readIdentOrKeyword(startLine, startCol, startOffset);
        }
        return (0, lexer_defs_1.readOperatorToken)(this, startLine, startCol, startOffset);
    }
    readIdentOrKeyword(line, col, offset) {
        let value = '';
        while (this.pos < this.source.length) {
            const c = this.peek();
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c === '_') {
                value += this.advance();
            }
            else
                break;
        }
        if (value === '_')
            return this.makeToken('underscore', value, line, col, offset);
        if (value === 'true' || value === 'false')
            return this.makeToken('bool', value, line, col, offset);
        if (value === 'null')
            return this.makeToken('null', value, line, col, offset);
        if (lexer_defs_1.KEYWORDS.has(value))
            return this.makeToken('keyword', value, line, col, offset);
        if (lexer_defs_1.PRIMITIVE_TYPES.has(value))
            return this.makeToken('type', value, line, col, offset);
        return this.makeToken('identifier', value, line, col, offset);
    }
    readNumber(line, col, offset) {
        let value = '';
        const ch = this.peek();
        if (ch === '0') {
            const next = this.peek(1);
            if (next === 'b' || next === 'B') {
                value += this.advance() + this.advance();
                while (/[01]/.test(this.peek()))
                    value += this.advance();
                return this.makeToken('integer', value, line, col, offset);
            }
            if (next === 'x' || next === 'X') {
                value += this.advance() + this.advance();
                while (/[0-9a-fA-F]/.test(this.peek()))
                    value += this.advance();
                return this.makeToken('integer', value, line, col, offset);
            }
            if (next === 'o' || next === 'O') {
                value += this.advance() + this.advance();
                while (/[0-7]/.test(this.peek()))
                    value += this.advance();
                return this.makeToken('integer', value, line, col, offset);
            }
        }
        while (/[0-9]/.test(this.peek()))
            value += this.advance();
        if (this.peek() === '.' && /[0-9]/.test(this.peek(1))) {
            value += this.advance();
            while (/[0-9]/.test(this.peek()))
                value += this.advance();
            if (this.peek() === 'e' || this.peek() === 'E') {
                value += this.advance();
                if (this.peek() === '+' || this.peek() === '-')
                    value += this.advance();
                while (/[0-9]/.test(this.peek()))
                    value += this.advance();
            }
            return this.makeToken('float', value, line, col, offset);
        }
        return this.makeToken('integer', value, line, col, offset);
    }
    readString(line, col, offset) {
        let value = '"';
        this.advance();
        while (this.pos < this.source.length && this.peek() !== '"') {
            if (this.peek() === '\\') {
                value += this.advance();
                if (this.pos < this.source.length)
                    value += this.advance();
            }
            else {
                value += this.advance();
            }
        }
        if (this.pos < this.source.length) {
            value += this.advance();
        }
        return this.makeToken('string', value, line, col, offset);
    }
    readChar(line, col, offset) {
        let value = "'";
        this.advance();
        if (this.peek() === '\\' && this.pos + 1 < this.source.length) {
            value += this.advance() + this.advance();
        }
        else if (this.pos < this.source.length && this.peek() !== "'") {
            value += this.advance();
        }
        if (this.pos < this.source.length && this.peek() === "'") {
            value += this.advance();
        }
        return this.makeToken('char', value, line, col, offset);
    }
    readTemplateLiteral(line, col, offset) {
        let value = '`';
        this.advance();
        let depth = 0;
        while (this.pos < this.source.length) {
            const c = this.peek();
            if (c === '`' && depth === 0) {
                value += this.advance();
                break;
            }
            if (c === '$' && this.peek(1) === '{') {
                value += this.advance() + this.advance();
                depth++;
                continue;
            }
            if (c === '{') {
                depth++;
                value += this.advance();
                continue;
            }
            if (c === '}') {
                if (depth > 0) {
                    depth--;
                }
                value += this.advance();
                continue;
            }
            if (c === '\\') {
                value += this.advance();
                if (this.pos < this.source.length)
                    value += this.advance();
                continue;
            }
            value += this.advance();
        }
        return this.makeToken('template_literal', value, line, col, offset);
    }
}
exports.Lexer = Lexer;
//# sourceMappingURL=lexer.js.map