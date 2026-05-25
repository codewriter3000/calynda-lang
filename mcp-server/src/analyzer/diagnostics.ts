import { findDiagnosticDocs, formatDiagnosticDoc } from '../knowledge/diagnostics';

export type DiagnosticSeverity = 'error' | 'warning' | 'info';

export interface DiagnosticCatalogMatch {
  key: string;
  title: string;
  kind: string;
  explanation: string;
}

export interface Diagnostic {
  severity: DiagnosticSeverity;
  message: string;
  line: number;
  column: number;
  endLine?: number;
  endColumn?: number;
  code?: string;
}

export interface DiagnosticReport extends Diagnostic {
  formatted: string;
  catalogMatches: DiagnosticCatalogMatch[];
}

export function formatDiagnostic(d: Diagnostic): string {
  const loc = `${d.line}:${d.column}`;
  return `[${d.severity.toUpperCase()}] ${loc}: ${d.message}`;
}

export function buildDiagnosticReport(d: Diagnostic): DiagnosticReport {
  const formatted = formatDiagnostic(d);
  const catalogMatches = findDiagnosticDocs(`${d.message} ${formatted}`).map(doc => ({
    key: doc.key,
    title: doc.title,
    kind: doc.kind,
    explanation: formatDiagnosticDoc(doc),
  }));

  return {
    ...d,
    formatted,
    catalogMatches,
  };
}
