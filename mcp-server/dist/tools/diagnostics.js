"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.explainDiagnostic = explainDiagnostic;
const diagnostics_1 = require("../knowledge/diagnostics");
function explainDiagnostic(input) {
    const matches = (0, diagnostics_1.findDiagnosticDocs)(input.diagnostic || '');
    if (matches.length === 0) {
        return {
            explanation: [
                `No specific diagnostic catalog entry matched "${input.diagnostic}".`,
                '',
                'Known diagnostics:',
                ...diagnostics_1.DIAGNOSTIC_DOCS.map(doc => `- ${doc.title} (${doc.kind})`),
            ].join('\n'),
        };
    }
    return {
        explanation: matches.map(doc => (0, diagnostics_1.formatDiagnosticDoc)(doc)).join('\n\n---\n\n'),
    };
}
//# sourceMappingURL=diagnostics.js.map