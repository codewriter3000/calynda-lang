"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getKeywordsResource = getKeywordsResource;
const keywords_1 = require("../knowledge/keywords");
function getKeywordsResource() {
    const lines = [
        '# Calynda Keywords and Reserved Words\n',
        '## Keywords',
        keywords_1.KEYWORDS.join(', '),
        '',
        '## Primitive Types',
        keywords_1.PRIMITIVE_TYPES.join(', '),
        '',
        '## Semantically Resolved Built-in Types',
        keywords_1.BUILTIN_TYPES.join(', '),
        '',
        '## Keyword Notes',
    ];
    for (const [keyword, description] of Object.entries(keywords_1.KEYWORD_DOCS)) {
        lines.push(`- \`${keyword}\`: ${description}`);
    }
    lines.push('', '## Declaration Syntax');
    for (const decl of keywords_1.DECLARATION_DOCS) {
        lines.push(`- **${decl.name}**: \`${decl.syntax}\` — ${decl.description}`);
    }
    lines.push('', '## Numeric Literals');
    for (const note of keywords_1.NUMERIC_LITERAL_NOTES)
        lines.push(`- ${note}`);
    return lines.join('\n');
}
//# sourceMappingURL=keywords.js.map