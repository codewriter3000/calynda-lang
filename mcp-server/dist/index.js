"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const index_js_1 = require("@modelcontextprotocol/sdk/server/index.js");
const stdio_js_1 = require("@modelcontextprotocol/sdk/server/stdio.js");
const types_js_1 = require("@modelcontextprotocol/sdk/types.js");
const validator_1 = require("./tools/validator");
const analyzer_1 = require("./tools/analyzer");
const explainer_1 = require("./tools/explainer");
const completer_1 = require("./tools/completer");
const examples_1 = require("./tools/examples");
const formatter_1 = require("./tools/formatter");
const grammar_1 = require("./resources/grammar");
const types_1 = require("./resources/types");
const keywords_1 = require("./resources/keywords");
const examples_2 = require("./resources/examples");
const architecture_1 = require("./resources/architecture");
const bytecode_1 = require("./resources/bytecode");
const index_1 = require("./prompts/index");
const server = new index_js_1.Server({ name: 'calynda-mcp-server', version: '1.0.0-alpha.2' }, { capabilities: { tools: {}, resources: {}, prompts: {} } });
server.setRequestHandler(types_js_1.ListToolsRequestSchema, async () => ({
    tools: [
        {
            name: 'analyze_calynda_code',
            description: 'Analyze Calynda source code for syntax errors, type issues, and provide suggestions',
            inputSchema: {
                type: 'object',
                properties: {
                    code: { type: 'string', description: 'The Calynda source code to analyze' },
                    filename: { type: 'string', description: 'Optional filename for better diagnostics' },
                },
                required: ['code'],
            },
        },
        {
            name: 'explain_calynda_syntax',
            description: 'Explain Calynda language features, syntax, or compiler pipeline stages',
            inputSchema: {
                type: 'object',
                properties: {
                    topic: { type: 'string', description: 'The feature, syntax, or pipeline stage to explain (e.g., "lambda", "union", "generics", "HIR", "MIR", "bytecode", "pipeline")' },
                },
                required: ['topic'],
            },
        },
        {
            name: 'complete_calynda_code',
            description: 'Provide context-aware code completions',
            inputSchema: {
                type: 'object',
                properties: {
                    code: { type: 'string', description: 'The Calynda source code context' },
                    cursorOffset: { type: 'number', description: 'The cursor position (character offset)' },
                },
                required: ['code', 'cursorOffset'],
            },
        },
        {
            name: 'validate_calynda_types',
            description: 'Type check Calynda code and return type information and errors',
            inputSchema: {
                type: 'object',
                properties: {
                    code: { type: 'string', description: 'The Calynda source code to type-check' },
                },
                required: ['code'],
            },
        },
        {
            name: 'get_calynda_examples',
            description: 'Get example code for specific Calynda features or patterns',
            inputSchema: {
                type: 'object',
                properties: {
                    feature: { type: 'string', description: 'Feature or pattern name to get examples for' },
                    tags: { type: 'array', items: { type: 'string' }, description: 'Filter by tags' },
                },
            },
        },
        {
            name: 'format_calynda_code',
            description: 'Format Calynda code according to language conventions',
            inputSchema: {
                type: 'object',
                properties: {
                    code: { type: 'string', description: 'The unformatted Calynda source code' },
                },
                required: ['code'],
            },
        },
        {
            name: 'explain_compiler_architecture',
            description: 'Explain the Calynda compiler architecture, pipeline stages, source tree, build targets, bytecode ISA, or backend strategy',
            inputSchema: {
                type: 'object',
                properties: {
                    topic: { type: 'string', description: 'The architecture topic to explain (e.g., "pipeline", "HIR", "MIR", "bytecode", "backend", "build", "source tree", "codegen", "runtime")' },
                },
                required: ['topic'],
            },
        },
    ],
}));
server.setRequestHandler(types_js_1.CallToolRequestSchema, async (request) => {
    const { name, arguments: args } = request.params;
    try {
        switch (name) {
            case 'analyze_calynda_code': {
                const a = args;
                const result = (0, validator_1.validateCode)({ code: a['code'], filename: a['filename'] });
                return { content: [{ type: 'text', text: JSON.stringify(result, null, 2) }] };
            }
            case 'explain_calynda_syntax': {
                const a = args;
                const result = (0, explainer_1.explainTopic)({ topic: a['topic'] });
                let text = result.explanation;
                if (result.examples && result.examples.length > 0) {
                    text += '\n\n**Examples:**\n' + result.examples.map(e => '```cal\n' + e + '\n```').join('\n');
                }
                return { content: [{ type: 'text', text }] };
            }
            case 'complete_calynda_code': {
                const a = args;
                const result = (0, completer_1.getCompletions)({ code: a['code'], cursorOffset: a['cursorOffset'] });
                return { content: [{ type: 'text', text: JSON.stringify(result, null, 2) }] };
            }
            case 'validate_calynda_types': {
                const a = args;
                const result = (0, analyzer_1.analyzeCode)({ code: a['code'] });
                return { content: [{ type: 'text', text: JSON.stringify(result, null, 2) }] };
            }
            case 'get_calynda_examples': {
                const a = args;
                const result = (0, examples_1.searchExamples)({ tags: a['tags'], query: a['feature'] });
                const text = result.examples.map(e => `### ${e.name}\n${e.description}\n\`\`\`cal\n${e.code}\n\`\`\``).join('\n\n');
                return { content: [{ type: 'text', text: `Found ${result.total} examples:\n\n${text}` }] };
            }
            case 'format_calynda_code': {
                const a = args;
                const result = (0, formatter_1.formatCode)({ code: a['code'] });
                return { content: [{ type: 'text', text: result.formatted }] };
            }
            case 'explain_compiler_architecture': {
                const a = args;
                const result = (0, explainer_1.explainTopic)({ topic: a['topic'] });
                return { content: [{ type: 'text', text: result.explanation }] };
            }
            default:
                throw new Error(`Unknown tool: ${name}`);
        }
    }
    catch (err) {
        return {
            content: [{ type: 'text', text: `Error: ${err instanceof Error ? err.message : String(err)}` }],
            isError: true,
        };
    }
});
server.setRequestHandler(types_js_1.ListResourcesRequestSchema, async () => ({
    resources: [
        { uri: 'calynda://grammar', name: 'Calynda Grammar (EBNF)', description: 'The full Calynda grammar snapshot (1.0.0-alpha.2)', mimeType: 'text/plain' },
        { uri: 'calynda://types', name: 'Calynda Types', description: 'Documentation for all built-in types', mimeType: 'text/markdown' },
        { uri: 'calynda://keywords', name: 'Calynda Keywords', description: 'All keywords and reserved words', mimeType: 'text/markdown' },
        { uri: 'calynda://examples', name: 'Calynda Examples', description: 'Code examples for common patterns', mimeType: 'text/markdown' },
        { uri: 'calynda://architecture', name: 'Compiler Architecture', description: 'Full compiler pipeline, source tree, build targets, and stage descriptions', mimeType: 'text/markdown' },
        { uri: 'calynda://bytecode', name: 'Bytecode ISA', description: 'Portable-v1 bytecode instruction set architecture', mimeType: 'text/markdown' },
    ],
}));
server.setRequestHandler(types_js_1.ReadResourceRequestSchema, async (request) => {
    const uri = request.params.uri;
    switch (uri) {
        case 'calynda://grammar':
            return { contents: [{ uri, mimeType: 'text/plain', text: (0, grammar_1.getGrammarResource)() }] };
        case 'calynda://types':
            return { contents: [{ uri, mimeType: 'text/markdown', text: (0, types_1.getTypesResource)() }] };
        case 'calynda://keywords':
            return { contents: [{ uri, mimeType: 'text/markdown', text: (0, keywords_1.getKeywordsResource)() }] };
        case 'calynda://examples':
            return { contents: [{ uri, mimeType: 'text/markdown', text: (0, examples_2.getExamplesResource)() }] };
        case 'calynda://architecture':
            return { contents: [{ uri, mimeType: 'text/markdown', text: (0, architecture_1.getArchitectureResource)() }] };
        case 'calynda://bytecode':
            return { contents: [{ uri, mimeType: 'text/markdown', text: (0, bytecode_1.getBytecodeResource)() }] };
        default:
            throw new Error(`Unknown resource: ${uri}`);
    }
});
server.setRequestHandler(types_js_1.ListPromptsRequestSchema, async () => ({
    prompts: index_1.PROMPTS.map(p => ({
        name: p.name,
        description: p.description,
        arguments: p.arguments,
    })),
}));
server.setRequestHandler(types_js_1.GetPromptRequestSchema, async (request) => {
    const { name, arguments: args } = request.params;
    const messages = (0, index_1.getPromptMessages)(name, args || {});
    return {
        messages: messages.map(m => ({
            role: m.role,
            content: { type: 'text', text: m.content },
        })),
    };
});
async function main() {
    const transport = new stdio_js_1.StdioServerTransport();
    await server.connect(transport);
    console.error('Calynda MCP server running on stdio');
}
main().catch((err) => {
    console.error('Fatal error:', err);
    process.exit(1);
});
//# sourceMappingURL=index.js.map