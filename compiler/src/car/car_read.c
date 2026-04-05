#define _POSIX_C_SOURCE 200809L

#include "car.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_uint32_le(const uint8_t *data) {
    return (uint32_t)data[0]
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

bool car_archive_read_from_memory(CarArchive *archive,
                                  const uint8_t *data,
                                  size_t data_length) {
    uint32_t file_count;
    size_t offset = 0;
    size_t i;

    if (!archive || !data) {
        return false;
    }

    /* Read and validate magic header */
    if (data_length < 8) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "Archive too small to contain a valid header.");
        archive->has_error = true;
        return false;
    }

    if (data[0] != CAR_MAGIC_0 || data[1] != CAR_MAGIC_1 ||
        data[2] != CAR_MAGIC_2 || data[3] != CAR_VERSION) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "Invalid CAR archive magic or unsupported version.");
        archive->has_error = true;
        return false;
    }
    offset = 4;

    file_count = read_uint32_le(data + offset);
    offset += 4;

    for (i = 0; i < file_count; i++) {
        uint32_t path_length;
        uint32_t content_length;
        char *path;
        char *content;

        /* Read path_length */
        if (offset + 4 > data_length) {
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu path length.", i);
            archive->has_error = true;
            return false;
        }
        path_length = read_uint32_le(data + offset);
        offset += 4;

        /* Read path */
        if (offset + path_length > data_length) {
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu path data.", i);
            archive->has_error = true;
            return false;
        }
        path = malloc(path_length + 1);
        if (!path) {
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Out of memory reading file %zu path.", i);
            archive->has_error = true;
            return false;
        }
        memcpy(path, data + offset, path_length);
        path[path_length] = '\0';
        offset += path_length;

        /* Read content_length */
        if (offset + 4 > data_length) {
            free(path);
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu content length.", i);
            archive->has_error = true;
            return false;
        }
        content_length = read_uint32_le(data + offset);
        offset += 4;

        /* Read content */
        if (offset + content_length > data_length) {
            free(path);
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Truncated archive at file %zu content data.", i);
            archive->has_error = true;
            return false;
        }
        content = malloc(content_length + 1);
        if (!content) {
            free(path);
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Out of memory reading file %zu content.", i);
            archive->has_error = true;
            return false;
        }
        memcpy(content, data + offset, content_length);
        content[content_length] = '\0';
        offset += content_length;

        if (!car_archive_add_file(archive, path, content, content_length)) {
            free(path);
            free(content);
            return false;
        }
        free(path);
        free(content);
    }

    return true;
}

bool car_archive_read(CarArchive *archive, const char *path) {
    FILE *file;
    uint8_t *data;
    long size;
    size_t read_size;
    bool result;

    if (!archive || !path) {
        return false;
    }

    file = fopen(path, "rb");
    if (!file) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: %s", path, strerror(errno));
        archive->has_error = true;
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to seek.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }
    size = ftell(file);
    if (size < 0) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to determine file size.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to rewind.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }

    data = malloc((size_t)size);
    if (!data) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: out of memory.", path);
        archive->has_error = true;
        fclose(file);
        return false;
    }

    read_size = fread(data, 1, (size_t)size, file);
    fclose(file);

    if (read_size != (size_t)size) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "%s: failed to read complete file.", path);
        archive->has_error = true;
        free(data);
        return false;
    }

    result = car_archive_read_from_memory(archive, data, (size_t)size);
    free(data);
    return result;
}
