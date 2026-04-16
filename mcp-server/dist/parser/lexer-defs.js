"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PRIMITIVE_TYPES = exports.KEYWORDS = exports.LexError = void 0;
exports.readOperatorToken = readOperatorToken;
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
exports.KEYWORDS = new Set([
    'package', 'import', 'public', 'private', 'final', 'var', 'start', 'boot',
    'return', 'exit', 'throw', 'null', 'true', 'false', 'void',
    'export', 'as', 'internal', 'static', 'union', 'manual', 'arr', 'ptr', 'layout',
    'checked', 'asm', 'malloc', 'calloc', 'realloc', 'free', 'deref', 'store',
    'offset', 'addr', 'cleanup', 'stackalloc',
]);
exports.PRIMITIVE_TYPES = new Set([
    'int8', 'int16', 'int32', 'int64',
    'uint8', 'uint16', 'uint32', 'uint64',
    'float32', 'float64',
    'bool', 'char', 'string',
    'byte', 'sbyte', 'short', 'int', 'long', 'ulong', 'uint', 'float', 'double',
]);
function readOperatorToken(ctx, line, col, offset) {
    const ch = ctx.advance();
    const next = ctx.peek();
    switch (ch) {
        case '-':
            if (next === '>') {
                ctx.advance();
                return ctx.makeToken('arrow', '->', line, col, offset);
            }
            if (next === '-') {
                ctx.advance();
                return ctx.makeToken('minusminus', '--', line, col, offset);
            }
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('minuseq', '-=', line, col, offset);
            }
            return ctx.makeToken('minus', '-', line, col, offset);
        case '+':
            if (next === '+') {
                ctx.advance();
                return ctx.makeToken('plusplus', '++', line, col, offset);
            }
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('pluseq', '+=', line, col, offset);
            }
            return ctx.makeToken('plus', '+', line, col, offset);
        case '*':
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('stareq', '*=', line, col, offset);
            }
            return ctx.makeToken('star', '*', line, col, offset);
        case '/':
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('slasheq', '/=', line, col, offset);
            }
            return ctx.makeToken('slash', '/', line, col, offset);
        case '%':
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('percenteq', '%=', line, col, offset);
            }
            return ctx.makeToken('percent', '%', line, col, offset);
        case '&':
            if (next === '&') {
                ctx.advance();
                return ctx.makeToken('ampamp', '&&', line, col, offset);
            }
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('ampeq', '&=', line, col, offset);
            }
            return ctx.makeToken('amp', '&', line, col, offset);
        case '|':
            if (next === '|') {
                ctx.advance();
                return ctx.makeToken('pipepipe', '||', line, col, offset);
            }
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('pipeeq', '|=', line, col, offset);
            }
            return ctx.makeToken('pipe', '|', line, col, offset);
        case '^':
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('careteq', '^=', line, col, offset);
            }
            return ctx.makeToken('caret', '^', line, col, offset);
        case '~':
            if (next === '&') {
                ctx.advance();
                return ctx.makeToken('tildeamp', '~&', line, col, offset);
            }
            if (next === '^') {
                ctx.advance();
                return ctx.makeToken('tildecaret', '~^', line, col, offset);
            }
            return ctx.makeToken('tilde', '~', line, col, offset);
        case '!':
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('bangeq', '!=', line, col, offset);
            }
            return ctx.makeToken('bang', '!', line, col, offset);
        case '<':
            if (next === '<') {
                ctx.advance();
                if (ctx.peek() === '=') {
                    ctx.advance();
                    return ctx.makeToken('ltlteq', '<<=', line, col, offset);
                }
                return ctx.makeToken('ltlt', '<<', line, col, offset);
            }
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('lteq', '<=', line, col, offset);
            }
            return ctx.makeToken('lt', '<', line, col, offset);
        case '>':
            if (next === '>') {
                ctx.advance();
                if (ctx.peek() === '=') {
                    ctx.advance();
                    return ctx.makeToken('gtgteq', '>>=', line, col, offset);
                }
                return ctx.makeToken('gtgt', '>>', line, col, offset);
            }
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('gteq', '>=', line, col, offset);
            }
            return ctx.makeToken('gt', '>', line, col, offset);
        case '=':
            if (next === '=') {
                ctx.advance();
                return ctx.makeToken('eqeq', '==', line, col, offset);
            }
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
            if (ctx.peek() === '.' && ctx.peek(1) === '.') {
                ctx.advance();
                ctx.advance();
                return ctx.makeToken('ellipsis', '...', line, col, offset);
            }
            return ctx.makeToken('dot', '.', line, col, offset);
        default:
            ctx.errors.push(new LexError(`Unexpected character: ${ch}`, line, col));
            return null;
    }
}
//# sourceMappingURL=lexer-defs.js.map