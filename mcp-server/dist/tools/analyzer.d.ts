import { DiagnosticReport } from '../analyzer/diagnostics';
export interface AnalyzeInput {
    code: string;
}
export interface AnalyzeResult {
    ast?: object;
    symbols?: Record<string, string>;
    diagnostics: string[];
    diagnosticDetails: DiagnosticReport[];
    parseErrors: string[];
    parseErrorDetails: DiagnosticReport[];
}
export declare function analyzeCode(input: AnalyzeInput): AnalyzeResult;
