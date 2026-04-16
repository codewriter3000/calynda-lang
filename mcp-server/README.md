# Calynda MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/) (MCP) server that enables AI assistants to deeply understand and work with the [Calynda](../README.md) programming language.

This server now tracks the 1.0.0-alpha.2 language/help surface, including `spawn`, `Thread`, `Future<T>`, `Mutex`, `Atomic<T>`, `thread_local`, strict race-check help text, the freestanding `boot()` contract, and the native plus bytecode backend split.

## Overview

This MCP server provides AI assistants with comprehensive knowledge of Calynda, including:

- **Code analysis** — syntax checking, semantic validation, and diagnostics
- **Type validation** — type checking and type information
- **Code completion** — context-aware suggestions
- **Syntax explanation** — detailed explanations of language features
- **Code examples** — searchable library of common patterns
- **Code formatting** — consistent code style

## Installation

### Prerequisites

- Node.js 18 or later
- npm 9 or later

### Build from Source

```sh
cd mcp-server
npm install
npm run build
```

The compiled server will be at `dist/index.js`.

## Configuration

### Claude Desktop

Add the following to your Claude Desktop configuration file:

**macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
**Windows**: `%APPDATA%\Claude\claude_desktop_config.json`

```json
{
  "mcpServers": {
    "calynda": {
      "command": "node",
      "args": ["/path/to/calynda-lang/mcp-server/dist/index.js"]
    }
  }
}
```

### Continue (VS Code Extension)

Add to your `.continue/config.json`:

```json
{
  "mcpServers": [
    {
      "name": "calynda",
      "command": "node",
      "args": ["/path/to/calynda-lang/mcp-server/dist/index.js"]
    }
  ]
}
```

### Any MCP-Compatible Client

The server uses **stdio transport** by default, which is compatible with all standard MCP clients.

```sh
node /path/to/calynda-lang/mcp-server/dist/index.js
```

## Available Tools

### `analyze_calynda_code`

Analyze Calynda source code for syntax errors, type issues, and provide suggestions.

**Input:**
- `code` (string, required) — The Calynda source code to analyze
- `filename` (string, optional) — Filename for better diagnostic messages

**Output:** Diagnostics list with errors, warnings, and suggestions including line/column information.

**Example:**
```json
{
  "tool": "analyze_calynda_code",
  "arguments": {
    "code": "start(string[] args) -> {\n  return 0;\n};"
  }
}
```

---

### `explain_calynda_syntax`

Explain Calynda language features and syntax with examples.

**Input:**
- `topic` (string, required) — Feature name or code snippet (e.g., `"lambda"`, `"int32"`, `"template literal"`)

**Output:** Detailed explanation with code examples.

**Example:**
```json
{
  "tool": "explain_calynda_syntax",
  "arguments": {
    "topic": "template literal"
  }
}
```

---

### `complete_calynda_code`

Provide context-aware code completions at a cursor position.

**Input:**
- `code` (string, required) — The Calynda source code context
- `cursorOffset` (number, required) — Cursor position as character offset

**Output:** List of completion suggestions with labels and documentation.

**Example:**
```json
{
  "tool": "complete_calynda_code",
  "arguments": {
    "code": "int",
    "cursorOffset": 3
  }
}
```

---

### `validate_calynda_types`

Type check Calynda code and return type information and errors.

**Input:**
- `code` (string, required) — The Calynda source code to type-check

**Output:** Type information, symbol table, and type errors.

**Example:**
```json
{
  "tool": "validate_calynda_types",
  "arguments": {
    "code": "int32 x = 42;\nstring s = x;"
  }
}
```

---

### `get_calynda_examples`

Get example code for specific Calynda features or patterns.

**Input:**
- `feature` (string, optional) — Feature or pattern name (e.g., `"lambda"`, `"array"`)
- `tags` (string[], optional) — Filter by tags

**Output:** Matching examples with descriptions and code.

**Example:**
```json
{
  "tool": "get_calynda_examples",
  "arguments": {
    "feature": "template literal"
  }
}
```

---

### `format_calynda_code`

Format Calynda code according to language conventions.

**Input:**
- `code` (string, required) — The unformatted Calynda source code

**Output:** Formatted code string.

**Example:**
```json
{
  "tool": "format_calynda_code",
  "arguments": {
    "code": "int32 add=(int32 a,int32 b)->a+b;"
  }
}
```

## Available Resources

Resources provide reference documentation accessible to the AI assistant.

| URI | Description |
|-----|-------------|
| `calynda://grammar` | Complete EBNF grammar specification |
| `calynda://types` | Type system documentation for all built-in types |
| `calynda://keywords` | All reserved words and keywords |
| `calynda://examples` | Code examples for common patterns |

## Available Prompts

Prompt templates guide the AI assistant when working on specific tasks.

### `write-calynda-function`

Template for writing Calynda functions.

**Arguments:**
- `task` (required) — Description of what the function should do
- `returnType` (optional) — The return type of the function

### `debug-calynda-code`

Template for debugging Calynda code.

**Arguments:**
- `code` (required) — The Calynda code to debug
- `problem` (optional) — Description of the problem or error

### `convert-to-calynda`

Template for converting code from other languages to Calynda.

**Arguments:**
- `code` (required) — The source code to convert
- `sourceLanguage` (optional) — The source language (e.g., `Python`, `JavaScript`, `C`)

## Usage Examples

### Analyzing a Calynda File

Ask your AI assistant:
> "Analyze this Calynda code and tell me if there are any issues: ..."

The assistant will use `analyze_calynda_code` to parse and validate your code.

### Learning the Language

Ask:
> "Explain how lambdas work in Calynda"

The assistant will use `explain_calynda_syntax` to provide a detailed explanation with examples.

### Getting Started

Ask:
> "Show me a hello world example in Calynda"

The assistant will use `get_calynda_examples` to retrieve the hello world pattern.

### Converting Code

Use the `convert-to-calynda` prompt:
> "Convert this Python function to Calynda: def add(a, b): return a + b"

## Development

### Project Structure

```
mcp-server/
├── src/
│   ├── index.ts             # MCP server entry point
│   ├── tools/               # Tool implementations
│   │   ├── analyzer.ts      # Code analysis
│   │   ├── explainer.ts     # Syntax explanation
│   │   ├── completer.ts     # Code completion
│   │   ├── validator.ts     # Type validation
│   │   ├── examples.ts      # Example provider
│   │   └── formatter.ts     # Code formatter
│   ├── resources/           # Resource implementations
│   │   ├── grammar.ts       # Grammar resource
│   │   ├── types.ts         # Types resource
│   │   ├── keywords.ts      # Keywords resource
│   │   └── examples.ts      # Examples resource
│   ├── prompts/             # Prompt templates
│   │   └── index.ts
│   ├── parser/              # Calynda language parser
│   │   ├── lexer.ts         # Tokenizer
│   │   ├── parser.ts        # Recursive-descent parser
│   │   └── ast.ts           # AST node definitions
│   ├── analyzer/            # Code analysis engine
│   │   ├── semantic.ts      # Semantic analysis
│   │   ├── types.ts         # Type checker
│   │   └── diagnostics.ts   # Diagnostics
│   └── knowledge/           # Language knowledge base
│       ├── grammar.ts       # Embedded EBNF grammar
│       ├── keywords.ts      # Keywords list
│       ├── types.ts         # Type definitions
│       └── examples.ts      # Code examples
├── dist/                    # Compiled output
├── package.json
└── tsconfig.json
```

### Scripts

```sh
npm run build    # Compile TypeScript to JavaScript
npm run watch    # Watch mode for development
npm run dev      # Run server in development mode
```

### Extending

To add new tools, resources, or prompts:

1. Add the implementation in the appropriate `src/` subdirectory
2. Register it in `src/index.ts`
3. Run `npm run build` to compile

## Language Quick Reference

| Feature | Syntax |
|---------|--------|
| Entry point | `start(string[] args) -> { return 0; };` |
| Lambda (expression) | `int32 add = (int32 a, int32 b) -> a + b;` |
| Lambda (block) | `void log = (string s) -> { exit; };` |
| Variable | `int32 x = 42;` or `var x = 42;` |
| Array | `int32[] nums = [1, 2, 3];` |
| Template literal | `` string s = `Hello ${name}`; `` |
| Cast | `float64 f = float64(myInt);` |
| Ternary | `int32 abs = x < 0 ? -x : x;` |
| Throw | `throw "error message";` |
| Modifiers | `public final int32 MAX = 100;` |

## License

MIT — see [LICENSE](../LICENSE)
