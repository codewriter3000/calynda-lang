"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.analyzeCode = analyzeCode;
const lexer_1 = require("../parser/lexer");
const parser_1 = require("../parser/parser");
const semantic_1 = require("../analyzer/semantic");
const types_1 = require("../analyzer/types");
const diagnostics_1 = require("../analyzer/diagnostics");
function analyzeCode(input) {
    const lexer = new lexer_1.Lexer(input.code);
    const tokens = lexer.tokenize();
    const parseResult = (0, parser_1.parse)(tokens);
    const parseErrorDetails = parseResult.errors.map(e => (0, diagnostics_1.buildDiagnosticReport)({
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
    const analysis = (0, semantic_1.analyze)(parseResult.ast);
    const symbols = {};
    for (const [name, type] of analysis.symbols) {
        symbols[name] = (0, types_1.typeToString)(type);
    }
    const diagnosticDetails = analysis.diagnostics.map(diagnostics_1.buildDiagnosticReport);
    return {
        ast: parseResult.ast,
        symbols,
        diagnostics: diagnosticDetails.map(d => d.formatted),
        diagnosticDetails,
        parseErrors,
        parseErrorDetails,
    };
}
//# sourceMappingURL=analyzer.js.map