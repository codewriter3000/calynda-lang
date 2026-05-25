import { DiagnosticReport } from '../analyzer/diagnostics';
export interface ValidateInput {
    code: string;
    filename?: string;
}
export interface ValidateResult {
    valid: boolean;
    errors: string[];
    warnings: string[];
    info: string[];
    errorDetails: DiagnosticReport[];
    warningDetails: DiagnosticReport[];
    infoDetails: DiagnosticReport[];
}
export declare function validateCode(input: ValidateInput): ValidateResult;
