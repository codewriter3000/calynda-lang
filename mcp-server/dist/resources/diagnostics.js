"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getDiagnosticsResource = getDiagnosticsResource;
const diagnostics_1 = require("../knowledge/diagnostics");
function getDiagnosticsResource() {
    const sections = diagnostics_1.DIAGNOSTIC_DOCS.map(doc => (0, diagnostics_1.formatDiagnosticDoc)(doc));
    return ['# Calynda Diagnostics Catalog', '', ...sections].join('\n\n');
}
//# sourceMappingURL=diagnostics.js.map