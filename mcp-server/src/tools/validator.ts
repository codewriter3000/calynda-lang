import { Lexer } from '../parser/lexer';
import { parse } from '../parser/parser';
import { analyze } from '../analyzer/semantic';
import { buildDiagnosticReport, DiagnosticReport } from '../analyzer/diagnostics';

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

export function validateCode(input: ValidateInput): ValidateResult {
  const lexer = new Lexer(input.code);
  const tokens = lexer.tokenize();
  const parseResult = parse(tokens);

  const parseErrorDetails = parseResult.errors.map(e => buildDiagnosticReport({
    severity: 'error',
    message: e.message,
    line: e.line,
    column: e.column,
  }));
  const errors: string[] = parseErrorDetails.map(d => d.formatted);
  const warnings: string[] = [];
  const info: string[] = [];
  const errorDetails: DiagnosticReport[] = [...parseErrorDetails];
  const warningDetails: DiagnosticReport[] = [];
  const infoDetails: DiagnosticReport[] = [];

  if (parseResult.ast) {
    const analysis = analyze(parseResult.ast);
    for (const diag of analysis.diagnostics) {
      const report = buildDiagnosticReport(diag);
      if (diag.severity === 'error') {
        errors.push(report.formatted);
        errorDetails.push(report);
      } else if (diag.severity === 'warning') {
        warnings.push(report.formatted);
        warningDetails.push(report);
      } else {
        info.push(report.formatted);
        infoDetails.push(report);
      }
    }
  }

  return {
    valid: errors.length === 0,
    errors,
    warnings,
    info,
    errorDetails,
    warningDetails,
    infoDetails,
  };
}
