"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.analyze = analyze;
const BUILTIN_SYMBOLS = [
    ['Thread', { kind: 'named', name: 'Thread', genericArgs: [] }],
    ['Future', { kind: 'named', name: 'Future', genericArgs: [] }],
    ['Mutex', { kind: 'named', name: 'Mutex', genericArgs: [] }],
    ['Atomic', { kind: 'named', name: 'Atomic', genericArgs: [] }],
];
class SemanticAnalyzer {
    diagnostics = [];
    scopes = [new Map(BUILTIN_SYMBOLS)];
    globalSymbols = new Map(BUILTIN_SYMBOLS);
    analyze(program) {
        for (const decl of program.declarations) {
            this.analyzeTopLevelDecl(decl);
        }
        return { diagnostics: this.diagnostics, symbols: this.globalSymbols };
    }
    currentScope() {
        return this.scopes[this.scopes.length - 1];
    }
    pushScope() {
        this.scopes.push(new Map());
    }
    popScope() {
        this.scopes.pop();
    }
    lookupSymbol(name) {
        for (let i = this.scopes.length - 1; i >= 0; i--) {
            const found = this.scopes[i].get(name);
            if (found)
                return found;
        }
        return undefined;
    }
    defineSymbol(name, type, node) {
        const scope = this.currentScope();
        if (scope.has(name)) {
            this.diagnostics.push({
                severity: 'error',
                message: `Symbol '${name}' is already declared in this scope`,
                line: node.start.line,
                column: node.start.column,
            });
        }
        else {
            scope.set(name, type);
        }
    }
    typeFromAnnotation(ann) {
        if (ann === 'var')
            return { kind: 'unknown' };
        switch (ann.kind) {
            case 'PrimitiveType': return { kind: 'primitive', name: ann.name };
            case 'VoidType': return { kind: 'void' };
            case 'ArrayType': return { kind: 'array', elementType: this.typeFromAnnotation(ann.elementType) };
            case 'NamedType': return { kind: 'named', name: ann.name, genericArgs: ann.genericArgs.map(a => this.typeFromAnnotation(a)) };
        }
    }
    isRecursiveTopLevelLambdaBinding(decl) {
        return decl.typeAnnotation !== 'var' && decl.value.kind === 'LambdaExpression';
    }
    analyzeTopLevelDecl(decl) {
        if (decl.kind === 'StartDecl') {
            this.pushScope();
            for (const param of decl.params) {
                const t = this.typeFromAnnotation(param.typeAnnotation);
                this.defineSymbol(param.name, t, param);
            }
            if (decl.body.kind === 'Block') {
                this.analyzeBlock(decl.body);
            }
            else {
                this.analyzeExpression(decl.body);
            }
            this.popScope();
        }
        else if (decl.kind === 'BindingDecl') {
            const type = this.typeFromAnnotation(decl.typeAnnotation);
            const isRecursiveLambdaBinding = this.isRecursiveTopLevelLambdaBinding(decl);
            if (isRecursiveLambdaBinding) {
                this.defineSymbol(decl.name, type, decl);
                this.globalSymbols.set(decl.name, type);
            }
            this.analyzeExpression(decl.value);
            if (!isRecursiveLambdaBinding) {
                this.defineSymbol(decl.name, type, decl);
            }
            this.globalSymbols.set(decl.name, type);
        }
        else if (decl.kind === 'TypeAliasDecl') {
            const aliasType = this.typeFromAnnotation(decl.target);
            this.defineSymbol(decl.name, aliasType, decl);
            this.globalSymbols.set(decl.name, aliasType);
        }
        else if (decl.kind === 'UnionDecl') {
            const unionType = { kind: 'named', name: decl.name, genericArgs: decl.genericParams.map(() => ({ kind: 'unknown' })) };
            this.defineSymbol(decl.name, unionType, decl);
            this.globalSymbols.set(decl.name, unionType);
        }
        else if (decl.kind === 'LayoutDecl') {
            const layoutType = { kind: 'named', name: decl.name, genericArgs: [] };
            this.defineSymbol(decl.name, layoutType, decl);
            this.globalSymbols.set(decl.name, layoutType);
        }
        else if (decl.kind === 'AsmDecl') {
            const type = this.typeFromAnnotation(decl.typeAnnotation);
            this.defineSymbol(decl.name, type, decl);
            this.globalSymbols.set(decl.name, type);
        }
        else if (decl.kind === 'BootDecl') {
            if (decl.body.kind === 'Block') {
                this.pushScope();
                this.analyzeBlock(decl.body);
                this.popScope();
            }
            else {
                this.analyzeExpression(decl.body);
            }
        }
    }
    analyzeBlock(block) {
        for (const stmt of block.statements) {
            this.analyzeStatement(stmt);
        }
    }
    analyzeStatement(stmt) {
        switch (stmt.kind) {
            case 'LocalBindingStatement': {
                const type = this.typeFromAnnotation(stmt.typeAnnotation);
                this.analyzeExpression(stmt.value);
                this.defineSymbol(stmt.name, type, stmt);
                break;
            }
            case 'ReturnStatement':
                if (stmt.value)
                    this.analyzeExpression(stmt.value);
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
    analyzeExpression(expr) {
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
                const elemType = elemTypes[0] ?? { kind: 'unknown' };
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
                for (const arg of expr.args)
                    this.analyzeExpression(arg);
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
                return { kind: 'primitive', name: expr.targetType };
            case 'LambdaExpression': {
                this.pushScope();
                const paramTypes = [];
                for (const param of expr.params) {
                    const t = this.typeFromAnnotation(param.typeAnnotation);
                    paramTypes.push(t);
                    this.defineSymbol(param.name, t, param);
                }
                let retType = { kind: 'unknown' };
                if (expr.body.kind === 'Block') {
                    this.analyzeBlock(expr.body);
                }
                else {
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
function analyze(program) {
    const analyzer = new SemanticAnalyzer();
    return analyzer.analyze(program);
}
//# sourceMappingURL=semantic.js.map