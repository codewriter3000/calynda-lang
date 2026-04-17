## Summary

Calynda currently appears to support process input through `start(string[] args)`, but it does not appear to expose a built-in stdin or interactive console input API.

That leaves hosted programs without a standard way to read user input at runtime.

## Why This Matters

Command-line arguments are useful for non-interactive programs, but they are not a replacement for ordinary console input.

Basic examples that currently need this include:

- prompting the user for a name
- reading a line from the terminal
- reading one character at a time
- building small REPL-style tools
- interactive debugging and experimentation

Without a standard input surface, even trivial hosted programs need custom runtime hooks, inline assembly, or external adapters.

## What I Verified

### 1. Hosted argument input exists

The documented hosted entry point is:

```cal
start(string[] args) -> { ... };
```

So process arguments are supported.

### 2. I could not find a built-in stdin/input API

I did not find a documented Calynda surface for things like:

- `read_line()`
- `read_char()`
- `stdin.read()`
- `input()`

and I did not find runtime hooks that obviously expose general console input as a language feature.

### 3. This is a missing standard-library/runtime capability, not just a bare-metal issue

Bare-metal boards and emulators can always fall back to device-specific UART reads.

The more important gap is that normal hosted Calynda programs do not seem to have a standard input story yet.

## Expected Behavior

At least one of these should be supported:

1. `stdlib.read_line()` returning `string`
2. `stdlib.read_char()` returning `char` or an option-style result
3. `stdin` as a standard stream object with read methods
4. A minimal input API that works for hosted native programs, with bare-metal remaining explicitly platform-specific

## Suggested Scope

The smallest useful hosted input surface would be:

1. one line-based API
2. one character-based API
3. clear EOF behavior
4. clear error behavior

That would be enough to support interactive examples without overcommitting the language runtime to a large I/O abstraction immediately.

## Why This Feels Like A Gap

The language already has a documented hosted entry model and a print path, so users will naturally expect simple console interaction to be available as well.

Right now, the surface feels asymmetric: output exists, argument passing exists, but interactive input does not appear to have a first-class API.

## Environment

- Host: Debian/Linux x86_64
- Compiler: Calynda 1.0.0-alpha.4
- Verified from current README, runtime surface, and compiler repository search