import { ParserState, ParseError } from './parser';
import * as AST from './ast';
import { parseType } from './parser-statements';
import { parseExpression } from './parser-expressions';

export function parseBlock(state: ParserState): AST.Block {
  const startPos = state.position();
  state.eat('lbrace');
  const statements: AST.Statement[] = [];
  while (!state.check('rbrace') && !state.check('eof')) {
    try {
      const stmt = parseStatement(state);
      if (stmt) statements.push(stmt);
    } catch (e) {
      if (e instanceof ParseError) {
        state.errors.push(e);
        while (!state.check('eof') && !state.check('semicolon') && !state.check('rbrace')) state.advance();
        if (state.check('semicolon')) state.advance();
      } else throw e;
    }
  }
  state.eat('rbrace');
  return { kind: 'Block', statements, start: startPos, end: state.position() };
}

function parseStatement(state: ParserState): AST.Statement {
  const startPos = state.position();
  const tok = state.peek();

  if (tok.type === 'keyword') {
    switch (tok.value) {
      case 'return': {
        state.advance();
        let value: AST.Expression | undefined;
        if (!state.check('semicolon')) value = parseExpression(state);
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
        const value = parseExpression(state);
        state.eat('semicolon');
        return { kind: 'ThrowStatement', value, start: startPos, end: state.position() };
      }
      case 'var': {
        state.advance();
        const name = state.eat('identifier').value;
        state.eat('eq');
        const value = parseExpression(state);
        state.eat('semicolon');
        return { kind: 'LocalBindingStatement', final: false, typeAnnotation: 'var', name, value, start: startPos, end: state.position() };
      }
      case 'final': {
        state.advance();
        const typeAnnotation = parseType(state);
        const name = state.eat('identifier').value;
        state.eat('eq');
        const value = parseExpression(state);
        state.eat('semicolon');
        return { kind: 'LocalBindingStatement', final: true, typeAnnotation, name, value, start: startPos, end: state.position() };
      }
      case 'manual': {
        state.advance();
        const body = parseBlock(state);
        state.eat('semicolon');
        return { kind: 'ManualStatement', body, start: startPos, end: state.position() };
      }
    }
  }

  // Check for local binding: type identifier = expr;
  if ((tok.type === 'type' || (tok.type === 'keyword' && tok.value === 'void'))
    && state.peek(1).type === 'identifier'
    && state.peek(2).type !== 'lparen') {
    const typeAnnotation = parseType(state);
    if (state.check('identifier') && (state.peek(1).type === 'eq' || state.peek(1).type === 'semicolon')) {
      const name = state.advance().value;
      state.eat('eq');
      const value = parseExpression(state);
      state.eat('semicolon');
      return { kind: 'LocalBindingStatement', final: false, typeAnnotation, name, value, start: startPos, end: state.position() };
    }
    throw new ParseError(`Unexpected type annotation in statement position`, tok.line, tok.column);
  }

  // Expression statement
  const expr = parseExpression(state);
  state.eat('semicolon');
  return { kind: 'ExpressionStatement', expression: expr, start: startPos, end: state.position() };
}
