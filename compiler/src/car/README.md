# CAR (Calynda ARchive)

## Overview

The CAR module implements the Calynda ARchive format for packaging source code. CAR files bundle multiple source files into a single binary archive for distribution and compilation.

## Purpose

- Package multiple source files into a single archive
- Enable library distribution without exposing original directory structure
- Support efficient module loading
- Facilitate source-level caching
- Enable source embedding in executables (future)

## Key Components

### Files

- **car.c/h**: CAR archive API and data structures
- **car_read.c**: Reading and parsing CAR archives
- **car_write.c**: Writing and serializing CAR archives
- **car_dir.c**: Directory tree scanning for archive creation

## Binary Format

### File Structure
```
Magic:        "CAR\x01" (4 bytes)
File Count:   uint32_le (4 bytes)

For each file:
    Path Length:    uint32_le
    Path:           UTF-8 string (relative path)
    Content Length: uint32_le
    Content:        UTF-8 source code
```

### Example
```
43 41 52 01              # Magic: "CAR", version 1
02 00 00 00              # File count: 2

0B 00 00 00              # Path length: 11
73 72 63 2F 6D 61 69 6E  # Path: "src/main.cal"
2E 63 61 6C
12 00 00 00              # Content length: 18
...                      # Content (18 bytes)

0F 00 00 00              # Path length: 15
73 72 63 2F 75 74 69 6C  # Path: "src/utils.cal"
73 2E 63 61 6C
2A 00 00 00              # Content length: 42
...                      # Content (42 bytes)
```

## Data Structures

### CAR Archive
```c
typedef struct {
    CarFile *files;
    size_t   file_count;
    char     error_message[256];
    bool     has_error;
} CarArchive;
```

### CAR File
```c
typedef struct {
    char   *path;            // Relative path (e.g., "src/main.cal")
    char   *content;         // Source code content
    size_t  content_length;  // Length in bytes
} CarFile;
```

## Usage

### Creating a CAR Archive from Directory
```c
CarArchive archive;
car_archive_init(&archive);

// Add all .cal files from directory
if (!car_archive_add_directory(&archive, "./src")) {
    fprintf(stderr, "Error: %s\n", car_archive_get_error(&archive));
    return 1;
}

// Write to file
if (!car_archive_write(&archive, "mylib.car")) {
    fprintf(stderr, "Error: %s\n", car_archive_get_error(&archive));
    return 1;
}

car_archive_free(&archive);
```

### Reading a CAR Archive
```c
CarArchive archive;
car_archive_init(&archive);

if (!car_archive_read(&archive, "mylib.car")) {
    fprintf(stderr, "Error: %s\n", car_archive_get_error(&archive));
    return 1;
}

// Find specific file
const CarFile *file = car_archive_find_file(&archive, "src/main.cal");
if (file) {
    printf("Content:\n%.*s\n", (int)file->content_length, file->content);
}

car_archive_free(&archive);
```

### Adding Files Programmatically
```c
CarArchive archive;
car_archive_init(&archive);

const char *source = "package com.example;\n// ...";
car_archive_add_file(&archive, "com/example/main.cal", source, strlen(source));

car_archive_write(&archive, "output.car");
car_archive_free(&archive);
```

## Directory Scanning

The `car_dir.c` module recursively scans directories:
- Finds all `.cal` files
- Preserves directory structure as relative paths
- Skips hidden files and directories
- Follows symlinks (with cycle detection)

## Use Cases

### Library Distribution
Package a library as a CAR file:
```bash
calynda car create ./stdlib -o stdlib.car
```

Distribute `stdlib.car` instead of source directory.

### Module Loading
The compiler can load modules from CAR archives:
```calynda
import com.example.utils from "libs/utils.car";
```

### Build Artifacts
CAR files can be intermediate build artifacts:
- Faster than recompiling sources
- Preserves source for debugging
- Smaller than object files for large projects

## Error Handling

CAR operations report errors:
- Invalid file format
- Corrupted data
- I/O errors
- Path traversal attempts
- Memory allocation failures

## Future Enhancements

- **Compression**: gzip or zstd compression
- **Checksums**: Verify archive integrity
- **Metadata**: Author, version, dependencies
- **Incremental Updates**: Append or modify files efficiently
- **Encryption**: Protect proprietary source code
- **Signatures**: Verify archive authenticity

## Design Notes

- CAR format is simple and efficient
- All strings are UTF-8 encoded
- Paths use forward slashes (Unix-style)
- Format version allows future extensions
- Archives can be read without unpacking to disk