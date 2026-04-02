CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

# Source files
SRCS      = $(SRC_DIR)/tokenizer.c $(SRC_DIR)/ast.c $(SRC_DIR)/parser.c $(SRC_DIR)/ast_dump.c $(SRC_DIR)/symbol_table.c $(SRC_DIR)/type_resolution.c $(SRC_DIR)/type_checker.c $(SRC_DIR)/semantic_dump.c $(SRC_DIR)/hir.c $(SRC_DIR)/hir_dump.c $(SRC_DIR)/mir.c $(SRC_DIR)/mir_dump.c $(SRC_DIR)/bytecode.c $(SRC_DIR)/bytecode_dump.c $(SRC_DIR)/lir.c $(SRC_DIR)/lir_dump.c $(SRC_DIR)/codegen.c $(SRC_DIR)/codegen_dump.c $(SRC_DIR)/runtime_abi.c $(SRC_DIR)/runtime.c $(SRC_DIR)/machine.c $(SRC_DIR)/asm_emit.c
TEST_SRCS = $(TEST_DIR)/test_tokenizer.c $(TEST_DIR)/test_ast.c $(TEST_DIR)/test_parser.c $(TEST_DIR)/test_ast_dump.c $(TEST_DIR)/test_symbol_table.c $(TEST_DIR)/test_type_resolution.c $(TEST_DIR)/test_type_checker.c $(TEST_DIR)/test_semantic_dump.c $(TEST_DIR)/test_hir_dump.c $(TEST_DIR)/test_mir_dump.c $(TEST_DIR)/test_bytecode_dump.c $(TEST_DIR)/test_lir_dump.c $(TEST_DIR)/test_codegen_dump.c $(TEST_DIR)/test_machine_dump.c $(TEST_DIR)/test_runtime.c $(TEST_DIR)/test_asm_emit.c $(TEST_DIR)/test_build_native.c $(TEST_DIR)/test_calynda_cli.c

# Object files
TOKENIZER_OBJS       = $(BUILD_DIR)/tokenizer.o
AST_OBJS             = $(BUILD_DIR)/ast.o
PARSER_OBJS          = $(BUILD_DIR)/parser.o
AST_DUMP_OBJS        = $(BUILD_DIR)/ast_dump.o
SYMBOL_TABLE_OBJS    = $(BUILD_DIR)/symbol_table.o
TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/type_resolution.o
TYPE_CHECKER_OBJS    = $(BUILD_DIR)/type_checker.o
SEMANTIC_DUMP_OBJS   = $(BUILD_DIR)/semantic_dump.o
HIR_OBJS             = $(BUILD_DIR)/hir.o
HIR_DUMP_OBJS        = $(BUILD_DIR)/hir_dump.o
MIR_OBJS             = $(BUILD_DIR)/mir.o
MIR_DUMP_OBJS        = $(BUILD_DIR)/mir_dump.o
BYTECODE_OBJS        = $(BUILD_DIR)/bytecode.o
BYTECODE_DUMP_OBJS   = $(BUILD_DIR)/bytecode_dump.o
LIR_OBJS             = $(BUILD_DIR)/lir.o
LIR_DUMP_OBJS        = $(BUILD_DIR)/lir_dump.o
CODEGEN_OBJS         = $(BUILD_DIR)/codegen.o
CODEGEN_DUMP_OBJS    = $(BUILD_DIR)/codegen_dump.o
RUNTIME_ABI_OBJS     = $(BUILD_DIR)/runtime_abi.o
RUNTIME_OBJS         = $(BUILD_DIR)/runtime.o
MACHINE_OBJS         = $(BUILD_DIR)/machine.o
ASM_EMIT_OBJS        = $(BUILD_DIR)/asm_emit.o
DUMP_AST_OBJS        = $(BUILD_DIR)/dump_ast.o
DUMP_SEMANTICS_OBJS  = $(BUILD_DIR)/dump_semantics.o
EMIT_ASM_OBJS        = $(BUILD_DIR)/emit_asm.o
EMIT_BYTECODE_OBJS   = $(BUILD_DIR)/emit_bytecode.o
BUILD_NATIVE_OBJS    = $(BUILD_DIR)/build_native.o
CALYNDA_CLI_OBJS     = $(BUILD_DIR)/calynda.o
TEST_TOKENIZER_OBJS  = $(BUILD_DIR)/test_tokenizer.o
TEST_AST_OBJS        = $(BUILD_DIR)/test_ast.o
TEST_PARSER_OBJS     = $(BUILD_DIR)/test_parser.o
TEST_AST_DUMP_OBJS   = $(BUILD_DIR)/test_ast_dump.o
TEST_SYMBOL_TABLE_OBJS = $(BUILD_DIR)/test_symbol_table.o
TEST_TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/test_type_resolution.o
TEST_TYPE_CHECKER_OBJS = $(BUILD_DIR)/test_type_checker.o
TEST_SEMANTIC_DUMP_OBJS = $(BUILD_DIR)/test_semantic_dump.o
TEST_HIR_DUMP_OBJS   = $(BUILD_DIR)/test_hir_dump.o
TEST_MIR_DUMP_OBJS   = $(BUILD_DIR)/test_mir_dump.o
TEST_BYTECODE_DUMP_OBJS = $(BUILD_DIR)/test_bytecode_dump.o
TEST_LIR_DUMP_OBJS   = $(BUILD_DIR)/test_lir_dump.o
TEST_CODEGEN_DUMP_OBJS = $(BUILD_DIR)/test_codegen_dump.o
TEST_MACHINE_DUMP_OBJS = $(BUILD_DIR)/test_machine_dump.o
TEST_RUNTIME_OBJS    = $(BUILD_DIR)/test_runtime.o
TEST_ASM_EMIT_OBJS   = $(BUILD_DIR)/test_asm_emit.o
TEST_BUILD_NATIVE_OBJS = $(BUILD_DIR)/test_build_native.o
TEST_CALYNDA_CLI_OBJS = $(BUILD_DIR)/test_calynda_cli.o

# Test binaries
TEST_TOKENIZER_BIN = $(BUILD_DIR)/test_tokenizer
TEST_AST_BIN       = $(BUILD_DIR)/test_ast
TEST_PARSER_BIN    = $(BUILD_DIR)/test_parser
TEST_AST_DUMP_BIN  = $(BUILD_DIR)/test_ast_dump
TEST_SYMBOL_TABLE_BIN = $(BUILD_DIR)/test_symbol_table
TEST_TYPE_RESOLUTION_BIN = $(BUILD_DIR)/test_type_resolution
TEST_TYPE_CHECKER_BIN = $(BUILD_DIR)/test_type_checker
TEST_SEMANTIC_DUMP_BIN = $(BUILD_DIR)/test_semantic_dump
TEST_HIR_DUMP_BIN   = $(BUILD_DIR)/test_hir_dump
TEST_MIR_DUMP_BIN   = $(BUILD_DIR)/test_mir_dump
TEST_BYTECODE_DUMP_BIN = $(BUILD_DIR)/test_bytecode_dump
TEST_LIR_DUMP_BIN   = $(BUILD_DIR)/test_lir_dump
TEST_CODEGEN_DUMP_BIN = $(BUILD_DIR)/test_codegen_dump
TEST_MACHINE_DUMP_BIN = $(BUILD_DIR)/test_machine_dump
TEST_RUNTIME_BIN   = $(BUILD_DIR)/test_runtime
TEST_ASM_EMIT_BIN  = $(BUILD_DIR)/test_asm_emit
TEST_BUILD_NATIVE_BIN = $(BUILD_DIR)/test_build_native
TEST_CALYNDA_CLI_BIN = $(BUILD_DIR)/test_calynda_cli
TEST_BINS          = $(TEST_TOKENIZER_BIN) $(TEST_AST_BIN) $(TEST_PARSER_BIN) $(TEST_AST_DUMP_BIN) $(TEST_SYMBOL_TABLE_BIN) $(TEST_TYPE_RESOLUTION_BIN) $(TEST_TYPE_CHECKER_BIN) $(TEST_SEMANTIC_DUMP_BIN) $(TEST_HIR_DUMP_BIN) $(TEST_MIR_DUMP_BIN) $(TEST_BYTECODE_DUMP_BIN) $(TEST_LIR_DUMP_BIN) $(TEST_CODEGEN_DUMP_BIN) $(TEST_MACHINE_DUMP_BIN) $(TEST_RUNTIME_BIN) $(TEST_ASM_EMIT_BIN) $(TEST_BUILD_NATIVE_BIN) $(TEST_CALYNDA_CLI_BIN)

# Tool binaries
DUMP_AST_BIN       = $(BUILD_DIR)/dump_ast
DUMP_SEMANTICS_BIN = $(BUILD_DIR)/dump_semantics
EMIT_ASM_BIN       = $(BUILD_DIR)/emit_asm
EMIT_BYTECODE_BIN  = $(BUILD_DIR)/emit_bytecode
BUILD_NATIVE_BIN   = $(BUILD_DIR)/build_native
CALYNDA_BIN        = $(BUILD_DIR)/calynda

PREFIX ?= /usr/local
INSTALL_BIN_DIR ?= $(PREFIX)/bin

.PHONY: all clean test dump_ast dump_semantics emit_asm emit_bytecode build_native calynda install

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/tokenizer.o: $(SRC_DIR)/tokenizer.c $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast.c $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump.o: $(SRC_DIR)/ast_dump.c $(SRC_DIR)/ast_dump.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table.o: $(SRC_DIR)/symbol_table.c $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution.o: $(SRC_DIR)/type_resolution.c $(SRC_DIR)/type_resolution.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker.o: $(SRC_DIR)/type_checker.c $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump.o: $(SRC_DIR)/semantic_dump.c $(SRC_DIR)/semantic_dump.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir.o: $(SRC_DIR)/hir.c $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump.o: $(SRC_DIR)/hir_dump.c $(SRC_DIR)/hir_dump.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir.o: $(SRC_DIR)/mir.c $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_dump.o: $(SRC_DIR)/mir_dump.c $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode.o: $(SRC_DIR)/bytecode.c $(SRC_DIR)/bytecode.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump.o: $(SRC_DIR)/bytecode_dump.c $(SRC_DIR)/bytecode.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir.o: $(SRC_DIR)/lir.c $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_dump.o: $(SRC_DIR)/lir_dump.c $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen.o: $(SRC_DIR)/codegen.c $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_dump.o: $(SRC_DIR)/codegen_dump.c $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_abi.o: $(SRC_DIR)/runtime_abi.c $(SRC_DIR)/runtime_abi.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime.o: $(SRC_DIR)/runtime.c $(SRC_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine.o: $(SRC_DIR)/machine.c $(SRC_DIR)/machine.h $(SRC_DIR)/runtime_abi.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit.o: $(SRC_DIR)/asm_emit.c $(SRC_DIR)/asm_emit.h $(SRC_DIR)/machine.h $(SRC_DIR)/runtime.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_ast.o: $(SRC_DIR)/dump_ast.c $(SRC_DIR)/ast_dump.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_semantics.o: $(SRC_DIR)/dump_semantics.c $(SRC_DIR)/semantic_dump.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/emit_asm.o: $(SRC_DIR)/emit_asm.c $(SRC_DIR)/asm_emit.h $(SRC_DIR)/machine.h $(SRC_DIR)/runtime_abi.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/emit_bytecode.o: $(SRC_DIR)/emit_bytecode.c $(SRC_DIR)/bytecode.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/build_native.o: $(SRC_DIR)/build_native.c $(SRC_DIR)/asm_emit.h $(SRC_DIR)/machine.h $(SRC_DIR)/runtime_abi.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda.o: $(SRC_DIR)/calynda.c $(SRC_DIR)/asm_emit.h $(SRC_DIR)/bytecode.h $(SRC_DIR)/machine.h $(SRC_DIR)/runtime_abi.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer.o: $(TEST_DIR)/test_tokenizer.c $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/test_ast.o: $(TEST_DIR)/test_ast.c $(SRC_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser.o: $(TEST_DIR)/test_parser.c $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_dump.o: $(TEST_DIR)/test_ast_dump.c $(SRC_DIR)/ast_dump.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table.o: $(TEST_DIR)/test_symbol_table.c $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_resolution.o: $(TEST_DIR)/test_type_resolution.c $(SRC_DIR)/type_resolution.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker.o: $(TEST_DIR)/test_type_checker.c $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_semantic_dump.o: $(TEST_DIR)/test_semantic_dump.c $(SRC_DIR)/semantic_dump.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_hir_dump.o: $(TEST_DIR)/test_hir_dump.c $(SRC_DIR)/hir_dump.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump.o: $(TEST_DIR)/test_mir_dump.c $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_bytecode_dump.o: $(TEST_DIR)/test_bytecode_dump.c $(SRC_DIR)/bytecode.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_lir_dump.o: $(TEST_DIR)/test_lir_dump.c $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_codegen_dump.o: $(TEST_DIR)/test_codegen_dump.c $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_machine_dump.o: $(TEST_DIR)/test_machine_dump.c $(SRC_DIR)/machine.h $(SRC_DIR)/runtime_abi.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_runtime.o: $(TEST_DIR)/test_runtime.c $(SRC_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_asm_emit.o: $(TEST_DIR)/test_asm_emit.c $(SRC_DIR)/asm_emit.h $(SRC_DIR)/machine.h $(SRC_DIR)/runtime.h $(SRC_DIR)/codegen.h $(SRC_DIR)/lir.h $(SRC_DIR)/mir.h $(SRC_DIR)/hir.h $(SRC_DIR)/type_checker.h $(SRC_DIR)/type_resolution.h $(SRC_DIR)/symbol_table.h $(SRC_DIR)/parser.h $(SRC_DIR)/ast.h $(SRC_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native.o: $(TEST_DIR)/test_build_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_calynda_cli.o: $(TEST_DIR)/test_calynda_cli.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TOKENIZER_BIN): $(TOKENIZER_OBJS) $(TEST_TOKENIZER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_AST_BIN): $(AST_OBJS) $(TEST_AST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_PARSER_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(TEST_PARSER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_AST_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(AST_DUMP_OBJS) $(TEST_AST_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_SYMBOL_TABLE_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TEST_SYMBOL_TABLE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_TYPE_RESOLUTION_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(TYPE_RESOLUTION_OBJS) $(TEST_TYPE_RESOLUTION_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_TYPE_CHECKER_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(TEST_TYPE_CHECKER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_SEMANTIC_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(SEMANTIC_DUMP_OBJS) $(TEST_SEMANTIC_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_HIR_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(HIR_DUMP_OBJS) $(TEST_HIR_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_MIR_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(TEST_MIR_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_BYTECODE_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(TEST_BYTECODE_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_LIR_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TEST_LIR_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_CODEGEN_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(TEST_CODEGEN_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_MACHINE_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(MACHINE_OBJS) $(TEST_MACHINE_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_RUNTIME_BIN): $(RUNTIME_OBJS) $(TEST_RUNTIME_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_ASM_EMIT_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(TEST_ASM_EMIT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_BUILD_NATIVE_BIN): $(TEST_BUILD_NATIVE_OBJS) $(BUILD_NATIVE_BIN)
	$(CC) $(CFLAGS) $(TEST_BUILD_NATIVE_OBJS) -o $@

$(TEST_CALYNDA_CLI_BIN): $(TEST_CALYNDA_CLI_OBJS) $(CALYNDA_BIN)
	$(CC) $(CFLAGS) $(TEST_CALYNDA_CLI_OBJS) -o $@

$(DUMP_AST_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(AST_DUMP_OBJS) $(DUMP_AST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(DUMP_SEMANTICS_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(SEMANTIC_DUMP_OBJS) $(DUMP_SEMANTICS_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(EMIT_ASM_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(EMIT_ASM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(EMIT_BYTECODE_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(EMIT_BYTECODE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_NATIVE_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(RUNTIME_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(BUILD_NATIVE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(CALYNDA_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(RUNTIME_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(CALYNDA_CLI_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_BINS)
	./$(TEST_TOKENIZER_BIN)
	./$(TEST_AST_BIN)
	./$(TEST_PARSER_BIN)
	./$(TEST_AST_DUMP_BIN)
	./$(TEST_SYMBOL_TABLE_BIN)
	./$(TEST_TYPE_RESOLUTION_BIN)
	./$(TEST_TYPE_CHECKER_BIN)
	./$(TEST_SEMANTIC_DUMP_BIN)
	./$(TEST_HIR_DUMP_BIN)
	./$(TEST_MIR_DUMP_BIN)
	./$(TEST_BYTECODE_DUMP_BIN)
	./$(TEST_LIR_DUMP_BIN)
	./$(TEST_CODEGEN_DUMP_BIN)
	./$(TEST_MACHINE_DUMP_BIN)
	./$(TEST_RUNTIME_BIN)
	./$(TEST_ASM_EMIT_BIN)
	./$(TEST_BUILD_NATIVE_BIN)
	./$(TEST_CALYNDA_CLI_BIN)

dump_ast: $(DUMP_AST_BIN)

dump_semantics: $(DUMP_SEMANTICS_BIN)

emit_asm: $(EMIT_ASM_BIN)

emit_bytecode: $(EMIT_BYTECODE_BIN)

build_native: $(BUILD_NATIVE_BIN)

calynda: $(CALYNDA_BIN)

install: $(CALYNDA_BIN)
	mkdir -p $(INSTALL_BIN_DIR)
	install -m 755 $(CALYNDA_BIN) $(INSTALL_BIN_DIR)/calynda

clean:
	rm -rf $(BUILD_DIR)
