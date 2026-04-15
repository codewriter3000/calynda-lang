import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  ListResourcesRequestSchema,
  ReadResourceRequestSchema,
  ListPromptsRequestSchema,
  GetPromptRequestSchema,
} from '@modelcontextprotocol/sdk/types.js';

import { validateCode } from './tools/validator';
import { analyzeCode } from './tools/analyzer';
import { explainTopic } from './tools/explainer';
import { getCompletions } from './tools/completer';
import { searchExamples } from './tools/examples';
import { formatCode } from './tools/formatter';

import { getGrammarResource } from './resources/grammar';
import { getTypesResource } from './resources/types';
import { getKeywordsResource } from './resources/keywords';
import { getExamplesResource } from './resources/examples';
import { getArchitectureResource } from './resources/architecture';
import { getBytecodeResource } from './resources/bytecode';

import { PROMPTS, getPromptMessages } from './prompts/index';

const server = new Server(
  { name: 'calynda-mcp-server', version: '0.3.0' },
  { capabilities: { tools: {}, resources: {}, prompts: {} } }
);

server.setRequestHandler(ListToolsRequestSchema, async () => ({
  tools: [
    {
      name: 'analyze_calynda_code',
      description: 'Analyze Calynda source code for syntax errors, type issues, and provide suggestions',
      inputSchema: {
        type: 'object' as const,
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
        type: 'object' as const,
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
        type: 'object' as const,
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
        type: 'object' as const,
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
        type: 'object' as const,
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
        type: 'object' as const,
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
        type: 'object' as const,
        properties: {
          topic: { type: 'string', description: 'The architecture topic to explain (e.g., "pipeline", "HIR", "MIR", "bytecode", "backend", "build", "source tree", "codegen", "runtime")' },
        },
        required: ['topic'],
      },
    },
  ],
}));

server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  try {
    switch (name) {
      case 'analyze_calynda_code': {
        const a = args as Record<string, unknown>;
        const result = validateCode({ code: a['code'] as string, filename: a['filename'] as string | undefined });
        return { content: [{ type: 'text' as const, text: JSON.stringify(result, null, 2) }] };
      }
      case 'explain_calynda_syntax': {
        const a = args as Record<string, unknown>;
        const result = explainTopic({ topic: a['topic'] as string });
        let text = result.explanation;
        if (result.examples && result.examples.length > 0) {
          text += '\n\n**Examples:**\n' + result.examples.map(e => '```cal\n' + e + '\n```').join('\n');
        }
        return { content: [{ type: 'text' as const, text }] };
      }
      case 'complete_calynda_code': {
        const a = args as Record<string, unknown>;
        const result = getCompletions({ code: a['code'] as string, cursorOffset: a['cursorOffset'] as number });
        return { content: [{ type: 'text' as const, text: JSON.stringify(result, null, 2) }] };
      }
      case 'validate_calynda_types': {
        const a = args as Record<string, unknown>;
        const result = analyzeCode({ code: a['code'] as string });
        return { content: [{ type: 'text' as const, text: JSON.stringify(result, null, 2) }] };
      }
      case 'get_calynda_examples': {
        const a = args as Record<string, unknown>;
        const result = searchExamples({ tags: a['tags'] as string[] | undefined, query: a['feature'] as string | undefined });
        const text = result.examples.map(e => `### ${e.name}\n${e.description}\n\`\`\`cal\n${e.code}\n\`\`\``).join('\n\n');
        return { content: [{ type: 'text' as const, text: `Found ${result.total} examples:\n\n${text}` }] };
      }
      case 'format_calynda_code': {
        const a = args as Record<string, unknown>;
        const result = formatCode({ code: a['code'] as string });
        return { content: [{ type: 'text' as const, text: result.formatted }] };
      }
      case 'explain_compiler_architecture': {
        const a = args as Record<string, unknown>;
        const result = explainTopic({ topic: a['topic'] as string });
        return { content: [{ type: 'text' as const, text: result.explanation }] };
      }
      default:
        throw new Error(`Unknown tool: ${name}`);
    }
  } catch (err) {
    return {
      content: [{ type: 'text' as const, text: `Error: ${err instanceof Error ? err.message : String(err)}` }],
      isError: true,
    };
  }
});

server.setRequestHandler(ListResourcesRequestSchema, async () => ({
  resources: [
    { uri: 'calynda://grammar', name: 'Calynda Grammar (EBNF)', description: 'The full V3 EBNF grammar specification', mimeType: 'text/plain' },
    { uri: 'calynda://types', name: 'Calynda Types', description: 'Documentation for all built-in types', mimeType: 'text/markdown' },
    { uri: 'calynda://keywords', name: 'Calynda Keywords', description: 'All keywords and reserved words', mimeType: 'text/markdown' },
    { uri: 'calynda://examples', name: 'Calynda Examples', description: 'Code examples for common patterns', mimeType: 'text/markdown' },
    { uri: 'calynda://architecture', name: 'Compiler Architecture', description: 'Full compiler pipeline, source tree, build targets, and stage descriptions', mimeType: 'text/markdown' },
    { uri: 'calynda://bytecode', name: 'Bytecode ISA', description: 'Portable-v1 bytecode instruction set architecture', mimeType: 'text/markdown' },
  ],
}));

server.setRequestHandler(ReadResourceRequestSchema, async (request) => {
  const uri = request.params.uri;
  switch (uri) {
    case 'calynda://grammar':
      return { contents: [{ uri, mimeType: 'text/plain', text: getGrammarResource() }] };
    case 'calynda://types':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getTypesResource() }] };
    case 'calynda://keywords':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getKeywordsResource() }] };
    case 'calynda://examples':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getExamplesResource() }] };
    case 'calynda://architecture':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getArchitectureResource() }] };
    case 'calynda://bytecode':
      return { contents: [{ uri, mimeType: 'text/markdown', text: getBytecodeResource() }] };
    default:
      throw new Error(`Unknown resource: ${uri}`);
  }
});

server.setRequestHandler(ListPromptsRequestSchema, async () => ({
  prompts: PROMPTS.map(p => ({
    name: p.name,
    description: p.description,
    arguments: p.arguments,
  })),
}));

server.setRequestHandler(GetPromptRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;
  const messages = getPromptMessages(name, (args as Record<string, string>) || {});
  return {
    messages: messages.map(m => ({
      role: m.role as 'user' | 'assistant',
      content: { type: 'text' as const, text: m.content },
    })),
  };
});

async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error('Calynda MCP server running on stdio');
}

main().catch((err) => {
  console.error('Fatal error:', err);
  process.exit(1);
});
