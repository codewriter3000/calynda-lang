import * as AST from '../parser/ast';
import { Diagnostic } from './diagnostics';
import { CalyndaType, PrimitiveCalyndaType, typeToString } from './types';

export interface AnalysisResult {
  diagnostics: Diagnostic[];
  symbols: Map<string, CalyndaType>;
}

type Scope = Map<string, CalyndaType>;

class SemanticAnalyzer {
  private diagnostics: Diagnostic[] = [];
  private scopes: Scope[] = [new Map()];
  private globalSymbols: Map<string, CalyndaType> = new Map();

  analyze(program: AST.Program): AnalysisResult {
    for (const decl of program.declarations) {
      this.analyzeTopLevelDecl(decl);
    }
    return { diagnostics: this.diagnostics, symbols: this.globalSymbols };
  }

  private currentScope(): Scope {
    return this.scopes[this.scopes.length - 1];
  }

  private pushScope(): void {
    this.scopes.push(new Map());
  }

  private popScope(): void {
    this.scopes.pop();
  }

  private lookupSymbol(name: string): CalyndaType | undefined {
    for (let i = this.scopes.length - 1; i >= 0; i--) {
      const found = this.scopes[i].get(name);
      if (found) return found;
    }
    return undefined;
  }

  private defineSymbol(name: string, type: CalyndaType, node: AST.ASTNode): void {
    const scope = this.currentScope();
    if (scope.has(name)) {
      this.diagnostics.push({
        severity: 'error',
        message: `Symbol '${name}' is already declared in this scope`,
        line: node.start.line,
        column: node.start.column,
      });
    } else {
      scope.set(name, type);
    }
  }

  private typeFromAnnotation(ann: AST.TypeNode | 'var'): CalyndaType {
    if (ann === 'var') return { kind: 'unknown' };
    switch (ann.kind) {
      case 'PrimitiveType': return { kind: 'primitive', name: ann.name as PrimitiveCalyndaType['name'] };
      case 'VoidType': return { kind: 'void' };
      case 'ArrayType': return { kind: 'array', elementType: this.typeFromAnnotation(ann.elementType) };
      case 'NamedType': return { kind: 'named', name: ann.name, genericArgs: ann.genericArgs.map(a => this.typeFromAnnotation(a)) };
    }
  }

  private analyzeTopLevelDecl(decl: AST.TopLevelDecl): void {
    if (decl.kind === 'StartDecl') {
      this.pushScope();
      for (const param of decl.params) {
        const t = this.typeFromAnnotation(param.typeAnnotation);
        this.defineSymbol(param.name, t, param);
      }
      if (decl.body.kind === 'Block') {
        this.analyzeBlock(decl.body);
      } else {
        this.analyzeExpression(decl.body);
      }
      this.popScope();
    } else if (decl.kind === 'BindingDecl') {
      const type = this.typeFromAnnotation(decl.typeAnnotation);
      this.analyzeExpression(decl.value);
      this.defineSymbol(decl.name, type, decl);
      this.globalSymbols.set(decl.name, type);
    } else if (decl.kind === 'UnionDecl') {
      const unionType: CalyndaType = { kind: 'named', name: decl.name, genericArgs: decl.genericParams.map(() => ({ kind: 'unknown' as const })) };
      this.defineSymbol(decl.name, unionType, decl);
      this.globalSymbols.set(decl.name, unionType);
    } else if (decl.kind === 'AsmDecl') {
      const type = this.typeFromAnnotation(decl.typeAnnotation);
      this.defineSymbol(decl.name, type, decl);
      this.globalSymbols.set(decl.name, type);
    } else if (decl.kind === 'BootDecl') {
      if (decl.body.kind === 'Block') {
        this.pushScope();
        this.analyzeBlock(decl.body);
        this.popScope();
      } else {
        this.analyzeExpression(decl.body);
      }
    }
  }

  private analyzeBlock(block: AST.Block): void {
    for (const stmt of block.statements) {
      this.analyzeStatement(stmt);
    }
  }

  private analyzeStatement(stmt: AST.Statement): void {
    switch (stmt.kind) {
      case 'LocalBindingStatement': {
        const type = this.typeFromAnnotation(stmt.typeAnnotation);
        this.analyzeExpression(stmt.value);
        this.defineSymbol(stmt.name, type, stmt);
        break;
      }
      case 'ReturnStatement':
        if (stmt.value) this.analyzeExpression(stmt.value);
        break;
      case 'ExitStatement':
        break;
      case 'ThrowStatement':
        this.analyzeExpression(stmt.value);
        break;
      case 'ManualStatement':
        this.pushScope();
        this.analyzeBlock(stmt.body);
        this.popScope();
        break;
      case 'ExpressionStatement':
        this.analyzeExpression(stmt.expression);
        break;
    }
  }

  private analyzeExpression(expr: AST.Expression): CalyndaType {
    switch (expr.kind) {
      case 'Identifier': {
        const type = this.lookupSymbol(expr.name);
        if (!type) {
          this.diagnostics.push({
            severity: 'warning',
            message: `Symbol '${expr.name}' may not be defined`,
            line: expr.start.line,
            column: expr.start.column,
          });
          return { kind: 'unknown' };
        }
        return type;
      }
      case 'IntegerLiteral': return { kind: 'primitive', name: 'int32' };
      case 'FloatLiteral': return { kind: 'primitive', name: 'float64' };
      case 'BoolLiteral': return { kind: 'primitive', name: 'bool' };
      case 'CharLiteral': return { kind: 'primitive', name: 'char' };
      case 'StringLiteral': return { kind: 'primitive', name: 'string' };
      case 'TemplateLiteral': return { kind: 'primitive', name: 'string' };
      case 'NullLiteral': return { kind: 'unknown' };
      case 'ArrayLiteral': {
        const elemTypes = expr.elements.map(e => this.analyzeExpression(e));
        const elemType: CalyndaType = elemTypes[0] ?? { kind: 'unknown' };
        return { kind: 'array', elementType: elemType };
      }
      case 'BinaryExpression':
        this.analyzeExpression(expr.left);
        this.analyzeExpression(expr.right);
        return { kind: 'unknown' };
      case 'UnaryExpression':
        this.analyzeExpression(expr.operand);
        return { kind: 'unknown' };
      case 'TernaryExpression':
        this.analyzeExpression(expr.condition);
        this.analyzeExpression(expr.consequent);
        this.analyzeExpression(expr.alternate);
        return { kind: 'unknown' };
      case 'AssignmentExpression':
        this.analyzeExpression(expr.left);
        this.analyzeExpression(expr.right);
        return { kind: 'unknown' };
      case 'CallExpression': {
        this.analyzeExpression(expr.callee);
        for (const arg of expr.args) this.analyzeExpression(arg);
        return { kind: 'unknown' };
      }
      case 'IndexExpression':
        this.analyzeExpression(expr.object);
        this.analyzeExpression(expr.index);
        return { kind: 'unknown' };
      case 'MemberExpression':
        this.analyzeExpression(expr.object);
        return { kind: 'unknown' };
      case 'CastExpression':
        this.analyzeExpression(expr.value);
        return { kind: 'primitive', name: expr.targetType as PrimitiveCalyndaType['name'] };
      case 'LambdaExpression': {
        this.pushScope();
        const paramTypes: CalyndaType[] = [];
        for (const param of expr.params) {
          const t = this.typeFromAnnotation(param.typeAnnotation);
          paramTypes.push(t);
          this.defineSymbol(param.name, t, param);
        }
        let retType: CalyndaType = { kind: 'unknown' };
        if (expr.body.kind === 'Block') {
          this.analyzeBlock(expr.body);
        } else {
          retType = this.analyzeExpression(expr.body);
        }
        this.popScope();
        return { kind: 'lambda', params: paramTypes, returnType: retType };
      }
      default:
        return { kind: 'unknown' };
    }
  }
}

export function analyze(program: AST.Program): AnalysisResult {
  const analyzer = new SemanticAnalyzer();
  return analyzer.analyze(program);
}
