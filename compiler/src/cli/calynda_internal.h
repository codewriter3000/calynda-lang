#ifndef CALYNDA_INTERNAL_H
#define CALYNDA_INTERNAL_H

#include "asm_emit.h"
#include "bytecode.h"
#include "car.h"
#include "parser.h"
#include "target.h"

#include <limits.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    CALYNDA_EMIT_MODE_ASM = 0,
    CALYNDA_EMIT_MODE_BYTECODE
} CalyndaEmitMode;

/* calynda_compile.c */
int calynda_compile_to_machine_program(const char *path,
                                       MachineProgram *machine_program,
                                       const TargetDescriptor *target);
int calynda_compile_to_bytecode_program(const char *path,
                                        BytecodeProgram *bytecode_program);

/* calynda_car.c */
int calynda_compile_car_to_machine_program(const CarArchive *archive,
                                           MachineProgram *machine_program,
                                           const TargetDescriptor *target);

/* calynda_utils.c */
char *calynda_read_entire_file(const char *path);
bool calynda_executable_directory(char *buffer, size_t buffer_size);
bool calynda_write_temp_file(const char *prefix,
                             const char *contents,
                             char *path_buffer,
                             size_t buffer_size);
int calynda_run_linker(const char *assembly_path,
                       const char *runtime_object_path,
                       const char *output_path,
                       const TargetDescriptor *target,
                       bool is_boot);
int calynda_run_child_process(const char *path, char *const argv[]);

/* calynda.c */
bool has_car_extension(const char *path);

/* calynda_commands.c */
int calynda_build_program_file(const char *source_path,
                               const char *output_path,
                               const TargetDescriptor *target);
int calynda_run_program_file(const char *source_path,
                             int argc, char **argv,
                             const TargetDescriptor *target);
int calynda_build_car_file(const char *car_path,
                           const char *output_path,
                           const TargetDescriptor *target);
int calynda_pack_directory(const char *dir_path, const char *output_path);

#endif
