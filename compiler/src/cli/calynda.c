#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"

#include <string.h>

typedef int (*CalyndaCommandHandler)(const char *program_name,
                                     int argc,
                                     char **argv);

typedef struct {
    const char *name;
    CalyndaCommandHandler handler;
} CalyndaCommandSpec;

static const CalyndaCommandSpec k_command_specs[] = {
    { "asm", calynda_command_asm },
    { "bytecode", calynda_command_bytecode },
    { "build", calynda_command_build },
    { "pack", calynda_command_pack },
    { "run", calynda_command_run }
};

bool has_car_extension(const char *path) {
    size_t len = strlen(path);
    return len > 4 && strcmp(path + len - 4, ".car") == 0;
}

int main(int argc, char **argv) {
    const char *program_name = argc > 0 ? argv[0] : calynda_cli_name();
    size_t i;

    if (argc < 2) {
        calynda_print_usage(stderr, program_name);
        return 64;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        calynda_print_usage(stdout, program_name);
        return 0;
    }
    if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0) {
        calynda_print_version(stdout);
        return 0;
    }

    for (i = 0; i < sizeof(k_command_specs) / sizeof(k_command_specs[0]); i++) {
        const CalyndaCommandSpec *spec = &k_command_specs[i];

        if (strcmp(argv[1], spec->name) == 0) {
            return spec->handler(program_name, argc - 2, argv + 2);
        }
    }

    calynda_print_usage(stderr, program_name);
    return 64;
}
