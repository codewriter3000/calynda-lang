#define _POSIX_C_SOURCE 200809L

#include "car.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static bool has_cal_extension(const char *name) {
    size_t len = strlen(name);

    if (len < 5) {
        return false;
    }
    return strcmp(name + len - 4, ".cal") == 0;
}

static bool scan_directory(CarArchive *archive, const char *base_dir,
                           const char *relative_prefix) {
    DIR *dir;
    struct dirent *entry;
    char fullpath[4096];
    char relpath[4096];

    dir = opendir(base_dir);
    if (!dir) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "cannot open directory: %s", base_dir);
        archive->has_error = true;
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", base_dir, entry->d_name);

        if (stat(fullpath, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            /* Build relative prefix for subdirectory */
            if (relative_prefix[0] != '\0') {
                snprintf(relpath, sizeof(relpath), "%s/%s",
                         relative_prefix, entry->d_name);
            } else {
                snprintf(relpath, sizeof(relpath), "%s", entry->d_name);
            }
            if (!scan_directory(archive, fullpath, relpath)) {
                closedir(dir);
                return false;
            }
        } else if (S_ISREG(st.st_mode) && has_cal_extension(entry->d_name)) {
            FILE *f;
            long fsize;
            char *content;
            size_t content_length;

            /* Build archive-relative path */
            if (relative_prefix[0] != '\0') {
                snprintf(relpath, sizeof(relpath), "%s/%s",
                         relative_prefix, entry->d_name);
            } else {
                snprintf(relpath, sizeof(relpath), "%s", entry->d_name);
            }

            /* Read file content */
            f = fopen(fullpath, "rb");
            if (!f) {
                snprintf(archive->error_message,
                         sizeof(archive->error_message),
                         "cannot open file: %s", fullpath);
                archive->has_error = true;
                closedir(dir);
                return false;
            }

            fseek(f, 0, SEEK_END);
            fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (fsize < 0) {
                fclose(f);
                snprintf(archive->error_message,
                         sizeof(archive->error_message),
                         "cannot determine size: %s", fullpath);
                archive->has_error = true;
                closedir(dir);
                return false;
            }

            content_length = (size_t)fsize;
            content = malloc(content_length + 1);
            if (!content) {
                fclose(f);
                snprintf(archive->error_message,
                         sizeof(archive->error_message),
                         "out of memory reading: %s", fullpath);
                archive->has_error = true;
                closedir(dir);
                return false;
            }

            if (content_length > 0 &&
                fread(content, 1, content_length, f) != content_length) {
                free(content);
                fclose(f);
                snprintf(archive->error_message,
                         sizeof(archive->error_message),
                         "read error: %s", fullpath);
                archive->has_error = true;
                closedir(dir);
                return false;
            }
            content[content_length] = '\0';
            fclose(f);

            if (!car_archive_add_file(archive, relpath, content,
                                      content_length)) {
                free(content);
                closedir(dir);
                return false;
            }
            free(content);
        }
    }

    closedir(dir);
    return true;
}

bool car_archive_add_directory(CarArchive *archive, const char *directory) {
    struct stat st;

    if (!archive || !directory) {
        return false;
    }

    if (stat(directory, &st) != 0 || !S_ISDIR(st.st_mode)) {
        snprintf(archive->error_message, sizeof(archive->error_message),
                 "not a directory: %s", directory);
        archive->has_error = true;
        return false;
    }

    return scan_directory(archive, directory, "");
}
