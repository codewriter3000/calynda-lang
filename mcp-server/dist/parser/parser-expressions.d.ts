import * as AST from './ast';
import { ParserState } from './parser';
export declare function parseLambdaBody(state: ParserState): AST.Block | AST.Expression;
export declare function parseExpression(state: ParserState): AST.Expression;
export declare function parsePostfix(state: ParserState): AST.Expression;
