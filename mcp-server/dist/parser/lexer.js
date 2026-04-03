"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Lexer = exports.LexError = void 0;
const KEYWORDS = new Set([
    'package', 'import', 'public', 'private', 'final', 'var', 'start',
    'return', 'exit', 'throw', 'null', 'true', 'false', 'void',
    'export', 'as', 'internal', 'static', 'union', 'manual', 'arr',
    'malloc', 'calloc', 'realloc', 'free',
]);
const PRIMITIVE_TYPES = new Set([
    'int8', 'int16', 'int32', 'int64',
    'uint8', 'uint16', 'uint32', 'uint64',
    'float32', 'float64',
    'bool', 'char', 'string',
    'byte', 'sbyte', 'short', 'int', 'long', 'ulong', 'uint', 'float', 'double',
]);
class LexError extends Error {
    line;
    column;
    constructor(message, line, column) {
        super(message);
        this.line = line;
        this.column = column;
    }
}
exports.LexError = LexError;
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
        return this.readOperator(startLine, startCol, startOffset);
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
        if (KEYWORDS.has(value))
            return this.makeToken('keyword', value, line, col, offset);
        if (PRIMITIVE_TYPES.has(value))
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
    readOperator(line, col, offset) {
        const ch = this.advance();
        const next = this.peek();
        switch (ch) {
            case '-':
                if (next === '>') {
                    this.advance();
                    return this.makeToken('arrow', '->', line, col, offset);
                }
                if (next === '-') {
                    this.advance();
                    return this.makeToken('minusminus', '--', line, col, offset);
                }
                if (next === '=') {
                    this.advance();
                    return this.makeToken('minuseq', '-=', line, col, offset);
                }
                return this.makeToken('minus', '-', line, col, offset);
            case '+':
                if (next === '+') {
                    this.advance();
                    return this.makeToken('plusplus', '++', line, col, offset);
                }
                if (next === '=') {
                    this.advance();
                    return this.makeToken('pluseq', '+=', line, col, offset);
                }
                return this.makeToken('plus', '+', line, col, offset);
            case '*':
                if (next === '=') {
                    this.advance();
                    return this.makeToken('stareq', '*=', line, col, offset);
                }
                return this.makeToken('star', '*', line, col, offset);
            case '/':
                if (next === '=') {
                    this.advance();
                    return this.makeToken('slasheq', '/=', line, col, offset);
                }
                return this.makeToken('slash', '/', line, col, offset);
            case '%':
                if (next === '=') {
                    this.advance();
                    return this.makeToken('percenteq', '%=', line, col, offset);
                }
                return this.makeToken('percent', '%', line, col, offset);
            case '&':
                if (next === '&') {
                    this.advance();
                    return this.makeToken('ampamp', '&&', line, col, offset);
                }
                if (next === '=') {
                    this.advance();
                    return this.makeToken('ampeq', '&=', line, col, offset);
                }
                return this.makeToken('amp', '&', line, col, offset);
            case '|':
                if (next === '|') {
                    this.advance();
                    return this.makeToken('pipepipe', '||', line, col, offset);
                }
                if (next === '=') {
                    this.advance();
                    return this.makeToken('pipeeq', '|=', line, col, offset);
                }
                return this.makeToken('pipe', '|', line, col, offset);
            case '^':
                if (next === '=') {
                    this.advance();
                    return this.makeToken('careteq', '^=', line, col, offset);
                }
                return this.makeToken('caret', '^', line, col, offset);
            case '~':
                if (next === '&') {
                    this.advance();
                    return this.makeToken('tildeamp', '~&', line, col, offset);
                }
                if (next === '^') {
                    this.advance();
                    return this.makeToken('tildecaret', '~^', line, col, offset);
                }
                return this.makeToken('tilde', '~', line, col, offset);
            case '!':
                if (next === '=') {
                    this.advance();
                    return this.makeToken('bangeq', '!=', line, col, offset);
                }
                return this.makeToken('bang', '!', line, col, offset);
            case '<':
                if (next === '<') {
                    this.advance();
                    if (this.peek() === '=') {
                        this.advance();
                        return this.makeToken('ltlteq', '<<=', line, col, offset);
                    }
                    return this.makeToken('ltlt', '<<', line, col, offset);
                }
                if (next === '=') {
                    this.advance();
                    return this.makeToken('lteq', '<=', line, col, offset);
                }
                return this.makeToken('lt', '<', line, col, offset);
            case '>':
                if (next === '>') {
                    this.advance();
                    if (this.peek() === '=') {
                        this.advance();
                        return this.makeToken('gtgteq', '>>=', line, col, offset);
                    }
                    return this.makeToken('gtgt', '>>', line, col, offset);
                }
                if (next === '=') {
                    this.advance();
                    return this.makeToken('gteq', '>=', line, col, offset);
                }
                return this.makeToken('gt', '>', line, col, offset);
            case '=':
                if (next === '=') {
                    this.advance();
                    return this.makeToken('eqeq', '==', line, col, offset);
                }
                return this.makeToken('eq', '=', line, col, offset);
            case '?': return this.makeToken('question', '?', line, col, offset);
            case ':': return this.makeToken('colon', ':', line, col, offset);
            case '(': return this.makeToken('lparen', '(', line, col, offset);
            case ')': return this.makeToken('rparen', ')', line, col, offset);
            case '{': return this.makeToken('lbrace', '{', line, col, offset);
            case '}': return this.makeToken('rbrace', '}', line, col, offset);
            case '[': return this.makeToken('lbracket', '[', line, col, offset);
            case ']': return this.makeToken('rbracket', ']', line, col, offset);
            case ';': return this.makeToken('semicolon', ';', line, col, offset);
            case ',': return this.makeToken('comma', ',', line, col, offset);
            case '.':
                if (this.peek() === '.' && this.peek(1) === '.') {
                    this.advance();
                    this.advance();
                    return this.makeToken('ellipsis', '...', line, col, offset);
                }
                return this.makeToken('dot', '.', line, col, offset);
            default:
                this.errors.push(new LexError(`Unexpected character: ${ch}`, line, col));
                return null;
        }
    }
}
exports.Lexer = Lexer;
//# sourceMappingURL=lexer.js.map