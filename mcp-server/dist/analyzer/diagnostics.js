"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.formatDiagnostic = formatDiagnostic;
exports.buildDiagnosticReport = buildDiagnosticReport;
const diagnostics_1 = require("../knowledge/diagnostics");
function formatDiagnostic(d) {
    const loc = `${d.line}:${d.column}`;
    return `[${d.severity.toUpperCase()}] ${loc}: ${d.message}`;
}
function buildDiagnosticReport(d) {
    const formatted = formatDiagnostic(d);
    const catalogMatches = (0, diagnostics_1.findDiagnosticDocs)(`${d.message} ${formatted}`).map(doc => ({
        key: doc.key,
        title: doc.title,
        kind: doc.kind,
        explanation: (0, diagnostics_1.formatDiagnosticDoc)(doc),
    }));
    return {
        ...d,
        formatted,
        catalogMatches,
    };
}
//# sourceMappingURL=diagnostics.js.map