# Calynda Philosophy
1. If it can be built without needing external C or Assembly adapters, it is unnecessary.
2. Make it look like Java first, or C if that's not possible.
3. Make it simple.

Working V2 grammar clone: `compiler/calynda_v2.ebnf`

# V2 - More Primitive Data Structures, New Function Styles, and Shorthands
1. Primitive Type Notations (just like Java)
- uint8 = byte
- int8 = sbyte
- uint16 = char
- int16 = short
- uint32 = uint
- int32 = int
- uint64 = ulong (may add L at the end)
- int64 = long (may add L at the end)
- bool = bool (stays the same)
- float32 = float
- float64 = double
- Allow both notations to be used
2. Generics
- Exact same syntax as Java
- ?[] arrays are evaluated at runtime. Unless explicitly specified in the declaration, the type of the value at the particular indice is the largest size that it could be.
    ?int[] intArr = [short(5), 7, 9]; <-- all types must be no larger than int
    ?[] intArr = [short(5), 7, 400]; <-- index 0 must be no larger than a 32-bit type, index 1 must be no larger than an 8-bit type, and index 2 must be no larger than an 16-bit type
4. Structures
- All structures are arr[?] but not all arr[?] are structures
- Upon declaration, you may choose to cache the structure so that creating an instance of this structure is done faster, or you may want to reserve space so if you want to create an instance of this structure it will have to figure out how to make it again
    struct Person {
        string firstName;
        string lastName;
        int age;
    }
5. Unions
- Allows different data types to be defined in the same structure
6. Internal access modifier for nested functions
- A nested function can be given the 'internal' access modifier so that it can only be called inside of the outer function's argument functions. You can also specify which argument function you can call it in.
7. ASM functions
- A function declaration that is in the form asm() -> {} will be written in Assembly. Semi-colons will still be used to distinguish between Assembly statements.
- Cryptographic verification will be used by default to mitigate against Assembly injection attacks, but this can be manually disabled.
8. Manual memory allocations
- manual() -> {} is used if you want to manually manage your own memory allocations.
- Inside of a manual function, you can use these statements for manual memory allocations:
    - * and & will work just like in C
    - malloc int; <-- Same as the C-equivalent for malloc(sizeof(int));
    - calloc 8, 1024 <-- Same as the C-equivalent for calloc(8, 1024);
    - calloc arr <-- Continuously allocate memory for the n items in arr and the size m of each item. This works the same for structures.
    - realloc works with the same syntax as calloc and comes with the same shorthand
    - free *arr <-- Frees the memory associated with the pointer for arr
9. Static functions and variables
- Just like C, a variable declared static is stored in the data segment instead of the stack
    - static int myNum = 26;
    - static anotherNum = 12; <-- Notation can be used where var is permitted
    - void loadGlobals = static() -> {}; <-- Every variable inside of this function must be static
    - static void execute = () -> {}; <-- The function is stored in the data segment instead of the stack
10. Enums
- Works just like C
11. ++ and -- postfix and prefix operators
12. One-statement method shortcut
- If a parameterized function inside of a called function is one statement long with 0 args, then you can use a shorthand notation like this:
    - Before:
        while(() -> x == 5, () -> {})
    - After:
        while(x == 5, () -> {})
13. Variable parameters
- int add = (int... nums) -> {}; <-- nums is an array that gets declared when add() is declared.
14. Void type
- Functions in parameters can now be typed as void. If you want to skip a void parameter, you can use 2 shorthands supposing the function is declared as void run = (void func, int num) -> {};
    - Shorthand 1: run(null, 7);
    - Shorthand 2: run(_, 7);

# V3 - Reflection and More Shorthands
1. Function in array notation
- A powerful new feature that allows you to take every statement in a function and turn it into an array of voids.
    void doIt = () -> {};
    void[] doItArr = void[doIt];
    doItArr[0](); <-- Calls just the first statement in doItArr();
2. Mapped functions
- Puts a function's statements into an arr[?] rather than an array
- Allows for a statement to return a value
    void doIt = (i) -> i++; i--;
    ?[] doItArr = void[doIt];
    println(`This will be 8: ${doItArr[1](9)});
- You only place parameters for variables in the specific statement from left to right
    void doIt = (i, j) -> i++; i+j; j++;
    ?[] doItArr = void[doIt];
    println(`This works too and also it's gonna be 10: #{doItArr[2](9)}); <-- This works because there is only 1 variable in the 3rd statement, which is j.

# V4 - Multithreading
1. Runnables
- Declare a function as runnable:
int add = runnable(a, b) -> a+b;
2. Start statement
- Starts a runnable function
start add(5, 5);
- Calling add() without start will just run it on the same thread
3. Shorthand
- Declaring a function as start will start a new thread every time the function is called
int add = start(a, b) -> a+b;
add(5, 5);
4. Thread Lifecycle
- New: Thread is created but has not been started
- Runnable: Ready to run and waiting for CPU allocation
- Running: Actively being executed
- Blocked: Waiting for a resource lock or another thread's actions
- Terminated: Execution has completed
5. Synchronized
- Makes sure that only one thread can access a function at a time
synchronized() -> {
    print("Only one thread can run this at a time");
}
6. Volatile
- Ensures changes to a variable are immediately visible to all threads by readiing directly from main memory
7. Lockables
- A lockable function provides more granular control than a synchronized function and allows you to use advanced features suchh as timed lock attempts and interruptible locking
8. More Thread Statements
sleep 100; <-- Pauses the thread for 100ms
join 1234; <-- Causes the current thread to wait until thread ID 1234 is willing to pause
isDaemon; <-- Treats this current thread as a daemon
notDaemon; <-- Treats this current thread as not a daemon
