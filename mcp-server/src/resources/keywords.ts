import {
  BUILTIN_TYPES,
  DECLARATION_DOCS,
  KEYWORD_DOCS,
  KEYWORDS,
  NUMERIC_LITERAL_NOTES,
  PRIMITIVE_TYPES,
} from '../knowledge/keywords';

export function getKeywordsResource(): string {
  const lines: string[] = [
    '# Calynda Keywords and Reserved Words\n',
    '## Keywords',
    KEYWORDS.join(', '),
    '',
    '## Primitive Types',
    PRIMITIVE_TYPES.join(', '),
    '',
    '## Semantically Resolved Built-in Types',
    BUILTIN_TYPES.join(', '),
    '',
    '## Keyword Notes',
  ];

  for (const [keyword, description] of Object.entries(KEYWORD_DOCS)) {
    lines.push(`- \`${keyword}\`: ${description}`);
  }

  lines.push('', '## Declaration Syntax');
  for (const decl of DECLARATION_DOCS) {
    lines.push(`- **${decl.name}**: \`${decl.syntax}\` — ${decl.description}`);
  }

  lines.push('', '## Numeric Literals');
  for (const note of NUMERIC_LITERAL_NOTES) lines.push(`- ${note}`);
  return lines.join('\n');
}
