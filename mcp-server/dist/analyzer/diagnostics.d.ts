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
export declare function formatDiagnostic(d: Diagnostic): string;
export declare function buildDiagnosticReport(d: Diagnostic): DiagnosticReport;
