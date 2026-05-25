fn fib(n: i32) -> i32 {
    if n <= 1 {
        n
    } else {
        fib(n - 1) + fib(n - 2)
    }
}

fn main() {
    let runtime_delta = std::env::args_os().count() as i32 - 1;
    let n = 38 + runtime_delta - runtime_delta;

    std::process::exit(fib(n) % 251);
}