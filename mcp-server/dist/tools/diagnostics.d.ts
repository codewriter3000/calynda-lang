export interface ExplainDiagnosticInput {
    diagnostic: string;
}
export interface ExplainDiagnosticResult {
    explanation: string;
}
export declare function explainDiagnostic(input: ExplainDiagnosticInput): ExplainDiagnosticResult;
