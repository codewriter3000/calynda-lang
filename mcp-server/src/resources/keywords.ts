import {
  BUILTIN_CALL_DOCS,
  BUILTIN_TYPES,
  DECLARATION_DOCS,
  KEYWORD_DOCS,
  KEYWORDS,
  NUMERIC_LITERAL_NOTES,
  PARAMETER_FORMS,
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

  lines.push('', '## Built-in Calls');
  for (const [name, description] of Object.entries(BUILTIN_CALL_DOCS)) {
    lines.push(`- \`${name}\`: ${description}`);
  }

  lines.push('', '## Parameter Forms');
  for (const form of PARAMETER_FORMS) {
    lines.push(`- \`${form.syntax}\` — ${form.description}`);
  }

  lines.push('', '## Declaration Syntax');
  for (const decl of DECLARATION_DOCS) {
    lines.push(`- **${decl.name}**: \`${decl.syntax}\` — ${decl.description}`);
  }

  lines.push('', '## Numeric Literals');
  for (const note of NUMERIC_LITERAL_NOTES) lines.push(`- ${note}`);
  return lines.join('\n');
}
