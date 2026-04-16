"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getCompletions = getCompletions;
const keywords_1 = require("../knowledge/keywords");
function getCompletions(input) {
    const prefix = getWordAtCursor(input.code, input.cursorOffset);
    const items = [];
    for (const kw of keywords_1.KEYWORDS) {
        if (kw.startsWith(prefix)) {
            items.push({ label: kw, kind: 'keyword', detail: 'keyword' });
        }
    }
    for (const t of keywords_1.PRIMITIVE_TYPES) {
        if (t.startsWith(prefix)) {
            items.push({ label: t, kind: 'type', detail: 'primitive type' });
        }
    }
    for (const t of keywords_1.BUILTIN_TYPES) {
        if (t.startsWith(prefix)) {
            items.push({ label: t, kind: 'type', detail: 'built-in type' });
        }
    }
    if ('start'.startsWith(prefix)) {
        items.push({
            label: 'start',
            kind: 'snippet',
            detail: 'Entry point',
            insertText: 'start(string[] args) -> {\n    \n    return 0;\n};',
        });
    }
    if ('union'.startsWith(prefix)) {
        items.push({
            label: 'union',
            kind: 'snippet',
            detail: 'Tagged union declaration',
            insertText: 'union ${1:Name}<${2:T}> { ${3:Variant}(${4:T}) };',
        });
    }
    if ('type'.startsWith(prefix)) {
        items.push({
            label: 'type',
            kind: 'snippet',
            detail: 'Type alias declaration',
            insertText: 'type ${1:Name} = ${2:int32};',
        });
    }
    if (prefix === '' || 'arr'.startsWith(prefix)) {
        items.push({
            label: 'arr<?>',
            kind: 'snippet',
            detail: 'Heterogeneous array type',
            insertText: 'arr<${1:?}>',
        });
    }
    if ('boot'.startsWith(prefix)) {
        items.push({
            label: 'boot',
            kind: 'snippet',
            detail: 'Bare-metal entry point',
            insertText: 'boot() -> {\n    ${1}\n};',
        });
    }
    if ('manual'.startsWith(prefix)) {
        items.push({
            label: 'manual',
            kind: 'snippet',
            detail: 'Stable unsafe manual memory boundary',
            insertText: 'manual {\n    ${1}\n};',
        });
        items.push({
            label: 'manual checked',
            kind: 'snippet',
            detail: 'Bounds-checked manual memory boundary',
            insertText: 'manual checked {\n    ${1}\n};',
        });
    }
    if ('spawn'.startsWith(prefix)) {
        items.push({
            label: 'spawn',
            kind: 'snippet',
            detail: 'Spawn a zero-argument void callable',
            insertText: 'spawn ${1:task}',
        });
    }
    if ('layout'.startsWith(prefix)) {
        items.push({
            label: 'layout',
            kind: 'snippet',
            detail: 'Primitive-field layout declaration',
            insertText: 'layout ${1:Point} {\n    int32 ${2:x};\n    int32 ${3:y};\n};',
        });
    }
    if ('ptr'.startsWith(prefix)) {
        items.push({
            label: 'ptr<T>',
            kind: 'snippet',
            detail: 'Typed pointer type',
            insertText: 'ptr<${1:int32}>',
        });
    }
    if ('asm'.startsWith(prefix)) {
        items.push({
            label: 'asm',
            kind: 'snippet',
            detail: 'Inline assembly declaration',
            insertText: '${1:int32} ${2:name} = asm(${3:int32 a}) -> {\n    ${4}\n};',
        });
    }
    if (prefix === '') {
        items.push({
            label: '() -> {}',
            kind: 'snippet',
            detail: 'Lambda expression',
            insertText: '(${1:type} ${2:param}) -> {\n    ${3}\n}',
        }, {
            label: '() -> expr',
            kind: 'snippet',
            detail: 'Expression-body lambda',
            insertText: '(${1:type} ${2:param}) -> ${3:expr}',
        });
    }
    return items;
}
function getWordAtCursor(code, offset) {
    const before = code.slice(0, offset);
    const match = before.match(/[a-zA-Z_][a-zA-Z0-9_]*$/);
    return match ? match[0] : '';
}
//# sourceMappingURL=completer.js.map