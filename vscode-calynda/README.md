# Calynda Syntax

Minimal VS Code extension for the frozen Calynda 0.4 syntax surface.

## What it provides

- `.cal` file association
- Line and block comment support
- Bracket and quote auto-closing
- Syntax highlighting for:
  - keywords and modifiers
  - primitive types
  - tagged unions, `arr<?>`, `ptr<T>`, `layout`, and manual-memory keywords
  - booleans and `null`
  - numeric literals
  - strings, chars, and template literals
  - operators, punctuation, declarations, and parameters

## Install locally in VS Code

1. Open this folder in VS Code: `vscode-calynda`
2. Press `F5` to launch an Extension Development Host
3. Open any `.cal` file in that host window

## Package it

If you want a `.vsix` package:

1. Install `vsce`: `npm install -g @vscode/vsce`
2. From this folder, run: `vsce package`
3. Install the generated `.vsix` in VS Code

## Notes

The grammar is based on the canonical language definition in `../compiler/calynda.ebnf` plus the tokenizer behavior in `../compiler/src/tokenizer/tokenizer.h`.