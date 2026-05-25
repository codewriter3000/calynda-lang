import { DIAGNOSTIC_DOCS, formatDiagnosticDoc } from '../knowledge/diagnostics';

export function getDiagnosticsResource(): string {
  const sections = DIAGNOSTIC_DOCS.map(doc => formatDiagnosticDoc(doc));
  return ['# Calynda Diagnostics Catalog', '', ...sections].join('\n\n');
}