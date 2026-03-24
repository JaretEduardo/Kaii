#ifndef KAII_UTILS_FILE_IO_H
#define KAII_UTILS_FILE_IO_H

#include <stddef.h>

/*
 * Reads a whole file into a NUL-terminated buffer.
 * Ownership of the returned buffer belongs to the caller.
 */
char *read_file_to_string(const char *filepath, size_t *out_size);

#endif /* KAII_UTILS_FILE_IO_H */
