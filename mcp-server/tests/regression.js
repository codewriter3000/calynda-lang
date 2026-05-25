const assert = require('node:assert/strict');

const { buildDiagnosticReport } = require('../dist/analyzer/diagnostics.js');
const { handleToolCall } = require('../dist/index.js');

function parseToolText(result) {
  assert.ok(result);
  assert.ok(Array.isArray(result.content));
  assert.equal(result.content[0].type, 'text');
  return JSON.parse(result.content[0].text);
}

async function run() {
  const code = 'start(string[] args) -> missingSymbol;\n';

  const analyzeResult = parseToolText(
    await handleToolCall('analyze_calynda_code', { code })
  );
  assert.ok('ast' in analyzeResult, 'analyze route should return analyzer-shaped output');
  assert.ok('symbols' in analyzeResult, 'analyze route should expose symbols');
  assert.ok('diagnosticDetails' in analyzeResult, 'analyze route should expose structured diagnostics');
  assert.ok(!('valid' in analyzeResult), 'analyze route must not return validator-shaped output');

  const validateResult = parseToolText(
    await handleToolCall('validate_calynda_types', { code })
  );
  assert.equal(typeof validateResult.valid, 'boolean', 'validate route should return validator-shaped output');
  assert.ok(Array.isArray(validateResult.warningDetails), 'validate route should expose structured warning details');
  assert.ok(!('ast' in validateResult), 'validate route must not return analyzer-shaped output');

  const report = buildDiagnosticReport({
    severity: 'warning',
    message: 'Dynamic callable dispatch through runtime helper dispatch.',
    line: 1,
    column: 1,
  });
  assert.ok(report.catalogMatches.length > 0, 'known diagnostics should attach catalog matches');
  assert.equal(report.catalogMatches[0].key, 'external-call-dispatch');

  console.log('MCP regression tests passed.');
}

run().catch((err) => {
  console.error(err);
  process.exit(1);
});