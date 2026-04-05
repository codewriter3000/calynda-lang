#define _POSIX_C_SOURCE 200809L

#include "car.h"

#include <stdlib.h>
#include <string.h>

void car_archive_init(CarArchive *archive) {
    if (!archive) {
        return;
    }
    memset(archive, 0, sizeof(*archive));
}

void car_archive_free(CarArchive *archive) {
    size_t i;

    if (!archive) {
        return;
    }

    for (i = 0; i < archive->file_count; i++) {
        free(archive->files[i].path);
        free(archive->files[i].content);
    }
    free(archive->files);
    memset(archive, 0, sizeof(*archive));
}

bool car_archive_add_file(CarArchive *archive,
                          const char *path,
                          const char *content,
                          size_t content_length) {
    CarFile *file;
    size_t new_capacity;
    CarFile *new_files;

    if (!archive || !path || !content) {
        return false;
    }

    if (archive->file_count >= archive->file_capacity) {
        new_capacity = archive->file_capacity == 0 ? 8 : archive->file_capacity * 2;
        new_files = realloc(archive->files, new_capacity * sizeof(CarFile));
        if (!new_files) {
            snprintf(archive->error_message, sizeof(archive->error_message),
                     "Out of memory while adding file to archive.");
            archive->has_error = true;
            return false;
        }
        archive->files = new_files;
        archive->file_capacity = new_capacity;
    }

    file = &archive->files[archive->file_count];
    file->path = strdup(path);
    file->content = malloc(content_length + 1);
    if (!file->path || !file->content) {
        free(file->path);
        free(file->content);
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "Out of memory while copying file data.");
        archive->has_error = true;
        return false;
    }

    memcpy(file->content, content, content_length);
    file->content[content_length] = '\0';
    file->content_length = content_length;
    archive->file_count++;
    return true;
}

const CarFile *car_archive_find_file(const CarArchive *archive,
                                     const char *path) {
    size_t i;

    if (!archive || !path) {
        return NULL;
    }

    for (i = 0; i < archive->file_count; i++) {
        if (strcmp(archive->files[i].path, path) == 0) {
            return &archive->files[i];
        }
    }
    return NULL;
}

const char *car_archive_get_error(const CarArchive *archive) {
    if (!archive || !archive->has_error) {
        return NULL;
    }
    return archive->error_message;
}
