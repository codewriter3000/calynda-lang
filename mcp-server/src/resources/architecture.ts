import { PIPELINE_STAGES, NATIVE_PIPELINE, BYTECODE_PIPELINE, BACKEND_STRATEGY, SOURCE_TREE, BUILD_TARGETS, ERROR_PATTERN } from '../knowledge/architecture';

export function getArchitectureResource(): string {
  const lines: string[] = [
    '# Calynda Compiler Architecture\n',
    '## Pipeline Overview\n',
    `**Native path:** ${NATIVE_PIPELINE}\n`,
    `**Bytecode path:** ${BYTECODE_PIPELINE}\n`,
    '## Backend Strategy\n',
    BACKEND_STRATEGY,
    '',
    '## Source Tree\n',
    '```',
    SOURCE_TREE,
    '```\n',
    '## Build Targets\n',
    `Build command: \`${BUILD_TARGETS.buildCommand}\``,
    `Test runner: \`${BUILD_TARGETS.testRunner}\`\n`,
    '### Main Binaries',
    ...BUILD_TARGETS.mainBinaries.map(b => `- **${b.name}** — ${b.description}`),
    '',
    '### Debug Tools',
    ...BUILD_TARGETS.debugTools.map(b => `- **${b.name}** — ${b.description}`),
    '',
    `### Test Suites (${BUILD_TARGETS.testSuites.length})`,
    BUILD_TARGETS.testSuites.join(', '),
    '',
    '## Pipeline Stages\n',
  ];

  for (const stage of PIPELINE_STAGES) {
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
  lines.push(ERROR_PATTERN);

  return lines.join('\n');
}
