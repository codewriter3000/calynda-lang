import { DIAGNOSTIC_DOCS, findDiagnosticDocs, formatDiagnosticDoc } from '../knowledge/diagnostics';

export interface ExplainDiagnosticInput {
  diagnostic: string;
}

export interface ExplainDiagnosticResult {
  explanation: string;
}

export function explainDiagnostic(input: ExplainDiagnosticInput): ExplainDiagnosticResult {
  const matches = findDiagnosticDocs(input.diagnostic || '');

  if (matches.length === 0) {
    return {
      explanation: [
        `No specific diagnostic catalog entry matched "${input.diagnostic}".`,
        '',
        'Known diagnostics:',
        ...DIAGNOSTIC_DOCS.map(doc => `- ${doc.title} (${doc.kind})`),
      ].join('\n'),
    };
  }

  return {
    explanation: matches.map(doc => formatDiagnosticDoc(doc)).join('\n\n---\n\n'),
  };
}