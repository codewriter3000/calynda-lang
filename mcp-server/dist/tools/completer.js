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
    if (prefix === '' || 'arr'.startsWith(prefix)) {
        items.push({
            label: 'arr<?>',
            kind: 'snippet',
            detail: 'Heterogeneous array type',
            insertText: 'arr<${1:?}>',
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