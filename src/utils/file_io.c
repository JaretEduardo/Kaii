#include "file_io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

char *read_file_to_string(const char *filepath, size_t *out_size) {
    FILE *file = NULL;
    long file_size_long = 0;
    size_t file_size = 0;
    size_t bytes_read = 0;
    char *buffer = NULL;

    if (out_size != NULL) {
        *out_size = 0;
    }

    if (filepath == NULL) {
        fprintf(stderr, "read_file_to_string: filepath is NULL\n");
        return NULL;
    }

    file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "read_file_to_string: failed to open '%s': %s\n", filepath, strerror(errno));
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "read_file_to_string: failed to seek end of '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    file_size_long = ftell(file);
    if (file_size_long < 0) {
        fprintf(stderr, "read_file_to_string: failed to get size of '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "read_file_to_string: failed to rewind '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    file_size = (size_t)file_size_long;
    if (file_size_long > 0 && file_size == 0) {
        fprintf(stderr, "read_file_to_string: file size overflow for '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    if (file_size > (SIZE_MAX - 1u)) {
        fprintf(stderr, "read_file_to_string: file too large to allocate '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    buffer = (char *)malloc(file_size + 1u);
    if (buffer == NULL) {
        fprintf(stderr, "read_file_to_string: out of memory while reading '%s'\n", filepath);
        fclose(file);
        return NULL;
    }

    if (file_size != 0u) {
        bytes_read = fread(buffer, 1u, file_size, file);
        if (bytes_read != file_size) {
            fprintf(stderr, "read_file_to_string: short read for '%s' (%zu/%zu bytes)\n", filepath, bytes_read, file_size);
            free(buffer);
            fclose(file);
            return NULL;
        }
    }

    buffer[file_size] = '\0';

    if (fclose(file) != 0) {
        fprintf(stderr, "read_file_to_string: warning: failed to close '%s'\n", filepath);
    }

    if (out_size != NULL) {
        *out_size = file_size;
    }

    return buffer;
}
