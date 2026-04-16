"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseBlock = parseBlock;
const parser_1 = require("./parser");
const parser_statements_1 = require("./parser-statements");
const parser_expressions_1 = require("./parser-expressions");
function startsTypeAnnotation(state) {
    const tok = state.peek();
    return tok.type === 'type' || tok.type === 'identifier' ||
        (tok.type === 'keyword' &&
            (tok.value === 'void' || tok.value === 'arr' || tok.value === 'ptr'));
}
function parseMaybeLocalBinding(state, startPos) {
    const originalPos = state.pos;
    const originalErrorCount = state.errors.length;
    let isFinal = false;
    let typeAnnotation;
    if (state.check('keyword', 'final')) {
        isFinal = true;
        state.advance();
    }
    if (state.check('keyword', 'var')) {
        state.advance();
        typeAnnotation = 'var';
    }
    else if (startsTypeAnnotation(state)) {
        typeAnnotation = (0, parser_statements_1.parseType)(state);
    }
    else {
        return null;
    }
    if (!state.check('identifier') || state.peek(1).type !== 'eq') {
        state.pos = originalPos;
        state.errors.length = originalErrorCount;
        return null;
    }
    const name = state.advance().value;
    state.eat('eq');
    const value = (0, parser_expressions_1.parseExpression)(state);
    state.eat('semicolon');
    return { kind: 'LocalBindingStatement', final: isFinal, typeAnnotation, name, value, start: startPos, end: state.position() };
}
function parseBlock(state) {
    const startPos = state.position();
    state.eat('lbrace');
    const statements = [];
    while (!state.check('rbrace') && !state.check('eof')) {
        try {
            const stmt = parseStatement(state);
            if (stmt)
                statements.push(stmt);
        }
        catch (e) {
            if (e instanceof parser_1.ParseError) {
                state.errors.push(e);
                while (!state.check('eof') && !state.check('semicolon') && !state.check('rbrace'))
                    state.advance();
                if (state.check('semicolon'))
                    state.advance();
            }
            else
                throw e;
        }
    }
    state.eat('rbrace');
    return { kind: 'Block', statements, start: startPos, end: state.position() };
}
function parseStatement(state) {
    const startPos = state.position();
    const tok = state.peek();
    if (tok.type === 'keyword') {
        switch (tok.value) {
            case 'return': {
                state.advance();
                let value;
                if (!state.check('semicolon'))
                    value = (0, parser_expressions_1.parseExpression)(state);
                state.eat('semicolon');
                return { kind: 'ReturnStatement', value, start: startPos, end: state.position() };
            }
            case 'exit': {
                state.advance();
                state.eat('semicolon');
                return { kind: 'ExitStatement', start: startPos, end: state.position() };
            }
            case 'throw': {
                state.advance();
                const value = (0, parser_expressions_1.parseExpression)(state);
                state.eat('semicolon');
                return { kind: 'ThrowStatement', value, start: startPos, end: state.position() };
            }
            case 'var': {
                state.advance();
                const name = state.eat('identifier').value;
                state.eat('eq');
                const value = (0, parser_expressions_1.parseExpression)(state);
                state.eat('semicolon');
                return { kind: 'LocalBindingStatement', final: false, typeAnnotation: 'var', name, value, start: startPos, end: state.position() };
            }
            case 'final': {
                state.advance();
                const typeAnnotation = (0, parser_statements_1.parseType)(state);
                const name = state.eat('identifier').value;
                state.eat('eq');
                const value = (0, parser_expressions_1.parseExpression)(state);
                state.eat('semicolon');
                return { kind: 'LocalBindingStatement', final: true, typeAnnotation, name, value, start: startPos, end: state.position() };
            }
            case 'manual': {
                state.advance();
                if (state.check('keyword', 'checked')) {
                    state.advance();
                }
                const body = parseBlock(state);
                state.eat('semicolon');
                return { kind: 'ManualStatement', body, start: startPos, end: state.position() };
            }
        }
    }
    {
        const binding = parseMaybeLocalBinding(state, startPos);
        if (binding) {
            return binding;
        }
    }
    // Expression statement
    const expr = (0, parser_expressions_1.parseExpression)(state);
    state.eat('semicolon');
    return { kind: 'ExpressionStatement', expression: expr, start: startPos, end: state.position() };
}
//# sourceMappingURL=parser-blocks.js.map