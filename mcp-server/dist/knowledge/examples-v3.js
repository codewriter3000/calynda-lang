"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.EXAMPLES_V3 = void 0;
exports.EXAMPLES_V3 = [
    {
        name: 'inline-asm',
        description: 'Inline assembly declaration with platform-specific code',
        tags: ['asm', 'v3', 'backend'],
        code: `int32 my_add = asm(int32 a, int32 b) -> {
    mov eax, edi
    add eax, esi
    ret
};`,
    },
    {
        name: 'boot-entry',
        description: 'Freestanding bare-metal entry point that bypasses the Calynda runtime without promising Linux-only exit behavior',
        tags: ['boot', 'v3', 'embedded'],
        code: `boot -> 0;`,
    },
    {
        name: 'future-value',
        description: 'Non-void spawn returning a Future<T>',
        tags: ['future', 'spawn', 'alpha-2'],
        code: `start(string[] args) -> {
    Future<int32> job = spawn () -> 42;
    int32 value = job.get();
    return value;
};`,
    },
    {
        name: 'manual-memory',
        description: 'Manual memory management with malloc, free, and pointer operations',
        tags: ['manual', 'memory', 'v3'],
        code: `start(string[] args) -> {
    manual checked {
      ptr<int32> scratch = malloc(8);
      store(scratch, 42);
      int32 value = deref(scratch);
      cleanup(scratch, free);
    };
    return 0;
};`,
    },
    {
        name: 'typed-pointer',
        description: 'Typed pointer with automatic element sizing via ptr<T>',
        tags: ['manual', 'memory', 'ptr', 'v3'],
        code: `start(string[] args) -> {
    manual {
        ptr<int32> p = malloc(40);
        store(p, 7);
        int32 v = deref(p);
        ptr<int32> q = offset(p, 2);
        free(p);
    };
    return 0;
};`,
    },
    {
        name: 'arm64-target',
        description: 'Compiling for AArch64 Linux target',
        tags: ['arm64', 'target', 'v3', 'backend'],
        code: `// Compile with: calynda build --target aarch64 main.cal
start -> 0;`,
    },
    {
        name: 'riscv64-target',
        description: 'Compiling for RISC-V 64 Linux target',
        tags: ['riscv64', 'target', 'v3', 'backend'],
        code: `// Compile with: calynda build --target riscv64 main.cal
start -> 0;`,
    },
    {
        name: 'car-archive',
        description: 'Working with CAR source archives',
        tags: ['car', 'archive', 'v3', 'cli'],
        code: `// Create archive: calynda pack src/ -o archive.car
// Emit asm from archive: calynda asm project.car
// Build from archive: calynda build project.car`,
    },
    {
        name: 'manual-lambda-shorthand',
        description: 'Recursive explicitly typed top-level manual lambda binding',
        tags: ['manual', 'lambda', 'alpha-3', 'alpha-4', 'syntax', 'recursion', 'recursive', 'top-level-binding'],
        code: `int32 factorial = manual(int32 value) -> {
    return value <= 1 ? 1 : value * factorial(value - 1);
};

start(string[] args) -> {
    return factorial(5);
};`,
    },
    {
        name: 'layout-pointer',
        description: 'Primitive-field layout used with ptr<T> memory operations',
        tags: ['layout', 'ptr', 'manual', 'v4'],
        code: `layout Point { int32 x; int32 y; };

start(string[] args) -> {
    manual {
        ptr<Point> points = stackalloc(16);
        ptr<Point> next = offset(points, 1);
        _ = next;
    };
    return 0;
};`,
    },
    {
        name: 'swap-statement',
        description: 'Swap two assignable l-values of the same type with the >< operator',
        tags: ['swap', 'statement', 'v5'],
        code: `start(string[] args) -> {
    int32[] values = [1, 2, 3];
    values[0] >< values[2];
    return values[0];
};`,
    },
    {
        name: 'bare-start',
        description: 'Entry point with no parameters (bare start form)',
        tags: ['start', 'basic', 'v5'],
        code: `start -> {
    return 0;
};`,
    },
    {
        name: 'default-parameter',
        description: 'Lambda with a parameter that has a default value',
        tags: ['lambda', 'default', 'parameter', 'v5'],
        code: `int32 add = (int32 left, int32 right = left + 1) -> left + right;

start(string[] args) -> {
    return add(3);
};`,
    },
];
//# sourceMappingURL=examples-v3.js.map