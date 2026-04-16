export interface Example {
  name: string;
  description: string;
  code: string;
  tags: string[];
}

import { EXAMPLES_V3 } from './examples-v3';

const BASE_EXAMPLES: Example[] = [
  {
    name: 'hello-world',
    description: 'Hello world program using template literals',
    tags: ['basic', 'template-literal', 'start', 'stdlib'],
    code: `import io.stdlib;

start(string[] args) -> {
    var render = (string name) -> \`hello \${name}\`;
    stdlib.print(render(args[0]));
    return 0;
};`,
  },
  {
    name: 'simple-lambda',
    description: 'Simple lambda with expression body',
    tags: ['lambda', 'basic'],
    code: `int32 add = (int32 a, int32 b) -> a + b;`,
  },
  {
    name: 'block-lambda',
    description: 'Lambda with block body',
    tags: ['lambda', 'block'],
    code: `int32 factorial = (int32 n) -> {
    return n <= 1 ? 1 : n * factorial(n - 1);
};`,
  },
  {
    name: 'type-cast',
    description: 'Type casting between numeric types',
    tags: ['cast', 'types'],
    code: `float64 toFloat = (int32 n) -> float64(n);
int32 fromFloat = (float64 f) -> int32(f);`,
  },
  {
    name: 'array-operations',
    description: 'Working with arrays',
    tags: ['array', 'index'],
    code: `int32[] nums = [1, 2, 3, 4, 5];
int32 first = nums[0];
int32 len = nums.length;`,
  },
  {
    name: 'template-literal',
    description: 'Template literals with interpolation',
    tags: ['template', 'string'],
    code: `string name = "Alice";
int32 age = 30;
string msg = \`Hello \${name}, you are \${age} years old!\`;`,
  },
  {
    name: 'ternary',
    description: 'Ternary expression',
    tags: ['ternary', 'conditional'],
    code: `int32 abs = (int32 x) -> x < 0 ? -x : x;`,
  },
  {
    name: 'member-access',
    description: 'Member access on objects',
    tags: ['member', 'access'],
    code: `import io.stdlib;

start(string[] args) -> {
    string s = "hello";
    int32 len = s.length;
    stdlib.print(\`Length: \${len}\`);
    return 0;
};`,
  },
  {
    name: 'public-binding',
    description: 'Public function binding with modifiers',
    tags: ['modifier', 'public', 'lambda'],
    code: `public int32 multiply = (int32 a, int32 b) -> a * b;
private final float64 PI = 3.141592653589793;`,
  },
  {
    name: 'throw',
    description: 'Throwing errors',
    tags: ['throw', 'error'],
    code: `int32 divide = (int32 a, int32 b) -> {
    b == 0 ? throw "division by zero" : null;
    return a / b;
};`,
  },
  {
    name: 'import-alias',
    description: 'Importing a module with an alias',
    tags: ['import', 'alias', 'v2'],
    code: `import io.stdlib as std;

start(string[] args) -> {
    std.print("hello");
    return 0;
};`,
  },
  {
    name: 'import-wildcard',
    description: 'Wildcard import of all direct exports',
    tags: ['import', 'wildcard', 'v2'],
    code: `import io.stdlib.*;

start(string[] args) -> {
    print("hello");
    return 0;
};`,
  },
  {
    name: 'import-selective',
    description: 'Selective import of specific names',
    tags: ['import', 'selective', 'v2'],
    code: `import io.stdlib.{print, readLine};

start(string[] args) -> {
    var line = readLine();
    print(line);
    return 0;
};`,
  },
  {
    name: 'export-binding',
    description: 'Exporting a top-level binding',
    tags: ['export', 'modifier', 'v2'],
    code: `export int32 add = (int32 a, int32 b) -> a + b;
export final float64 TAU = 6.283185307179586;`,
  },
  {
    name: 'increment-decrement',
    description: 'Prefix and postfix increment/decrement operators',
    tags: ['operator', 'increment', 'decrement', 'v2'],
    code: `start(string[] args) -> {
    var x = 5;
    x++;
    ++x;
    x--;
    --x;
    return x;
};`,
  },
  {
    name: 'varargs',
    description: 'Variadic parameter function',
    tags: ['varargs', 'v2'],
    code: `int32 sum = (int32... nums) -> {
    var total = 0;
    return total;
};`,
  },
  {
    name: 'discard',
    description: 'Discard expression to ignore a value',
    tags: ['discard', 'v2'],
    code: `start(string[] args) -> {
    _ = computeSomething();
    return 0;
};`,
  },
  {
    name: 'java-type-aliases',
    description: 'Java-style primitive type aliases',
    tags: ['types', 'alias', 'v2'],
    code: `int x = 42;
long big = 9999999999;
double pi = 3.14159;
float f = 1.5;
byte b = 0xFF;
short s = 256;`,
  },
  {
    name: 'tagged-union',
    description: 'Tagged union declaration with generic parameters and variant construction',
    tags: ['union', 'generics', 'v2'],
    code: `union Option<T> { Some(T), None };
union Result<T, E> { Ok(T), Err(E) };
union Direction { North, South, East, West };

start(string[] args) -> {
  Option<int32> x = Option.Some(42);
  Option<int32> nothing = Option.None;
  Direction d = Direction.North;
};`,
  },
  {
    name: 'heterogeneous-array',
    description: 'Heterogeneous array with arr<?> type annotation',
    tags: ['array', 'generics', 'v2'],
    code: `start(string[] args) -> {
  arr<?> mixed = [1, "hello", true];
};`,
  },
  {
    name: 'alpha-threading-aliases',
    description: 'Alpha.1 threading, mutex, type alias, and numeric literal separator example',
    tags: ['alpha-1', 'spawn', 'thread', 'mutex', 'type-alias', 'numeric-literal'],
    code: `type Counter = int32;

start(string[] args) -> {
    Counter shared = 1_000;
    Mutex guard = Mutex.new();
    void work = () -> {
        guard.lock();
        Counter next = shared + 0x10_00;
        shared = next;
        guard.unlock();
        exit;
    };
    Thread worker = spawn work;
    worker.join();
    return 0;
};`,
  },
];

export const EXAMPLES: Example[] = [...BASE_EXAMPLES, ...EXAMPLES_V3];
