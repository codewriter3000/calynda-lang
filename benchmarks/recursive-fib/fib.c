#include <stdint.h>
#include <stdlib.h>

static int32_t fib(int32_t n) {
    return n <= 1 ? n : fib(n - 1) + fib(n - 2);
}

int main(int argc, char **argv) {
    int32_t runtime_delta = (int32_t)(argc - 1);
    int32_t n = 38 + runtime_delta - runtime_delta;

    (void) argv;
    return fib(n) % 251;
}