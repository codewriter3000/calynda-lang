#define _POSIX_C_SOURCE 200809L

#include "car.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool write_uint32_le(FILE *out, uint32_t value) {
    uint8_t bytes[4];

    bytes[0] = (uint8_t)(value & 0xFF);
    bytes[1] = (uint8_t)((value >> 8) & 0xFF);
    bytes[2] = (uint8_t)((value >> 16) & 0xFF);
    bytes[3] = (uint8_t)((value >> 24) & 0xFF);
    return fwrite(bytes, 1, 4, out) == 4;
}

bool car_archive_write_to_stream(const CarArchive *archive, FILE *out) {
    uint8_t magic[4];
    size_t i;

    if (!archive || !out) {
        return false;
    }

    /* Write magic header */
    magic[0] = CAR_MAGIC_0;
    magic[1] = CAR_MAGIC_1;
    magic[2] = CAR_MAGIC_2;
    magic[3] = CAR_VERSION;
    if (fwrite(magic, 1, 4, out) != 4) {
        return false;
    }

    /* Write file count */
    if (!write_uint32_le(out, (uint32_t)archive->file_count)) {
        return false;
    }

    /* Write each file */
    for (i = 0; i < archive->file_count; i++) {
        const CarFile *file = &archive->files[i];
        size_t path_length = strlen(file->path);

        if (!write_uint32_le(out, (uint32_t)path_length)) {
            return false;
        }
        if (fwrite(file->path, 1, path_length, out) != path_length) {
            return false;
        }
        if (!write_uint32_le(out, (uint32_t)file->content_length)) {
            return false;
        }
        if (file->content_length > 0 &&
            fwrite(file->content, 1, file->content_length, out) != file->content_length) {
            return false;
        }
    }

    return true;
}

bool car_archive_write(const CarArchive *archive, const char *path) {
    FILE *file;
    bool result;

    if (!archive || !path) {
        return false;
    }

    file = fopen(path, "wb");
    if (!file) {
        return false;
    }

    result = car_archive_write_to_stream(archive, file);
    if (fclose(file) != 0) {
        return false;
    }
    return result;
}
