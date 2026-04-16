"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseLambdaBody = parseLambdaBody;
exports.parseExpression = parseExpression;
exports.parsePostfix = parsePostfix;
const lexer_1 = require("./lexer");
const parser_1 = require("./parser");
const parser_statements_1 = require("./parser-statements");
const parser_blocks_1 = require("./parser-blocks");
const CALLEE_KEYWORDS = new Set([
    'malloc', 'calloc', 'realloc', 'free', 'deref', 'store',
    'offset', 'addr', 'cleanup', 'stackalloc',
]);
function parseLambdaBody(state) {
    if (state.check('lbrace'))
        return (0, parser_blocks_1.parseBlock)(state);
    return parseExpression(state);
}
function parseExpression(state) {
    if (isLambdaAhead(state))
        return parseLambdaExpression(state);
    return parseAssignmentExpression(state);
}
function isLambdaAhead(state) {
    if (!state.check('lparen'))
        return false;
    let i = 1;
    let depth = 1;
    while (state.pos + i < state.tokens.length && depth > 0) {
        const t = state.tokens[state.pos + i];
        if (t.type === 'lparen')
            depth++;
        else if (t.type === 'rparen')
            depth--;
        i++;
    }
    return state.tokens[state.pos + i]?.type === 'arrow';
}
function parseLambdaExpression(state) {
    const startPos = state.position();
    state.eat('lparen');
    const params = (0, parser_statements_1.parseParameterList)(state);
    state.eat('rparen');
    state.eat('arrow');
    const body = parseLambdaBody(state);
    return { kind: 'LambdaExpression', params, body, start: startPos, end: state.position() };
}
function parseAssignmentExpression(state) {
    const left = (0, parser_statements_1.parseTernaryExpression)(state);
    const tok = state.peek();
    if (parser_1.ASSIGNMENT_OPS.has(tok.value) && (tok.type === 'eq' || tok.type.endsWith('eq'))) {
        const op = state.advance().value;
        const right = parseAssignmentExpression(state);
        return { kind: 'AssignmentExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parsePostfix(state) {
    let expr = parsePrimary(state);
    while (true) {
        if (state.check('lparen')) {
            state.advance();
            const args = [];
            if (!state.check('rparen')) {
                args.push(parseExpression(state));
                while (state.check('comma')) {
                    state.advance();
                    args.push(parseExpression(state));
                }
            }
            state.eat('rparen');
            expr = { kind: 'CallExpression', callee: expr, args, start: expr.start, end: state.position() };
        }
        else if (state.check('lbracket')) {
            state.advance();
            const index = parseExpression(state);
            state.eat('rbracket');
            expr = { kind: 'IndexExpression', object: expr, index, start: expr.start, end: state.position() };
        }
        else if (state.check('dot')) {
            state.advance();
            const member = state.eat('identifier').value;
            expr = { kind: 'MemberExpression', object: expr, member, start: expr.start, end: state.position() };
        }
        else {
            break;
        }
    }
    return expr;
}
function parsePrimary(state) {
    const startPos = state.position();
    const tok = state.peek();
    if (tok.type === 'null') {
        state.advance();
        return { kind: 'NullLiteral', start: startPos, end: state.position() };
    }
    if (tok.type === 'bool') {
        state.advance();
        return { kind: 'BoolLiteral', value: tok.value === 'true', start: startPos, end: state.position() };
    }
    if (tok.type === 'integer') {
        state.advance();
        const raw = tok.value;
        const normalized = raw.replaceAll('_', '');
        let val;
        if (normalized.startsWith('0b') || normalized.startsWith('0B'))
            val = parseInt(normalized.slice(2), 2);
        else if (normalized.startsWith('0x') || normalized.startsWith('0X'))
            val = parseInt(normalized.slice(2), 16);
        else if (normalized.startsWith('0o') || normalized.startsWith('0O'))
            val = parseInt(normalized.slice(2), 8);
        else
            val = parseInt(normalized, 10);
        return { kind: 'IntegerLiteral', value: val, raw, start: startPos, end: state.position() };
    }
    if (tok.type === 'float') {
        state.advance();
        return { kind: 'FloatLiteral', value: parseFloat(tok.value.replaceAll('_', '')), raw: tok.value, start: startPos, end: state.position() };
    }
    if (tok.type === 'string') {
        state.advance();
        const raw = tok.value.slice(1, -1);
        return { kind: 'StringLiteral', value: raw, start: startPos, end: state.position() };
    }
    if (tok.type === 'char') {
        state.advance();
        let val = tok.value.slice(1, -1);
        if (val.startsWith('\\')) {
            switch (val[1]) {
                case 'n':
                    val = '\n';
                    break;
                case 't':
                    val = '\t';
                    break;
                case 'r':
                    val = '\r';
                    break;
                case '\\':
                    val = '\\';
                    break;
                case "'":
                    val = "'";
                    break;
                default: break;
            }
        }
        return { kind: 'CharLiteral', value: val, start: startPos, end: state.position() };
    }
    if (tok.type === 'template_literal') {
        state.advance();
        const parts = parseTemplateParts(tok.value);
        return { kind: 'TemplateLiteral', parts, start: startPos, end: state.position() };
    }
    if (tok.type === 'lbracket') {
        state.advance();
        const elements = [];
        if (!state.check('rbracket')) {
            elements.push(parseExpression(state));
            while (state.check('comma')) {
                state.advance();
                elements.push(parseExpression(state));
            }
        }
        state.eat('rbracket');
        return { kind: 'ArrayLiteral', elements, start: startPos, end: state.position() };
    }
    if (tok.type === 'lparen') {
        state.advance();
        const expr = parseExpression(state);
        state.eat('rparen');
        return expr;
    }
    // Cast expression: PrimitiveType ( expr )
    if (tok.type === 'type' && state.peek(1).type === 'lparen') {
        const typeName = state.advance().value;
        state.advance(); // (
        const value = parseExpression(state);
        state.eat('rparen');
        return { kind: 'CastExpression', targetType: typeName, value, start: startPos, end: state.position() };
    }
    if (tok.type === 'identifier' ||
        (tok.type === 'keyword' && CALLEE_KEYWORDS.has(tok.value))) {
        state.advance();
        return { kind: 'Identifier', name: tok.value, start: startPos, end: state.position() };
    }
    // throw in expression context (e.g. ternary branch): model as unary
    if (tok.type === 'keyword' && tok.value === 'throw') {
        state.advance();
        const value = parseExpression(state);
        return { kind: 'UnaryExpression', operator: 'throw', operand: value, start: startPos, end: state.position() };
    }
    throw new parser_1.ParseError(`Unexpected token '${tok.value}' (${tok.type})`, tok.line, tok.column);
}
function parseTemplateParts(raw) {
    const content = raw.slice(1, -1); // remove surrounding backticks
    const parts = [];
    let i = 0;
    let str = '';
    while (i < content.length) {
        if (content[i] === '$' && content[i + 1] === '{') {
            if (str) {
                parts.push(str);
                str = '';
            }
            i += 2;
            let depth = 1;
            let exprSrc = '';
            while (i < content.length && depth > 0) {
                if (content[i] === '{') {
                    depth++;
                    exprSrc += content[i++];
                }
                else if (content[i] === '}') {
                    depth--;
                    if (depth > 0)
                        exprSrc += content[i];
                    i++;
                }
                else {
                    exprSrc += content[i++];
                }
            }
            try {
                const innerLexer = new lexer_1.Lexer(exprSrc);
                const innerTokens = innerLexer.tokenize();
                const innerState = new parser_1.ParserState(innerTokens);
                const expr = parseExpression(innerState);
                parts.push(expr);
            }
            catch {
                parts.push('${' + exprSrc + '}');
            }
        }
        else if (content[i] === '\\' && i + 1 < content.length) {
            str += content[i] + content[i + 1];
            i += 2;
        }
        else {
            str += content[i++];
        }
    }
    if (str)
        parts.push(str);
    return parts;
}
//# sourceMappingURL=parser-expressions.js.map