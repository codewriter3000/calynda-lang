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
