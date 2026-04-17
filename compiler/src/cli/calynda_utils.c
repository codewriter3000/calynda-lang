#define _POSIX_C_SOURCE 200809L

#include "calynda_internal.h"

#include <errno.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int calynda_wait_for_child_exit(pid_t child) {
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

char *calynda_read_entire_file(const char *path) {
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

bool calynda_executable_directory(char *buffer, size_t buffer_size) {
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

bool calynda_write_temp_file(const char *prefix,
                             const char *contents,
                             char *path_buffer,
                             size_t buffer_size) {
    char template_path[64];
    FILE *file;
    int fd;

    if (!prefix || !contents || !path_buffer || buffer_size == 0) {
        return false;
    }

    if (snprintf(template_path, sizeof(template_path), "/tmp/%s-XXXXXX", prefix) < 0) {
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
    if (fputs(contents, file) == EOF || fclose(file) != 0) {
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

int calynda_run_linker(const char *assembly_path,
                       const char *runtime_object_path,
                       const char *output_path,
                       const TargetDescriptor *target,
                       bool is_boot) {
    char *boot_argv[] = {
        (char *)target->linker_command,
        (char *)"-nostdlib",
        (char *)"-static",
        (char *)"-x",
        (char *)"assembler",
        (char *)assembly_path,
        (char *)"-o",
        (char *)output_path,
        NULL
    };
    char *link_argv[] = {
        (char *)target->linker_command,
        (char *)target->linker_flags,
        (char *)"-pthread",
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

    return calynda_wait_for_child_exit(child);
}

int calynda_run_child_process(const char *path, char *const argv[]) {
    pid_t child;
    int spawn_error;

    spawn_error = posix_spawn(&child, path, NULL, NULL, argv, environ);
    if (spawn_error != 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(spawn_error));
        return -1;
    }

    return calynda_wait_for_child_exit(child);
}
