import { ParserState, ParseError, MODIFIERS } from './parser';
import * as AST from './ast';
import { parseType, parseGenericArgs, parseParameterList } from './parser-statements';
import { parseExpression, parseLambdaBody } from './parser-expressions';
import { parseBlock } from './parser-blocks';

export function parseProgram(state: ParserState): AST.Program {
  const start = state.position();
  let packageDecl: AST.PackageDecl | undefined;
  const imports: AST.ImportDecl[] = [];
  const declarations: AST.TopLevelDecl[] = [];

  if (state.check('keyword', 'package')) {
    packageDecl = parsePackageDecl(state);
  }

  while (state.check('keyword', 'import')) {
    imports.push(parseImportDecl(state));
  }

  while (!state.check('eof')) {
    try {
      const decl = parseTopLevelDecl(state);
      if (decl) declarations.push(decl);
    } catch (e) {
      if (e instanceof ParseError) {
        state.errors.push(e);
        while (!state.check('eof') && !state.check('semicolon')) state.advance();
        if (state.check('semicolon')) state.advance();
      } else throw e;
    }
  }

  const end = state.position();
  return { kind: 'Program', packageDecl, imports, declarations, start, end };
}

function parsePackageDecl(state: ParserState): AST.PackageDecl {
  const startTok = state.eat('keyword', 'package');
  const name = parseQualifiedName(state);
  state.eat('semicolon');
  return { kind: 'PackageDecl', name, start: state.tokenToPosition(startTok), end: state.position() };
}

function parseImportDecl(state: ParserState): AST.ImportDecl {
  const startTok = state.eat('keyword', 'import');
  const name = parseQualifiedName(state);
  state.eat('semicolon');
  return { kind: 'ImportDecl', name, start: state.tokenToPosition(startTok), end: state.position() };
}

function parseQualifiedName(state: ParserState): string {
  let name = state.eat('identifier').value;
  while (state.check('dot')) {
    state.advance();
    name += '.' + state.eat('identifier').value;
  }
  return name;
}

function parseTopLevelDecl(state: ParserState): AST.TopLevelDecl | null {
  if (state.check('keyword', 'start')) {
    return parseStartDecl(state);
  }

  if (state.check('keyword', 'boot')) {
    return parseBootDecl(state);
  }

  if (state.check('keyword', 'layout')) {
    return parseLayoutDecl(state);
  }

  const modifiers: string[] = [];
  while (state.check('keyword') && MODIFIERS.has(state.peek().value)) {
    modifiers.push(state.advance().value);
  }

  if (state.check('keyword', 'var') || state.check('type') || state.check('keyword', 'void') ||
      state.check('keyword', 'arr') || state.check('keyword', 'ptr')) {
    return parseBindingDecl(state, modifiers);
  }

  if (state.check('keyword', 'union')) {
    return parseUnionDecl(state, modifiers);
  }

  /* An identifier in type position could be a named type used as a binding type annotation */
  if (state.check('identifier')) {
    return parseBindingDecl(state, modifiers);
  }

  const tok = state.peek();
  throw new ParseError(`Unexpected token '${tok.value}'`, tok.line, tok.column);
}

function parseStartDecl(state: ParserState): AST.StartDecl {
  const startTok = state.eat('keyword', 'start');
  state.eat('lparen');
  const params = parseParameterList(state);
  state.eat('rparen');
  state.eat('arrow');
  const body = parseLambdaBody(state);
  state.eat('semicolon');
  return { kind: 'StartDecl', params, body, start: state.tokenToPosition(startTok), end: state.position() };
}

function parseUnionDecl(state: ParserState, modifiers: string[]): AST.UnionDecl {
  const startPos = state.position();
  state.eat('keyword', 'union');
  const name = state.eat('identifier').value;

  const genericParams: string[] = [];
  if (state.check('lt')) {
    state.advance();
    genericParams.push(state.eat('identifier').value);
    while (state.check('comma')) {
      state.advance();
      genericParams.push(state.eat('identifier').value);
    }
    state.eat('gt');
  }

  state.eat('lbrace');
  const variants: AST.UnionVariant[] = [];
  if (!state.check('rbrace')) {
    variants.push(parseUnionVariant(state));
    while (state.check('comma')) {
      state.advance();
      if (state.check('rbrace')) break;
      variants.push(parseUnionVariant(state));
    }
  }
  state.eat('rbrace');
  state.eat('semicolon');

  return { kind: 'UnionDecl', modifiers, name, genericParams, variants, start: startPos, end: state.position() };
}

function parseUnionVariant(state: ParserState): AST.UnionVariant {
  const name = state.eat('identifier').value;
  let payloadType: AST.TypeNode | undefined;
  if (state.check('lparen')) {
    state.advance();
    payloadType = parseType(state);
    state.eat('rparen');
  }
  return { name, payloadType };
}

function parseLayoutDecl(state: ParserState): AST.LayoutDecl {
  const startTok = state.eat('keyword', 'layout');
  const name = state.eat('identifier').value;
  const fields: Array<{ typeAnnotation: AST.TypeNode; name: string }> = [];

  state.eat('lbrace');
  while (!state.check('rbrace') && !state.check('eof')) {
    const typeAnnotation = parseType(state);
    const fieldName = state.eat('identifier').value;

    state.eat('semicolon');
    fields.push({ typeAnnotation, name: fieldName });
  }
  state.eat('rbrace');
  state.eat('semicolon');
  return { kind: 'LayoutDecl', name, fields, start: state.tokenToPosition(startTok), end: state.position() };
}

function parseBootDecl(state: ParserState): AST.BootDecl {
  const startTok = state.eat('keyword', 'boot');
  state.eat('lparen');
  state.eat('rparen');
  state.eat('arrow');
  const body = parseLambdaBody(state);
  state.eat('semicolon');
  return { kind: 'BootDecl', body, start: state.tokenToPosition(startTok), end: state.position() };
}

function parseBindingDecl(state: ParserState, modifiers: string[]): AST.BindingDecl | AST.AsmDecl {
  const startPos = state.position();
  let typeAnnotation: AST.TypeNode | 'var';

  if (state.check('keyword', 'var')) {
    state.advance();
    typeAnnotation = 'var';
  } else {
    typeAnnotation = parseType(state);
  }

  const name = state.eat('identifier').value;
  state.eat('eq');

  // Check for asm expression: asm(params) -> { ... }
  if (state.check('keyword', 'asm')) {
    state.advance();
    state.eat('lparen');
    const params = parseParameterList(state);
    state.eat('rparen');
    state.eat('arrow');
    state.eat('lbrace');
    let asmBody = '';
    while (!state.check('rbrace') && !state.check('eof')) {
      asmBody += state.advance().value + ' ';
    }
    state.eat('rbrace');
    state.eat('semicolon');
    return {
      kind: 'AsmDecl', modifiers, typeAnnotation: typeAnnotation === 'var'
        ? { kind: 'VoidType', start: startPos, end: startPos } as AST.VoidTypeNode
        : typeAnnotation,
      name, params, body: asmBody.trim(), start: startPos, end: state.position(),
    };
  }

  const value = parseExpression(state);
  state.eat('semicolon');
  return { kind: 'BindingDecl', modifiers, typeAnnotation, name, value, start: startPos, end: state.position() };
}
