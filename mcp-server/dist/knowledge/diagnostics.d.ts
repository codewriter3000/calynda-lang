export type DiagnosticKind = 'warning' | 'performance warning' | 'performance advisory';
export interface DiagnosticDoc {
    key: string;
    title: string;
    kind: DiagnosticKind;
    aliases: string[];
    messagePatterns: string[];
    summary: string;
    when: string[];
    why: string[];
    prefer: string[];
    relatedFlags?: string[];
}
export declare const DIAGNOSTIC_DOCS: DiagnosticDoc[];
export declare function findDiagnosticDocs(query: string): DiagnosticDoc[];
export declare function formatDiagnosticDoc(doc: DiagnosticDoc): string;
