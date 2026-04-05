#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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
    pid_t child;
    int status;

    child = fork();
    if (child < 0) {
        return -1;
    }

    if (child == 0) {
        if (is_boot) {
            execlp("gcc",
                   "gcc",
                   "-nostdlib",
                   "-static",
                   "-no-pie",
                   "-x",
                   "assembler",
                   assembly_path,
                   "-o",
                   output_path,
                   (char *)NULL);
        } else {
            execlp("gcc",
                   "gcc",
                   "-no-pie",
                   "-x",
                   "assembler",
                   assembly_path,
                   "-x",
                   "none",
                   runtime_object_path,
                   "-o",
                   output_path,
                   (char *)NULL);
        }
        _exit(127);
    }

    if (waitpid(child, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}
