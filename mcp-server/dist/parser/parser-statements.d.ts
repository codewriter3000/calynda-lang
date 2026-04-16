import { ParserState } from './parser';
import * as AST from './ast';
export declare function parseType(state: ParserState): AST.TypeNode;
export declare function parseGenericArgs(state: ParserState): AST.TypeNode[];
export declare function parseParameterList(state: ParserState): AST.Parameter[];
export declare function parseTernaryExpression(state: ParserState): AST.Expression;
export declare function parseUnary(state: ParserState): AST.Expression;
