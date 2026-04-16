"use strict";
/**
 * Bytecode ISA knowledge base for the portable-v1 target.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.BYTECODE_TERMINATORS = exports.BYTECODE_INSTRUCTIONS = exports.BYTECODE_ISA = exports.BYTECODE_TARGET = void 0;
exports.BYTECODE_TARGET = 'portable-v1';
exports.BYTECODE_ISA = `# Bytecode ISA — portable-v1

## Target

The bytecode backend targets portable-v1. It is a compiled backend, not an
interpreter surface. The compiler lowers MIR into structured bytecode programs.

## Program Shape

Each BytecodeProgram contains:
  - target tag: portable-v1
  - constant pool shared across all units (literals, symbols, template text)
  - one bytecode unit per MIR unit

## Unit Kinds

  - start   — program entry point
  - binding — top-level variable/function
  - init    — variable initializer (__mir_module_init)
  - lambda  — closure body

Each unit carries: name, return type, locals (kind + type), parameter count,
temp count, and an ordered list of basic blocks.

Each block keeps: label, linear instruction sequence, one terminator.

## Operands (BytecodeValue)

  - temp     — temporary / virtual register
  - local    — stack slot
  - global   — global symbol reference
  - constant — constant-pool index

## Instructions (18)

  Computation:  BC_BINARY, BC_UNARY, BC_CAST
  Control:      BC_CLOSURE, BC_CALL
  Access:       BC_MEMBER, BC_INDEX_LOAD
  Construct:    BC_ARRAY_LITERAL, BC_TEMPLATE
  Store:        BC_STORE_LOCAL, BC_STORE_GLOBAL, BC_STORE_INDEX, BC_STORE_MEMBER
  Union:        BC_UNION_NEW, BC_UNION_GET_TAG, BC_UNION_GET_PAYLOAD
  HeteroArray:  BC_HETERO_ARRAY_NEW, BC_HETERO_ARRAY_GET_TAG

## Terminators (4)

  BC_RETURN [value]                   — return from unit
  BC_JUMP target_block                — unconditional jump
  BC_BRANCH cond true_blk false_blk   — conditional branch
  BC_THROW value                      — throw exception

## Concurrency Helper Lowering

Alpha.2 concurrency stays on the existing helper-call boundary. There are no
new bytecode opcodes for threads, futures, mutexes, or atomics.

\`spawn\`, \`Thread.join()\`, \`Thread.cancel()\`, \`Future<T>.get()\`,
\`Future<T>.cancel()\`, \`Mutex.new()\`, \`Mutex.lock()\`, \`Mutex.unlock()\`,
and \`Atomic<T>.new/load/store/exchange\` all lower as \`BC_CALL\`
instructions targeting well-known \`__calynda_rt_*\` runtime symbols.

## Backend Boundary

  Native:   AST → HIR → MIR → LIR → Codegen → Machine → target asm → executable
  Bytecode: AST → HIR → MIR → BytecodeProgram (portable-v1)
`;
exports.BYTECODE_INSTRUCTIONS = [
    'BC_BINARY', 'BC_UNARY', 'BC_CAST',
    'BC_CLOSURE', 'BC_CALL',
    'BC_MEMBER', 'BC_INDEX_LOAD',
    'BC_ARRAY_LITERAL', 'BC_TEMPLATE',
    'BC_STORE_LOCAL', 'BC_STORE_GLOBAL', 'BC_STORE_INDEX', 'BC_STORE_MEMBER',
    'BC_UNION_NEW', 'BC_UNION_GET_TAG', 'BC_UNION_GET_PAYLOAD',
    'BC_HETERO_ARRAY_NEW', 'BC_HETERO_ARRAY_GET_TAG',
];
exports.BYTECODE_TERMINATORS = [
    'BC_RETURN', 'BC_JUMP', 'BC_BRANCH', 'BC_THROW',
];
//# sourceMappingURL=bytecode.js.map