"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseType = parseType;
exports.parseGenericArgs = parseGenericArgs;
exports.parseParameterList = parseParameterList;
exports.parseTernaryExpression = parseTernaryExpression;
exports.parseUnary = parseUnary;
const parser_expressions_1 = require("./parser-expressions");
function parseType(state) {
    const startPos = state.position();
    if (state.check('keyword', 'void')) {
        state.advance();
        const end = state.position();
        const voidNode = { kind: 'VoidType', start: startPos, end };
        return voidNode;
    }
    if (state.check('keyword', 'arr') || state.check('keyword', 'ptr')) {
        const nameTok = state.advance();
        const genericArgs = parseGenericArgs(state);
        const namedNode = { kind: 'NamedType', name: nameTok.value, genericArgs, start: startPos, end: state.position() };
        return namedNode;
    }
    if (state.check('keyword', 'checked')) {
        const checkedTok = state.advance();
        return {
            kind: 'NamedType',
            name: checkedTok.value,
            genericArgs: [],
            start: state.tokenToPosition(checkedTok),
            end: state.position(),
        };
    }
    let typeNode;
    if (state.check('identifier')) {
        const nameTok = state.advance();
        const genericArgs = state.check('lt') ? parseGenericArgs(state) : [];
        typeNode = { kind: 'NamedType', name: nameTok.value, genericArgs, start: state.tokenToPosition(nameTok), end: state.position() };
    }
    else {
        const typeTok = state.eat('type');
        typeNode = { kind: 'PrimitiveType', name: typeTok.value, start: state.tokenToPosition(typeTok), end: state.position() };
    }
    while (state.check('lbracket')) {
        const arrStart = state.position();
        state.advance();
        let size;
        if (state.check('integer')) {
            size = parseInt(state.advance().value, 10);
        }
        state.eat('rbracket');
        typeNode = { kind: 'ArrayType', elementType: typeNode, size, start: arrStart, end: state.position() };
    }
    return typeNode;
}
function parseGenericArg(state) {
    if (state.check('question')) {
        const pos = state.position();
        state.advance();
        return { kind: 'NamedType', name: '?', genericArgs: [], start: pos, end: state.position() };
    }
    return parseType(state);
}
function parseGenericArgs(state) {
    const args = [];
    state.eat('lt');
    args.push(parseGenericArg(state));
    while (state.check('comma')) {
        state.advance();
        args.push(parseGenericArg(state));
    }
    state.eat('gt');
    return args;
}
function parseParameterList(state) {
    const params = [];
    if (state.check('rparen'))
        return params;
    params.push(parseParameter(state));
    while (state.check('comma')) {
        state.advance();
        params.push(parseParameter(state));
    }
    return params;
}
function parseParameter(state) {
    const startPos = state.position();
    const typeAnnotation = parseType(state);
    const name = state.eat('identifier').value;
    return { kind: 'Parameter', typeAnnotation, name, start: startPos, end: state.position() };
}
function parseTernaryExpression(state) {
    const cond = parseLogicalOr(state);
    if (state.check('question')) {
        state.advance();
        const consequent = (0, parser_expressions_1.parseExpression)(state);
        state.eat('colon');
        const alternate = parseTernaryExpression(state);
        return { kind: 'TernaryExpression', condition: cond, consequent, alternate, start: cond.start, end: alternate.end };
    }
    return cond;
}
function parseLogicalOr(state) {
    let left = parseLogicalAnd(state);
    while (state.check('pipepipe')) {
        const op = state.advance().value;
        const right = parseLogicalAnd(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseLogicalAnd(state) {
    let left = parseBitwiseOr(state);
    while (state.check('ampamp')) {
        const op = state.advance().value;
        const right = parseBitwiseOr(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseBitwiseOr(state) {
    let left = parseBitwiseNand(state);
    while (state.check('pipe')) {
        const op = state.advance().value;
        const right = parseBitwiseNand(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseBitwiseNand(state) {
    let left = parseBitwiseXor(state);
    while (state.check('tildeamp')) {
        const op = state.advance().value;
        const right = parseBitwiseXor(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseBitwiseXor(state) {
    let left = parseBitwiseXnor(state);
    while (state.check('caret')) {
        const op = state.advance().value;
        const right = parseBitwiseXnor(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseBitwiseXnor(state) {
    let left = parseBitwiseAnd(state);
    while (state.check('tildecaret')) {
        const op = state.advance().value;
        const right = parseBitwiseAnd(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseBitwiseAnd(state) {
    let left = parseEquality(state);
    while (state.check('amp')) {
        const op = state.advance().value;
        const right = parseEquality(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseEquality(state) {
    let left = parseRelational(state);
    while (state.check('eqeq') || state.check('bangeq')) {
        const op = state.advance().value;
        const right = parseRelational(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseRelational(state) {
    let left = parseShift(state);
    while (state.check('lt') || state.check('gt') || state.check('lteq') || state.check('gteq')) {
        const op = state.advance().value;
        const right = parseShift(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseShift(state) {
    let left = parseAdditive(state);
    while (state.check('ltlt') || state.check('gtgt')) {
        const op = state.advance().value;
        const right = parseAdditive(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseAdditive(state) {
    let left = parseMultiplicative(state);
    while (state.check('plus') || state.check('minus')) {
        const op = state.advance().value;
        const right = parseMultiplicative(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseMultiplicative(state) {
    let left = parseUnary(state);
    while (state.check('star') || state.check('slash') || state.check('percent')) {
        const op = state.advance().value;
        const right = parseUnary(state);
        left = { kind: 'BinaryExpression', operator: op, left, right, start: left.start, end: right.end };
    }
    return left;
}
function parseUnary(state) {
    const startPos = state.position();
    if (state.check('bang') || state.check('tilde') || state.check('minus') || state.check('plus')) {
        const op = state.advance().value;
        const operand = parseUnary(state);
        return { kind: 'UnaryExpression', operator: op, operand, start: startPos, end: state.position() };
    }
    return (0, parser_expressions_1.parsePostfix)(state);
}
//# sourceMappingURL=parser-statements.js.map