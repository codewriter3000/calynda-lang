# Contributing to Calynda

Thank you for your interest in contributing to Calynda! We welcome contributions from developers of all experience levels. Whether you're fixing a typo, reporting a bug, or proposing a new language feature, your contributions help make Calynda better for everyone.

If you have any questions or concerns about contributing, please reach out to [amicharski@outlook.com](mailto:amicharski@outlook.com) and I'll be glad to help.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and collaborative environment. We're building Calynda together as a community.

This project is licensed under the [MIT License](LICENSE). All contributions will be made under the same license.

## Prerequisites

Before contributing, make sure you have the following tools installed:

- **gcc** 12.2 or later
- **make**
- **Node.js** 25.6.1 or later (only needed for MCP server work)
- **npm** 11.10.0 or later (only needed for MCP server work)

For installation and initial setup instructions, see the [README.md](README.md).

## Repository Structure

This is a monorepo containing three main components:

| Component | Path | Description |
|-----------|------|-------------|
| **Compiler** | [`compiler/`](compiler/) | C compiler, runtime, and CLI |
| **MCP Server** | [`mcp-server/`](mcp-server/) | Model Context Protocol server for AI assistant integration |
| **VS Code Extension** | [`vscode-calynda/`](vscode-calynda/) | Syntax highlighting extension for VS Code |

Most language and compiler contributions will be in the `compiler/` directory. The MCP server and VS Code extension support tooling and editor integration.

## Development Workflow

Here's the basic workflow for contributing:

1. **Fork and clone** the repository
2. **Create a feature branch** for your changes
3. **Make your changes** with corresponding tests
4. **Run the test suite** to ensure everything passes
5. **Update documentation** as needed
6. **Submit a pull request** with a clear description

### Running Tests

To run the full compiler test suite:

```sh
./compiler/run_tests.sh
```

Or using make:

```sh
make -C compiler test
```

Individual test binaries are available in `compiler/build/` and can be run directly:

```sh
./compiler/build/test_parser
./compiler/build/test_type_checker
./compiler/build/test_mir_dump
# ... and many more
```

## Testing Requirements

**Testing is mandatory for all code contributions.** Here are the requirements:

- **New features** must include test coverage in [`compiler/tests/`](compiler/tests/)
- **Bug fixes** must include regression tests that would have caught the bug
- Tests must cover both **success cases** and **error/diagnostic cases**
- All existing tests must continue to pass

Each compiler module has a corresponding test file. For example:
- `src/parser.c` has tests in `tests/test_parser.c`
- `src/type_checker.c` has tests in `tests/test_type_checker.c`

Follow the existing test patterns when adding new test cases.

## Coding Standards

### Compiler (C)

- **Standard**: C11 (`-std=c11`)
- **Warnings**: Code must compile cleanly with `-Wall -Wextra -pedantic`
- **Style**: Match the existing code style in the project
- Write clear, readable code with appropriate comments for complex logic

### MCP Server (TypeScript)

- Follow standard TypeScript conventions
- Match the existing code style in the `mcp-server/` directory

### VS Code Extension

- Follow [VS Code extension guidelines](https://code.visualstudio.com/api/references/extension-guidelines)
- Match the existing patterns in `vscode-calynda/`

### Commit Messages

Write clear, descriptive commit messages that explain what changed and why. There's no strict format requirement—just make sure your messages are helpful to reviewers and future maintainers.

## ⚠️ Grammar Stability Policy

**This is critical for contributors working on language features:**

### V1 Grammar is Canonical and Stable

The file [`compiler/calynda.ebnf`](compiler/calynda.ebnf) defines the **V1 grammar** and is the canonical, shipped language specification. **DO NOT MODIFY THIS FILE** unless you are implementing a fully validated V2 feature.

### V2 Experimental Grammar

All V2 language experiments must go in [`compiler/calynda_v2.ebnf`](compiler/calynda_v2.ebnf). This is the sandbox for proposed language changes.

### When V2 Changes Can Be Promoted to V1

A V2 feature can only be promoted to V1 when it meets ALL of these criteria:

1. ✅ **Parser-complete**: The parser fully supports the feature
2. ✅ **Semantically specified**: Type checking and semantic analysis are complete
3. ✅ **Tested**: Comprehensive test coverage exists
4. ✅ **Mirrored**: Changes are reflected across compiler pipeline, tooling, and documentation

**If in doubt, ask first.** Open an issue to discuss grammar changes before implementing them.

For more context, see [version_wishlist.md](version_wishlist.md).

## Language Feature Contributions

### Process for Proposing New Features

**Always open an issue for discussion first** before implementing language features. This helps ensure:

- The feature aligns with Calynda's design philosophy
- There's community consensus on the approach
- We avoid duplicate or conflicting efforts

### Design Philosophy

When proposing features, keep Calynda's philosophy in mind:

1. **Java-first, C-second**: Prefer Java-like syntax first, then C-like syntax when Java conventions don't apply
2. **Simplicity**: Keep the language approachable for developers coming from Java 8 or C
3. **External adapter principle**: Features that can live in external C or assembly adapters may be unnecessary in the core language

See [version_wishlist.md](version_wishlist.md) and [backend_strategy.md](backend_strategy.md) for more details on language design decisions.

### Breaking Changes

Breaking changes to the language or CLI require **explicit discussion and approval**. Open an issue to discuss the change, its impact, and migration strategy before implementing it.

## Documentation Requirements

**All user-visible changes must be documented.** This includes:

- ✅ New features
- ✅ Bug fixes that affect behavior
- ✅ Breaking changes
- ✅ Command-line interface changes

### Where to Document

1. **[RELEASE_NOTES.md](RELEASE_NOTES.md)**: Add entries for all user-visible changes
2. **Relevant .md files**: Update architecture docs if you change compiler pipeline stages
3. **README files**: Update CLI usage documentation if you change tool behavior
4. **Code comments**: Document complex logic and design decisions in the code itself

## Pull Request Process

1. **Check existing issues**: See if there's already discussion on your topic
2. **Open an issue**: For features or significant changes, discuss the approach first
3. **Create a feature branch**: Work in a branch off `main`
4. **Implement changes with tests**: Write code and tests together
5. **Run the test suite**: Ensure all tests pass with `./compiler/run_tests.sh`
6. **Update RELEASE_NOTES.md**: Document user-visible changes
7. **Submit pull request**: Write a clear description of what changed and why
8. **Code review**: Maintainers will review on a best-effort basis (no strict timeline)
9. **Address feedback**: Make requested changes if needed
10. **Merge**: Once approved, your PR will be merged

### What to Include in Your PR Description

- What problem does this solve?
- What approach did you take?
- Are there any breaking changes?
- What testing did you do?
- Screenshots or example output (if applicable)

## Debugging and Inspection Tools

The compiler includes several tools for inspecting compiler internals. These are built automatically when you run `make -C compiler test`:

| Tool | Description |
|------|-------------|
| `dump_ast` | Display the parsed Abstract Syntax Tree |
| `dump_semantics` | Show semantic analysis results (scopes, types, symbol resolution) |
| `emit_asm` | Generate x86_64 assembly code |
| `emit_bytecode` | Generate portable bytecode |
| `build_native` | Build a native executable |

Build a specific tool:

```sh
make -C compiler dump_ast
```

These tools are useful when debugging compiler changes or understanding how the pipeline transforms code.

## Questions and Support

- **Bug reports**: Open an issue on GitHub
- **Feature discussions**: Open an issue to discuss before implementing
- **General questions**: Email [amicharski@outlook.com](mailto:amicharski@outlook.com)

We're here to help! Don't hesitate to ask if you're unsure about anything.

---

Thank you for contributing to Calynda! 🚀
