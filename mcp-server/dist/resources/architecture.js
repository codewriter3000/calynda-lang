"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getArchitectureResource = getArchitectureResource;
const architecture_1 = require("../knowledge/architecture");
function getArchitectureResource() {
    const lines = [
        '# Calynda Compiler Architecture\n',
        '## Pipeline Overview\n',
        `**Native path:** ${architecture_1.NATIVE_PIPELINE}\n`,
        `**Bytecode path:** ${architecture_1.BYTECODE_PIPELINE}\n`,
        '## Backend Strategy\n',
        architecture_1.BACKEND_STRATEGY,
        '',
        '## Source Tree\n',
        '```',
        architecture_1.SOURCE_TREE,
        '```\n',
        '## Build Targets\n',
        `Build command: \`${architecture_1.BUILD_TARGETS.buildCommand}\``,
        `Test runner: \`${architecture_1.BUILD_TARGETS.testRunner}\`\n`,
        '### Main Binaries',
        ...architecture_1.BUILD_TARGETS.mainBinaries.map(b => `- **${b.name}** — ${b.description}`),
        '',
        '### Debug Tools',
        ...architecture_1.BUILD_TARGETS.debugTools.map(b => `- **${b.name}** — ${b.description}`),
        '',
        `### Test Suites (${architecture_1.BUILD_TARGETS.testSuites.length})`,
        architecture_1.BUILD_TARGETS.testSuites.join(', '),
        '',
        '## Pipeline Stages\n',
    ];
    for (const stage of architecture_1.PIPELINE_STAGES) {
        lines.push(`### ${stage.name}`);
        lines.push(`Directory: \`${stage.dir}\``);
        lines.push(stage.description);
        if (stage.keyTypes.length > 0)
            lines.push(`**Key types:** ${stage.keyTypes.join(', ')}`);
        if (stage.keyFunctions.length > 0)
            lines.push(`**Key functions:** ${stage.keyFunctions.join(', ')}`);
        lines.push(`**Files (${stage.files.length}):** ${stage.files.join(', ')}`);
        lines.push('');
    }
    lines.push('## Error Handling Pattern\n');
    lines.push(architecture_1.ERROR_PATTERN);
    return lines.join('\n');
}
//# sourceMappingURL=architecture.js.map