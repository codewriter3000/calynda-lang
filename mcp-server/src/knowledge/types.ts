export interface TypeInfo {
  name: string;
  description: string;
  size?: string;
  range?: string;
  examples: string[];
}

export const TYPE_DOCS: Record<string, TypeInfo> = {
  int8: { name: 'int8', description: 'Signed 8-bit integer', size: '1 byte', range: '-128 to 127', examples: ['int8 x = 42;', 'int8 y = -100;'] },
  int16: { name: 'int16', description: 'Signed 16-bit integer', size: '2 bytes', range: '-32768 to 32767', examples: ['int16 x = 1000;'] },
  int32: { name: 'int32', description: 'Signed 32-bit integer', size: '4 bytes', range: '-2^31 to 2^31-1', examples: ['int32 count = 0;', 'int32 x = -500000;'] },
  int64: { name: 'int64', description: 'Signed 64-bit integer', size: '8 bytes', range: '-2^63 to 2^63-1', examples: ['int64 bigNum = 9999999999;'] },
  uint8: { name: 'uint8', description: 'Unsigned 8-bit integer', size: '1 byte', range: '0 to 255', examples: ['uint8 byte = 255;'] },
  uint16: { name: 'uint16', description: 'Unsigned 16-bit integer', size: '2 bytes', range: '0 to 65535', examples: ['uint16 port = 8080;'] },
  uint32: { name: 'uint32', description: 'Unsigned 32-bit integer', size: '4 bytes', range: '0 to 2^32-1', examples: ['uint32 flags = 0xFF;'] },
  uint64: { name: 'uint64', description: 'Unsigned 64-bit integer', size: '8 bytes', range: '0 to 2^64-1', examples: ['uint64 bigUnsigned = 18446744073709551615;'] },
  float32: { name: 'float32', description: 'IEEE 754 32-bit floating point', size: '4 bytes', examples: ['float32 pi = 3.14;', 'float32 e = 2.718;'] },
  float64: { name: 'float64', description: 'IEEE 754 64-bit floating point', size: '8 bytes', examples: ['float64 precise = 3.141592653589793;'] },
  bool: { name: 'bool', description: 'Boolean true/false value', size: '1 byte', examples: ['bool flag = true;', 'bool done = false;'] },
  char: { name: 'char', description: 'Single Unicode character', size: '4 bytes', examples: ["char letter = 'A';", "char newline = '\\n';"] },
  string: { name: 'string', description: 'Immutable UTF-8 string', examples: ['string greeting = "hello";', 'string name = "world";'] },
  void: { name: 'void', description: 'No return value (functions implicitly return null)', examples: ['void doSomething = () -> { exit; };'] },
  thread: { name: 'Thread', description: 'Opaque handle to a running thread. Created by spawning a zero-argument void callable. Supports .join() and .cancel(). Cancellation follows pthread cancellation semantics.', examples: ['Thread worker = spawn () -> { exit; };', 'worker.join();', 'worker.cancel();'] },
  future: { name: 'Future<T>', description: 'Opaque handle to a spawned zero-argument non-void callable. Supports .get() and .cancel().', examples: ['Future<int32> job = spawn () -> 42;', 'int32 value = job.get();', 'job.cancel();'] },
  mutex: { name: 'Mutex', description: 'Mutual exclusion lock. Created with Mutex.new(). Supports .lock() and .unlock().', examples: ['Mutex guard = Mutex.new();', 'guard.lock();', 'guard.unlock();'] },
  atomic: { name: 'Atomic<T>', description: 'Alpha.2 atomic cell for first-class single-word runtime values. Construct with Atomic.new(value); supported operations are load, store, and exchange.', examples: ['Atomic<int32> cell = Atomic.new(0);', 'int32 current = cell.load();', 'cell.store(1);', 'int32 old = cell.exchange(2);'] },
};
