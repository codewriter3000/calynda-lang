#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int build_native_wait_for_child_exit(pid_t child) {
    int status;

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return -1;
}

char *build_native_read_entire_file(const char *path) {
    FILE *file;
    char *buffer;
    long size;
    size_t read_size;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: failed to seek: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0) {
        fprintf(stderr, "%s: failed to size file: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: failed to rewind: %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "%s: out of memory while reading file\n", path);
        fclose(file);
        return NULL;
    }
    read_size = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    if (read_size != (size_t)size) {
        fprintf(stderr, "%s: failed to read file contents\n", path);
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

static const char *bn_diag_find_line(const char *source, int target_line) {
    int cur = 1;
    if (target_line <= 0 || !source) return NULL;
    while (*source) {
        if (cur == target_line) return source;
        if (*source++ == '\n') cur++;
    }
    return (cur == target_line) ? source : NULL;
}

void build_native_print_diagnostic(const char *path, const char *source,
                                    int line, int col_start, int col_end,
                                    const char *kind, const char *message) {
    const char *line_ptr;
    int line_len, gutter, i;

    if (line > 0 && col_start > 0) {
        fprintf(stderr, "%s:%d:%d: %s: %s\n",
                path ? path : "?", line, col_start, kind, message);
    } else if (line > 0) {
        fprintf(stderr, "%s:%d: %s: %s\n",
                path ? path : "?", line, kind, message);
    } else {
        fprintf(stderr, "%s: %s: %s\n",
                path ? path : "?", kind, message);
    }

    if (!source || line <= 0) return;

    gutter = 1;
    for (i = line; i >= 10; i /= 10) gutter++;

    if (line > 1) {
        line_ptr = bn_diag_find_line(source, line - 1);
        if (line_ptr) {
            line_len = 0;
            while (line_ptr[line_len] && line_ptr[line_len] != '\n') line_len++;
            fprintf(stderr, " %*d | %.*s\n", gutter, line - 1, line_len, line_ptr);
        }
    }

    line_ptr = bn_diag_find_line(source, line);
    if (!line_ptr) return;
    line_len = 0;
    while (line_ptr[line_len] && line_ptr[line_len] != '\n') line_len++;
    fprintf(stderr, " %*d | %.*s\n", gutter, line, line_len, line_ptr);

    if (col_start > 0) {
        int cs = col_start;
        int ce = (col_end >= col_start && col_end > 0) ? col_end : col_start;
        int carets = ce - cs + 1;
        if (carets < 1) carets = 1;
        fprintf(stderr, " %*s | ", gutter, "");
        for (i = 1; i < cs; i++) fputc(' ', stderr);
        for (i = 0; i < carets; i++) fputc('^', stderr);
        fputc('\n', stderr);
    }
}

bool build_native_executable_directory(char *buffer, size_t buffer_size) {
    ssize_t written;
    char *slash;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    written = readlink("/proc/self/exe", buffer, buffer_size - 1);
    if (written < 0 || (size_t)written >= buffer_size) {
        return false;
    }
    buffer[written] = '\0';

    slash = strrchr(buffer, '/');
    if (!slash) {
        return false;
    }
    *slash = '\0';
    return true;
}

bool build_native_write_temp_assembly(const char *assembly, char *path_buffer, size_t buffer_size) {
    char template_path[] = "/tmp/calynda-native-XXXXXX";
    FILE *file;
    int fd;

    if (!assembly || !path_buffer || buffer_size == 0) {
        return false;
    }

    fd = mkstemp(template_path);
    if (fd < 0) {
        return false;
    }

    file = fdopen(fd, "wb");
    if (!file) {
        close(fd);
        unlink(template_path);
        return false;
    }
    if (fputs(assembly, file) == EOF || fclose(file) != 0) {
        unlink(template_path);
        return false;
    }

    if (strlen(template_path) + 1 > buffer_size) {
        unlink(template_path);
        return false;
    }

    memcpy(path_buffer, template_path, strlen(template_path) + 1);
    return true;
}

int build_native_run_linker(const char *assembly_path,
                            const char *runtime_object_path,
                            const char *output_path,
                            bool is_boot) {
    char *boot_argv[] = {
        (char *)"gcc",
        (char *)"-nostdlib",
        (char *)"-static",
        (char *)"-no-pie",
        (char *)"-x",
        (char *)"assembler",
        (char *)assembly_path,
        (char *)"-x",
        (char *)"none",
        (char *)runtime_object_path,
        (char *)"-o",
        (char *)output_path,
        NULL
    };
    char *link_argv[] = {
        (char *)"gcc",
        (char *)"-pthread",
        (char *)"-no-pie",
        (char *)"-x",
        (char *)"assembler",
        (char *)assembly_path,
        (char *)"-x",
        (char *)"none",
        (char *)runtime_object_path,
        (char *)"-o",
        (char *)output_path,
        NULL
    };
    char **argv = is_boot ? boot_argv : link_argv;
    pid_t child;
    int spawn_error;

    spawn_error = posix_spawnp(&child, argv[0], NULL, NULL, argv, environ);
    if (spawn_error != 0) {
        fprintf(stderr, "%s: %s\n", argv[0], strerror(spawn_error));
        return -1;
    }

    return build_native_wait_for_child_exit(child);
}
