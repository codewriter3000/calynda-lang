import { Lexer } from '../parser/lexer';
import { parse } from '../parser/parser';
import { analyze } from '../analyzer/semantic';
import { typeToString } from '../analyzer/types';
import { buildDiagnosticReport, DiagnosticReport } from '../analyzer/diagnostics';

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

export function analyzeCode(input: AnalyzeInput): AnalyzeResult {
  const lexer = new Lexer(input.code);
  const tokens = lexer.tokenize();
  const parseResult = parse(tokens);
  const parseErrorDetails = parseResult.errors.map(e => buildDiagnosticReport({
    severity: 'error',
    message: e.message,
    line: e.line,
    column: e.column,
  }));
  const parseErrors = parseErrorDetails.map(d => d.formatted);

  if (!parseResult.ast) {
    return {
      diagnostics: [],
      diagnosticDetails: [],
      parseErrors,
      parseErrorDetails,
    };
  }

  const analysis = analyze(parseResult.ast);
  const symbols: Record<string, string> = {};
  for (const [name, type] of analysis.symbols) {
    symbols[name] = typeToString(type);
  }

  const diagnosticDetails = analysis.diagnostics.map(buildDiagnosticReport);

  return {
    ast: parseResult.ast as unknown as object,
    symbols,
    diagnostics: diagnosticDetails.map(d => d.formatted),
    diagnosticDetails,
    parseErrors,
    parseErrorDetails,
  };
}
