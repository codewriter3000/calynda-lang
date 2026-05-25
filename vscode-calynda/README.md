# Calynda Syntax

Minimal VS Code extension for the current Calynda alpha.8 syntax surface.

## What it provides

- `.cal` file association
- Line and block comment support
- Bracket and quote auto-closing
- Syntax highlighting for:
  - keywords and modifiers, including `spawn`
  - primitive and built-in types, including `num`, `Thread`, `Mutex`, `Future<T>`, and `Atomic<T>`
  - tagged unions, `arr<?>`, `ptr<T>`, `mmio<T>`, `layout`, and manual-memory syntax
  - untyped `var` and `|var` parameters
  - built-in call identifiers such as `typeof`, `isarray`, `issametype`, `fence`, `cacheclean`, `cachefinal`, `car`, and `cdr`
  - booleans and `null`
  - numeric literals, including underscore separators
  - strings, chars, and template literals
  - operators, including the swap operator `><`
  - declarations and typed parameters across primitive, generic, and user-defined types

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
## Changes in 1.0.0-alpha.8

- The grammar now catches up to the current compiler surface instead of the old 0.4-era subset. Highlighting covers `spawn`, the `><` swap operator, current built-in threading types, generic declaration sites, untyped `var` / `|var` parameters, and numeric literals with underscore separators.
- Runtime-backed builtins such as `fence`, `cacheclean`, `cachefinal`, `typeof`, `isarray`, and `issametype` are highlighted as built-in call identifiers rather than language keywords.
- The syntax package continues to track the canonical compiler grammar/tokenizer behaviour rather than a frozen release subset.

## Changes in 1.0.0-alpha.7

- Highlighting now covers `mmio`, `num`, `thread_local`, `type`, and the low-level barrier/cache builtin names used by the current alpha.7 surface.
- The syntax package continues to track the canonical compiler grammar/tokenizer behaviour rather than a frozen 0.4 subset.

## Changes in 1.0.0-alpha.6

- Highlighting for the new `var` parameter modifier, the `|var` early-return parameter form, and the built-in generic types `num` and `arr<?>` (the latter was already partially supported and is now treated consistently).
- The grammar continues to track `compiler/calynda.ebnf`; the recent EBNF addition is a new `Parameter` alternative for untyped parameters.
