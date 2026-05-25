"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.validateCode = validateCode;
const lexer_1 = require("../parser/lexer");
const parser_1 = require("../parser/parser");
const semantic_1 = require("../analyzer/semantic");
const diagnostics_1 = require("../analyzer/diagnostics");
function validateCode(input) {
    const lexer = new lexer_1.Lexer(input.code);
    const tokens = lexer.tokenize();
    const parseResult = (0, parser_1.parse)(tokens);
    const parseErrorDetails = parseResult.errors.map(e => (0, diagnostics_1.buildDiagnosticReport)({
        severity: 'error',
        message: e.message,
        line: e.line,
        column: e.column,
    }));
    const errors = parseErrorDetails.map(d => d.formatted);
    const warnings = [];
    const info = [];
    const errorDetails = [...parseErrorDetails];
    const warningDetails = [];
    const infoDetails = [];
    if (parseResult.ast) {
        const analysis = (0, semantic_1.analyze)(parseResult.ast);
        for (const diag of analysis.diagnostics) {
            const report = (0, diagnostics_1.buildDiagnosticReport)(diag);
            if (diag.severity === 'error') {
                errors.push(report.formatted);
                errorDetails.push(report);
            }
            else if (diag.severity === 'warning') {
                warnings.push(report.formatted);
                warningDetails.push(report);
            }
            else {
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
//# sourceMappingURL=validator.js.map