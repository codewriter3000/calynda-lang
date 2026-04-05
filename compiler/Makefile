CC      = gcc

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

# Subdirectory layout
TOKENIZER_DIR    = $(SRC_DIR)/tokenizer
AST_DIR          = $(SRC_DIR)/ast
PARSER_DIR       = $(SRC_DIR)/parser
SEMA_DIR         = $(SRC_DIR)/sema
HIR_DIR          = $(SRC_DIR)/hir
MIR_DIR          = $(SRC_DIR)/mir
BYTECODE_DIR     = $(SRC_DIR)/bytecode
LIR_DIR          = $(SRC_DIR)/lir
CODEGEN_DIR      = $(SRC_DIR)/codegen
BACKEND_DIR      = $(SRC_DIR)/backend
RUNTIME_DIR      = $(SRC_DIR)/runtime
CAR_DIR          = $(SRC_DIR)/car
CLI_DIR          = $(SRC_DIR)/cli

INCLUDES = -I$(TOKENIZER_DIR) -I$(AST_DIR) -I$(PARSER_DIR) -I$(SEMA_DIR) \
           -I$(HIR_DIR) -I$(MIR_DIR) -I$(BYTECODE_DIR) -I$(LIR_DIR) \
           -I$(CODEGEN_DIR) -I$(BACKEND_DIR) -I$(RUNTIME_DIR) -I$(CAR_DIR) -I$(CLI_DIR)

CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g $(INCLUDES)

# Source files
SRCS      = $(TOKENIZER_DIR)/tokenizer.c $(TOKENIZER_DIR)/tokenizer_keywords.c $(TOKENIZER_DIR)/tokenizer_literals.c $(TOKENIZER_DIR)/tokenizer_punctuation.c $(AST_DIR)/ast.c $(AST_DIR)/ast_expressions.c $(AST_DIR)/ast_statements.c $(AST_DIR)/ast_declarations.c $(PARSER_DIR)/parser.c $(PARSER_DIR)/parser_utils.c $(PARSER_DIR)/parser_types.c $(PARSER_DIR)/parser_decl.c $(PARSER_DIR)/parser_union.c $(PARSER_DIR)/parser_stmt.c $(PARSER_DIR)/parser_expr.c $(PARSER_DIR)/parser_binary.c $(PARSER_DIR)/parser_postfix.c $(PARSER_DIR)/parser_literals.c $(PARSER_DIR)/parser_lookahead.c $(AST_DIR)/ast_dump.c $(AST_DIR)/ast_dump_names.c $(AST_DIR)/ast_dump_types.c $(AST_DIR)/ast_dump_expr.c $(AST_DIR)/ast_dump_stmt.c $(AST_DIR)/ast_dump_decl.c $(SEMA_DIR)/symbol_table.c $(SEMA_DIR)/symbol_table_query.c $(SEMA_DIR)/symbol_table_core.c $(SEMA_DIR)/symbol_table_registry.c $(SEMA_DIR)/symbol_table_imports.c $(SEMA_DIR)/symbol_table_predecl.c $(SEMA_DIR)/symbol_table_analyze.c $(SEMA_DIR)/symbol_table_analyze_expr.c $(SEMA_DIR)/type_resolution.c $(SEMA_DIR)/type_resolution_helpers.c $(SEMA_DIR)/type_resolution_resolve.c $(SEMA_DIR)/type_resolution_expr.c $(SEMA_DIR)/type_checker.c $(SEMA_DIR)/type_checker_check.c $(SEMA_DIR)/type_checker_names.c $(SEMA_DIR)/type_checker_types.c $(SEMA_DIR)/type_checker_convert.c $(SEMA_DIR)/type_checker_util.c $(SEMA_DIR)/type_checker_resolve.c $(SEMA_DIR)/type_checker_lambda.c $(SEMA_DIR)/type_checker_block.c $(SEMA_DIR)/type_checker_ops.c $(SEMA_DIR)/type_checker_expr.c $(SEMA_DIR)/type_checker_expr_ext.c $(SEMA_DIR)/type_checker_expr_more.c $(SEMA_DIR)/semantic_dump.c $(SEMA_DIR)/semantic_dump_builder.c $(SEMA_DIR)/semantic_dump_types.c $(SEMA_DIR)/semantic_dump_scope.c $(SEMA_DIR)/semantic_dump_symbols.c $(HIR_DIR)/hir.c $(HIR_DIR)/hir_memory.c $(HIR_DIR)/hir_helpers.c $(HIR_DIR)/hir_lower.c $(HIR_DIR)/hir_lower_stmt.c $(HIR_DIR)/hir_lower_expr.c $(HIR_DIR)/hir_lower_expr_ext.c $(HIR_DIR)/hir_lower_decls.c $(HIR_DIR)/hir_dump.c $(HIR_DIR)/hir_dump_helpers.c $(HIR_DIR)/hir_dump_expr.c $(HIR_DIR)/hir_dump_expr_ext.c $(MIR_DIR)/mir.c $(MIR_DIR)/mir_union.c $(MIR_DIR)/mir_capture.c $(MIR_DIR)/mir_capture_analysis.c $(MIR_DIR)/mir_value.c $(MIR_DIR)/mir_cleanup.c $(MIR_DIR)/mir_builders.c $(MIR_DIR)/mir_store.c $(MIR_DIR)/mir_lvalue.c $(MIR_DIR)/mir_control.c $(MIR_DIR)/mir_assign.c $(MIR_DIR)/mir_expr_helpers.c $(MIR_DIR)/mir_expr.c $(MIR_DIR)/mir_lower.c $(MIR_DIR)/mir_lambda.c $(MIR_DIR)/mir_dump.c $(MIR_DIR)/mir_dump_helpers.c $(MIR_DIR)/mir_dump_instr.c $(BYTECODE_DIR)/bytecode.c $(BYTECODE_DIR)/bytecode_memory.c $(BYTECODE_DIR)/bytecode_helpers.c $(BYTECODE_DIR)/bytecode_constants.c $(BYTECODE_DIR)/bytecode_lower_instr.c $(BYTECODE_DIR)/bytecode_lower.c $(BYTECODE_DIR)/bytecode_dump.c $(BYTECODE_DIR)/bytecode_dump_helpers.c $(BYTECODE_DIR)/bytecode_dump_instr.c $(BYTECODE_DIR)/bytecode_dump_names.c $(LIR_DIR)/lir.c $(LIR_DIR)/lir_memory.c $(LIR_DIR)/lir_helpers.c $(LIR_DIR)/lir_lower_instr.c $(LIR_DIR)/lir_lower_instr_ext.c $(LIR_DIR)/lir_lower_instr_stores.c $(LIR_DIR)/lir_lower.c $(LIR_DIR)/lir_dump.c $(LIR_DIR)/lir_dump_helpers.c $(LIR_DIR)/lir_dump_instr.c $(CODEGEN_DIR)/codegen.c $(CODEGEN_DIR)/codegen_names.c $(CODEGEN_DIR)/codegen_helpers.c $(CODEGEN_DIR)/codegen_infer.c $(CODEGEN_DIR)/codegen_build.c $(CODEGEN_DIR)/codegen_dump.c $(CODEGEN_DIR)/codegen_dump_helpers.c $(BACKEND_DIR)/runtime_abi.c $(BACKEND_DIR)/runtime_abi_names.c $(BACKEND_DIR)/runtime_abi_dump.c $(RUNTIME_DIR)/runtime.c $(RUNTIME_DIR)/runtime_names.c $(RUNTIME_DIR)/runtime_format.c $(RUNTIME_DIR)/runtime_ops.c $(RUNTIME_DIR)/runtime_union.c $(BACKEND_DIR)/machine.c $(BACKEND_DIR)/machine_dump.c $(BACKEND_DIR)/machine_helpers.c $(BACKEND_DIR)/machine_layout.c $(BACKEND_DIR)/machine_operand.c $(BACKEND_DIR)/machine_emit.c $(BACKEND_DIR)/machine_ops.c $(BACKEND_DIR)/machine_instr.c $(BACKEND_DIR)/machine_instr_rt.c $(BACKEND_DIR)/machine_instr_rt_ext.c $(BACKEND_DIR)/target.c $(BACKEND_DIR)/target_x86_64.c $(BACKEND_DIR)/target_aarch64.c $(BACKEND_DIR)/machine_build.c $(BACKEND_DIR)/asm_emit.c $(BACKEND_DIR)/asm_emit_helpers.c $(BACKEND_DIR)/asm_emit_symbols.c $(BACKEND_DIR)/asm_emit_operand.c $(BACKEND_DIR)/asm_emit_operand_ext.c $(BACKEND_DIR)/asm_emit_instr.c $(BACKEND_DIR)/asm_emit_instr_aarch64.c $(BACKEND_DIR)/asm_emit_sections.c $(BACKEND_DIR)/asm_emit_entry.c $(BACKEND_DIR)/asm_emit_entry_aarch64.c $(CAR_DIR)/car.c $(CAR_DIR)/car_read.c $(CAR_DIR)/car_write.c $(CAR_DIR)/car_dir.c
TEST_SRCS = $(TEST_DIR)/test_tokenizer.c $(TEST_DIR)/test_ast.c $(TEST_DIR)/test_parser.c $(TEST_DIR)/test_ast_dump.c $(TEST_DIR)/test_symbol_table.c $(TEST_DIR)/test_type_resolution.c $(TEST_DIR)/test_type_checker.c $(TEST_DIR)/test_semantic_dump.c $(TEST_DIR)/test_hir_dump.c $(TEST_DIR)/test_mir_dump.c $(TEST_DIR)/test_bytecode_dump.c $(TEST_DIR)/test_lir_dump.c $(TEST_DIR)/test_codegen_dump.c $(TEST_DIR)/test_machine_dump.c $(TEST_DIR)/test_runtime.c $(TEST_DIR)/test_asm_emit.c $(TEST_DIR)/test_build_native.c $(TEST_DIR)/test_calynda_cli.c

# Object files
TOKENIZER_OBJS       = $(BUILD_DIR)/tokenizer.o $(BUILD_DIR)/tokenizer_keywords.o $(BUILD_DIR)/tokenizer_literals.o $(BUILD_DIR)/tokenizer_punctuation.o
AST_OBJS             = $(BUILD_DIR)/ast.o $(BUILD_DIR)/ast_expressions.o $(BUILD_DIR)/ast_statements.o $(BUILD_DIR)/ast_declarations.o
PARSER_OBJS          = $(BUILD_DIR)/parser.o $(BUILD_DIR)/parser_utils.o $(BUILD_DIR)/parser_types.o $(BUILD_DIR)/parser_decl.o $(BUILD_DIR)/parser_union.o $(BUILD_DIR)/parser_stmt.o $(BUILD_DIR)/parser_expr.o $(BUILD_DIR)/parser_binary.o $(BUILD_DIR)/parser_postfix.o $(BUILD_DIR)/parser_literals.o $(BUILD_DIR)/parser_lookahead.o
AST_DUMP_OBJS        = $(BUILD_DIR)/ast_dump.o $(BUILD_DIR)/ast_dump_names.o $(BUILD_DIR)/ast_dump_types.o $(BUILD_DIR)/ast_dump_expr.o $(BUILD_DIR)/ast_dump_stmt.o $(BUILD_DIR)/ast_dump_decl.o
SYMBOL_TABLE_OBJS    = $(BUILD_DIR)/symbol_table.o $(BUILD_DIR)/symbol_table_query.o $(BUILD_DIR)/symbol_table_core.o $(BUILD_DIR)/symbol_table_registry.o $(BUILD_DIR)/symbol_table_imports.o $(BUILD_DIR)/symbol_table_predecl.o $(BUILD_DIR)/symbol_table_analyze.o $(BUILD_DIR)/symbol_table_analyze_expr.o
TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/type_resolution.o $(BUILD_DIR)/type_resolution_helpers.o $(BUILD_DIR)/type_resolution_resolve.o $(BUILD_DIR)/type_resolution_expr.o
TYPE_CHECKER_OBJS    = $(BUILD_DIR)/type_checker.o $(BUILD_DIR)/type_checker_check.o $(BUILD_DIR)/type_checker_names.o $(BUILD_DIR)/type_checker_types.o $(BUILD_DIR)/type_checker_convert.o $(BUILD_DIR)/type_checker_util.o $(BUILD_DIR)/type_checker_resolve.o $(BUILD_DIR)/type_checker_lambda.o $(BUILD_DIR)/type_checker_block.o $(BUILD_DIR)/type_checker_ops.o $(BUILD_DIR)/type_checker_expr.o $(BUILD_DIR)/type_checker_expr_ext.o $(BUILD_DIR)/type_checker_expr_more.o
SEMANTIC_DUMP_OBJS   = $(BUILD_DIR)/semantic_dump.o $(BUILD_DIR)/semantic_dump_builder.o $(BUILD_DIR)/semantic_dump_types.o $(BUILD_DIR)/semantic_dump_scope.o $(BUILD_DIR)/semantic_dump_symbols.o
HIR_OBJS             = $(BUILD_DIR)/hir.o $(BUILD_DIR)/hir_memory.o $(BUILD_DIR)/hir_helpers.o $(BUILD_DIR)/hir_lower.o $(BUILD_DIR)/hir_lower_stmt.o $(BUILD_DIR)/hir_lower_expr.o $(BUILD_DIR)/hir_lower_expr_ext.o $(BUILD_DIR)/hir_lower_decls.o
HIR_DUMP_OBJS        = $(BUILD_DIR)/hir_dump.o $(BUILD_DIR)/hir_dump_helpers.o $(BUILD_DIR)/hir_dump_expr.o $(BUILD_DIR)/hir_dump_expr_ext.o
MIR_OBJS             = $(BUILD_DIR)/mir.o $(BUILD_DIR)/mir_union.o $(BUILD_DIR)/mir_capture.o $(BUILD_DIR)/mir_capture_analysis.o $(BUILD_DIR)/mir_value.o $(BUILD_DIR)/mir_cleanup.o $(BUILD_DIR)/mir_builders.o $(BUILD_DIR)/mir_store.o $(BUILD_DIR)/mir_lvalue.o $(BUILD_DIR)/mir_control.o $(BUILD_DIR)/mir_assign.o $(BUILD_DIR)/mir_expr_helpers.o $(BUILD_DIR)/mir_expr.o $(BUILD_DIR)/mir_lower.o $(BUILD_DIR)/mir_lambda.o
MIR_DUMP_OBJS        = $(BUILD_DIR)/mir_dump.o $(BUILD_DIR)/mir_dump_helpers.o $(BUILD_DIR)/mir_dump_instr.o
BYTECODE_OBJS        = $(BUILD_DIR)/bytecode.o $(BUILD_DIR)/bytecode_memory.o $(BUILD_DIR)/bytecode_helpers.o $(BUILD_DIR)/bytecode_constants.o $(BUILD_DIR)/bytecode_lower_instr.o $(BUILD_DIR)/bytecode_lower.o
BYTECODE_DUMP_OBJS   = $(BUILD_DIR)/bytecode_dump.o $(BUILD_DIR)/bytecode_dump_helpers.o $(BUILD_DIR)/bytecode_dump_instr.o $(BUILD_DIR)/bytecode_dump_names.o
LIR_OBJS             = $(BUILD_DIR)/lir.o $(BUILD_DIR)/lir_memory.o $(BUILD_DIR)/lir_helpers.o $(BUILD_DIR)/lir_lower_instr.o $(BUILD_DIR)/lir_lower_instr_ext.o $(BUILD_DIR)/lir_lower_instr_stores.o $(BUILD_DIR)/lir_lower.o
LIR_DUMP_OBJS        = $(BUILD_DIR)/lir_dump.o $(BUILD_DIR)/lir_dump_helpers.o $(BUILD_DIR)/lir_dump_instr.o
CODEGEN_OBJS         = $(BUILD_DIR)/codegen.o $(BUILD_DIR)/codegen_names.o $(BUILD_DIR)/codegen_helpers.o $(BUILD_DIR)/codegen_infer.o $(BUILD_DIR)/codegen_build.o
CODEGEN_DUMP_OBJS    = $(BUILD_DIR)/codegen_dump.o $(BUILD_DIR)/codegen_dump_helpers.o
TARGET_OBJS          = $(BUILD_DIR)/target.o $(BUILD_DIR)/target_x86_64.o $(BUILD_DIR)/target_aarch64.o
RUNTIME_ABI_OBJS     = $(BUILD_DIR)/runtime_abi.o $(BUILD_DIR)/runtime_abi_names.o $(BUILD_DIR)/runtime_abi_dump.o
RUNTIME_OBJS         = $(BUILD_DIR)/runtime.o $(BUILD_DIR)/runtime_names.o $(BUILD_DIR)/runtime_format.o $(BUILD_DIR)/runtime_ops.o $(BUILD_DIR)/runtime_union.o
MACHINE_OBJS         = $(BUILD_DIR)/machine.o $(BUILD_DIR)/machine_dump.o $(BUILD_DIR)/machine_helpers.o $(BUILD_DIR)/machine_layout.o $(BUILD_DIR)/machine_operand.o $(BUILD_DIR)/machine_emit.o $(BUILD_DIR)/machine_ops.o $(BUILD_DIR)/machine_instr.o $(BUILD_DIR)/machine_instr_rt.o $(BUILD_DIR)/machine_instr_rt_ext.o $(BUILD_DIR)/machine_build.o
ASM_EMIT_OBJS        = $(BUILD_DIR)/asm_emit.o $(BUILD_DIR)/asm_emit_helpers.o $(BUILD_DIR)/asm_emit_symbols.o $(BUILD_DIR)/asm_emit_operand.o $(BUILD_DIR)/asm_emit_operand_ext.o $(BUILD_DIR)/asm_emit_instr.o $(BUILD_DIR)/asm_emit_instr_aarch64.o $(BUILD_DIR)/asm_emit_sections.o $(BUILD_DIR)/asm_emit_entry.o $(BUILD_DIR)/asm_emit_entry_aarch64.o
DUMP_AST_OBJS        = $(BUILD_DIR)/dump_ast.o
DUMP_SEMANTICS_OBJS  = $(BUILD_DIR)/dump_semantics.o
EMIT_ASM_OBJS        = $(BUILD_DIR)/emit_asm.o
EMIT_BYTECODE_OBJS   = $(BUILD_DIR)/emit_bytecode.o
BUILD_NATIVE_OBJS    = $(BUILD_DIR)/build_native.o $(BUILD_DIR)/build_native_utils.o
CALYNDA_CLI_OBJS     = $(BUILD_DIR)/calynda.o $(BUILD_DIR)/calynda_compile.o $(BUILD_DIR)/calynda_car.o $(BUILD_DIR)/calynda_utils.o
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
CAR_OBJS             = $(BUILD_DIR)/car.o $(BUILD_DIR)/car_read.o $(BUILD_DIR)/car_write.o $(BUILD_DIR)/car_dir.o
TEST_CAR_OBJS        = $(BUILD_DIR)/test_car.o

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
TEST_CAR_BIN       = $(BUILD_DIR)/test_car
TEST_BINS          = $(TEST_TOKENIZER_BIN) $(TEST_AST_BIN) $(TEST_PARSER_BIN) $(TEST_AST_DUMP_BIN) $(TEST_SYMBOL_TABLE_BIN) $(TEST_TYPE_RESOLUTION_BIN) $(TEST_TYPE_CHECKER_BIN) $(TEST_SEMANTIC_DUMP_BIN) $(TEST_HIR_DUMP_BIN) $(TEST_MIR_DUMP_BIN) $(TEST_BYTECODE_DUMP_BIN) $(TEST_LIR_DUMP_BIN) $(TEST_CODEGEN_DUMP_BIN) $(TEST_MACHINE_DUMP_BIN) $(TEST_RUNTIME_BIN) $(TEST_ASM_EMIT_BIN) $(TEST_BUILD_NATIVE_BIN) $(TEST_CALYNDA_CLI_BIN) $(TEST_CAR_BIN)

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

$(BUILD_DIR)/tokenizer.o: $(TOKENIZER_DIR)/tokenizer.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_keywords.o: $(TOKENIZER_DIR)/tokenizer_keywords.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_literals.o: $(TOKENIZER_DIR)/tokenizer_literals.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_punctuation.o: $(TOKENIZER_DIR)/tokenizer_punctuation.c $(TOKENIZER_DIR)/tokenizer_internal.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(AST_DIR)/ast.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_expressions.o: $(AST_DIR)/ast_expressions.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_statements.o: $(AST_DIR)/ast_statements.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_declarations.o: $(AST_DIR)/ast_declarations.c $(AST_DIR)/ast_internal.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: $(PARSER_DIR)/parser.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_utils.o: $(PARSER_DIR)/parser_utils.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_types.o: $(PARSER_DIR)/parser_types.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_decl.o: $(PARSER_DIR)/parser_decl.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_union.o: $(PARSER_DIR)/parser_union.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_stmt.o: $(PARSER_DIR)/parser_stmt.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_expr.o: $(PARSER_DIR)/parser_expr.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_binary.o: $(PARSER_DIR)/parser_binary.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_postfix.o: $(PARSER_DIR)/parser_postfix.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_literals.o: $(PARSER_DIR)/parser_literals.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_lookahead.o: $(PARSER_DIR)/parser_lookahead.c $(PARSER_DIR)/parser_internal.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump.o: $(AST_DIR)/ast_dump.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_names.o: $(AST_DIR)/ast_dump_names.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_types.o: $(AST_DIR)/ast_dump_types.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_expr.o: $(AST_DIR)/ast_dump_expr.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_stmt.o: $(AST_DIR)/ast_dump_stmt.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_decl.o: $(AST_DIR)/ast_dump_decl.c $(AST_DIR)/ast_dump_internal.h $(AST_DIR)/ast_dump.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table.o: $(SEMA_DIR)/symbol_table.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_query.o: $(SEMA_DIR)/symbol_table_query.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_core.o: $(SEMA_DIR)/symbol_table_core.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_registry.o: $(SEMA_DIR)/symbol_table_registry.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_imports.o: $(SEMA_DIR)/symbol_table_imports.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_predecl.o: $(SEMA_DIR)/symbol_table_predecl.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_analyze.o: $(SEMA_DIR)/symbol_table_analyze.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_analyze_expr.o: $(SEMA_DIR)/symbol_table_analyze_expr.c $(SEMA_DIR)/symbol_table_internal.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution.o: $(SEMA_DIR)/type_resolution.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_helpers.o: $(SEMA_DIR)/type_resolution_helpers.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_resolve.o: $(SEMA_DIR)/type_resolution_resolve.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_expr.o: $(SEMA_DIR)/type_resolution_expr.c $(SEMA_DIR)/type_resolution_internal.h $(SEMA_DIR)/type_resolution.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker.o: $(SEMA_DIR)/type_checker.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_check.o: $(SEMA_DIR)/type_checker_check.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_names.o: $(SEMA_DIR)/type_checker_names.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_types.o: $(SEMA_DIR)/type_checker_types.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_convert.o: $(SEMA_DIR)/type_checker_convert.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_util.o: $(SEMA_DIR)/type_checker_util.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_resolve.o: $(SEMA_DIR)/type_checker_resolve.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_lambda.o: $(SEMA_DIR)/type_checker_lambda.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_block.o: $(SEMA_DIR)/type_checker_block.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_ops.o: $(SEMA_DIR)/type_checker_ops.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr.o: $(SEMA_DIR)/type_checker_expr.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_ext.o: $(SEMA_DIR)/type_checker_expr_ext.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_more.o: $(SEMA_DIR)/type_checker_expr_more.c $(SEMA_DIR)/type_checker_internal.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump.o: $(SEMA_DIR)/semantic_dump.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_builder.o: $(SEMA_DIR)/semantic_dump_builder.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_types.o: $(SEMA_DIR)/semantic_dump_types.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_scope.o: $(SEMA_DIR)/semantic_dump_scope.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_symbols.o: $(SEMA_DIR)/semantic_dump_symbols.c $(SEMA_DIR)/semantic_dump_internal.h $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir.o: $(HIR_DIR)/hir.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_memory.o: $(HIR_DIR)/hir_memory.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_helpers.o: $(HIR_DIR)/hir_helpers.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower.o: $(HIR_DIR)/hir_lower.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_stmt.o: $(HIR_DIR)/hir_lower_stmt.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_expr.o: $(HIR_DIR)/hir_lower_expr.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_expr_ext.o: $(HIR_DIR)/hir_lower_expr_ext.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_decls.o: $(HIR_DIR)/hir_lower_decls.c $(HIR_DIR)/hir_internal.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump.o: $(HIR_DIR)/hir_dump.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_helpers.o: $(HIR_DIR)/hir_dump_helpers.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_expr.o: $(HIR_DIR)/hir_dump_expr.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_expr_ext.o: $(HIR_DIR)/hir_dump_expr_ext.c $(HIR_DIR)/hir_dump_internal.h $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir.o: $(MIR_DIR)/mir.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_union.o: $(MIR_DIR)/mir_union.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_capture.o: $(MIR_DIR)/mir_capture.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_capture_analysis.o: $(MIR_DIR)/mir_capture_analysis.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_value.o: $(MIR_DIR)/mir_value.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_cleanup.o: $(MIR_DIR)/mir_cleanup.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_builders.o: $(MIR_DIR)/mir_builders.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_store.o: $(MIR_DIR)/mir_store.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_lvalue.o: $(MIR_DIR)/mir_lvalue.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_control.o: $(MIR_DIR)/mir_control.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_assign.o: $(MIR_DIR)/mir_assign.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_expr_helpers.o: $(MIR_DIR)/mir_expr_helpers.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_expr.o: $(MIR_DIR)/mir_expr.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_lower.o: $(MIR_DIR)/mir_lower.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_lambda.o: $(MIR_DIR)/mir_lambda.c $(MIR_DIR)/mir_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_dump.o: $(MIR_DIR)/mir_dump.c $(MIR_DIR)/mir_dump_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_dump_helpers.o: $(MIR_DIR)/mir_dump_helpers.c $(MIR_DIR)/mir_dump_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_dump_instr.o: $(MIR_DIR)/mir_dump_instr.c $(MIR_DIR)/mir_dump_internal.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode.o: $(BYTECODE_DIR)/bytecode.c $(BYTECODE_DIR)/bytecode_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_memory.o: $(BYTECODE_DIR)/bytecode_memory.c $(BYTECODE_DIR)/bytecode_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_helpers.o: $(BYTECODE_DIR)/bytecode_helpers.c $(BYTECODE_DIR)/bytecode_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_constants.o: $(BYTECODE_DIR)/bytecode_constants.c $(BYTECODE_DIR)/bytecode_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_lower_instr.o: $(BYTECODE_DIR)/bytecode_lower_instr.c $(BYTECODE_DIR)/bytecode_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_lower.o: $(BYTECODE_DIR)/bytecode_lower.c $(BYTECODE_DIR)/bytecode_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump.o: $(BYTECODE_DIR)/bytecode_dump.c $(BYTECODE_DIR)/bytecode_dump_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump_helpers.o: $(BYTECODE_DIR)/bytecode_dump_helpers.c $(BYTECODE_DIR)/bytecode_dump_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump_instr.o: $(BYTECODE_DIR)/bytecode_dump_instr.c $(BYTECODE_DIR)/bytecode_dump_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump_names.o: $(BYTECODE_DIR)/bytecode_dump_names.c $(BYTECODE_DIR)/bytecode_dump_internal.h $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir.o: $(LIR_DIR)/lir.c $(LIR_DIR)/lir_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_memory.o: $(LIR_DIR)/lir_memory.c $(LIR_DIR)/lir_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_helpers.o: $(LIR_DIR)/lir_helpers.c $(LIR_DIR)/lir_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower_instr.o: $(LIR_DIR)/lir_lower_instr.c $(LIR_DIR)/lir_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower_instr_ext.o: $(LIR_DIR)/lir_lower_instr_ext.c $(LIR_DIR)/lir_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower_instr_stores.o: $(LIR_DIR)/lir_lower_instr_stores.c $(LIR_DIR)/lir_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower.o: $(LIR_DIR)/lir_lower.c $(LIR_DIR)/lir_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_dump.o: $(LIR_DIR)/lir_dump.c $(LIR_DIR)/lir_dump_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_dump_helpers.o: $(LIR_DIR)/lir_dump_helpers.c $(LIR_DIR)/lir_dump_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_dump_instr.o: $(LIR_DIR)/lir_dump_instr.c $(LIR_DIR)/lir_dump_internal.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen.o: $(CODEGEN_DIR)/codegen.c $(CODEGEN_DIR)/codegen_internal.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_names.o: $(CODEGEN_DIR)/codegen_names.c $(CODEGEN_DIR)/codegen_internal.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_helpers.o: $(CODEGEN_DIR)/codegen_helpers.c $(CODEGEN_DIR)/codegen_internal.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_infer.o: $(CODEGEN_DIR)/codegen_infer.c $(CODEGEN_DIR)/codegen_internal.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_build.o: $(CODEGEN_DIR)/codegen_build.c $(CODEGEN_DIR)/codegen_internal.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_dump.o: $(CODEGEN_DIR)/codegen_dump.c $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_dump_helpers.o: $(CODEGEN_DIR)/codegen_dump_helpers.c $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/target.o: $(BACKEND_DIR)/target.c $(BACKEND_DIR)/target.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/target_x86_64.o: $(BACKEND_DIR)/target_x86_64.c $(BACKEND_DIR)/target.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/target_aarch64.o: $(BACKEND_DIR)/target_aarch64.c $(BACKEND_DIR)/target.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_abi.o: $(BACKEND_DIR)/runtime_abi.c $(BACKEND_DIR)/runtime_abi.h $(BACKEND_DIR)/target.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_abi_dump.o: $(BACKEND_DIR)/runtime_abi_dump.c $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_abi_names.o: $(BACKEND_DIR)/runtime_abi_names.c $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime.o: $(RUNTIME_DIR)/runtime.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_names.o: $(RUNTIME_DIR)/runtime_names.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_format.o: $(RUNTIME_DIR)/runtime_format.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_ops.o: $(RUNTIME_DIR)/runtime_ops.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_union.o: $(RUNTIME_DIR)/runtime_union.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine.o: $(BACKEND_DIR)/machine.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_dump.o: $(BACKEND_DIR)/machine_dump.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_helpers.o: $(BACKEND_DIR)/machine_helpers.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_layout.o: $(BACKEND_DIR)/machine_layout.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_operand.o: $(BACKEND_DIR)/machine_operand.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(CODEGEN_DIR)/codegen.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_emit.o: $(BACKEND_DIR)/machine_emit.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_ops.o: $(BACKEND_DIR)/machine_ops.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_instr.o: $(BACKEND_DIR)/machine_instr.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_instr_rt.o: $(BACKEND_DIR)/machine_instr_rt.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_instr_rt_ext.o: $(BACKEND_DIR)/machine_instr_rt_ext.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_build.o: $(BACKEND_DIR)/machine_build.c $(BACKEND_DIR)/machine_internal.h $(BACKEND_DIR)/machine.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit.o: $(BACKEND_DIR)/asm_emit.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(RUNTIME_DIR)/runtime.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/symbol_table.h $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_helpers.o: $(BACKEND_DIR)/asm_emit_helpers.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_symbols.o: $(BACKEND_DIR)/asm_emit_symbols.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand.o: $(BACKEND_DIR)/asm_emit_operand.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand_ext.o: $(BACKEND_DIR)/asm_emit_operand_ext.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_instr.o: $(BACKEND_DIR)/asm_emit_instr.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_sections.o: $(BACKEND_DIR)/asm_emit_sections.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_entry.o: $(BACKEND_DIR)/asm_emit_entry.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_instr_aarch64.o: $(BACKEND_DIR)/asm_emit_instr_aarch64.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_entry_aarch64.o: $(BACKEND_DIR)/asm_emit_entry_aarch64.c $(BACKEND_DIR)/asm_emit_internal.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_ast.o: $(CLI_DIR)/dump_ast.c $(AST_DIR)/ast_dump.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_semantics.o: $(CLI_DIR)/dump_semantics.c $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/emit_asm.o: $(CLI_DIR)/emit_asm.c $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/emit_bytecode.o: $(CLI_DIR)/emit_bytecode.c $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/build_native.o: $(CLI_DIR)/build_native.c $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/build_native_utils.o: $(CLI_DIR)/build_native_utils.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda.o: $(CLI_DIR)/calynda.c $(CLI_DIR)/calynda_internal.h $(CAR_DIR)/car.h $(BACKEND_DIR)/asm_emit.h $(BYTECODE_DIR)/bytecode.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_compile.o: $(CLI_DIR)/calynda_compile.c $(CLI_DIR)/calynda_internal.h $(BACKEND_DIR)/asm_emit.h $(BYTECODE_DIR)/bytecode.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_car.o: $(CLI_DIR)/calynda_car.c $(CLI_DIR)/calynda_internal.h $(CAR_DIR)/car.h $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_utils.o: $(CLI_DIR)/calynda_utils.c $(CLI_DIR)/calynda_internal.h $(BACKEND_DIR)/asm_emit.h $(BYTECODE_DIR)/bytecode.h $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car.o: $(CAR_DIR)/car.c $(CAR_DIR)/car.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car_read.o: $(CAR_DIR)/car_read.c $(CAR_DIR)/car.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car_write.o: $(CAR_DIR)/car_write.c $(CAR_DIR)/car.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car_dir.o: $(CAR_DIR)/car_dir.c $(CAR_DIR)/car.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer.o: $(TEST_DIR)/test_tokenizer.c $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/test_ast.o: $(TEST_DIR)/test_ast.c $(AST_DIR)/ast.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser.o: $(TEST_DIR)/test_parser.c $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_dump.o: $(TEST_DIR)/test_ast_dump.c $(AST_DIR)/ast_dump.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table.o: $(TEST_DIR)/test_symbol_table.c $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_resolution.o: $(TEST_DIR)/test_type_resolution.c $(SEMA_DIR)/type_resolution.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker.o: $(TEST_DIR)/test_type_checker.c $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_semantic_dump.o: $(TEST_DIR)/test_semantic_dump.c $(SEMA_DIR)/semantic_dump.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_hir_dump.o: $(TEST_DIR)/test_hir_dump.c $(HIR_DIR)/hir_dump.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump.o: $(TEST_DIR)/test_mir_dump.c $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_bytecode_dump.o: $(TEST_DIR)/test_bytecode_dump.c $(BYTECODE_DIR)/bytecode.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_lir_dump.o: $(TEST_DIR)/test_lir_dump.c $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_codegen_dump.o: $(TEST_DIR)/test_codegen_dump.c $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_machine_dump.o: $(TEST_DIR)/test_machine_dump.c $(BACKEND_DIR)/machine.h $(BACKEND_DIR)/runtime_abi.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_runtime.o: $(TEST_DIR)/test_runtime.c $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_asm_emit.o: $(TEST_DIR)/test_asm_emit.c $(BACKEND_DIR)/asm_emit.h $(BACKEND_DIR)/machine.h $(RUNTIME_DIR)/runtime.h $(CODEGEN_DIR)/codegen.h $(LIR_DIR)/lir.h $(MIR_DIR)/mir.h $(HIR_DIR)/hir.h $(SEMA_DIR)/type_checker.h $(SEMA_DIR)/type_resolution.h $(SEMA_DIR)/symbol_table.h $(PARSER_DIR)/parser.h $(AST_DIR)/ast.h $(TOKENIZER_DIR)/tokenizer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native.o: $(TEST_DIR)/test_build_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_calynda_cli.o: $(TEST_DIR)/test_calynda_cli.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_car.o: $(TEST_DIR)/test_car.c $(CAR_DIR)/car.h | $(BUILD_DIR)
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

$(TEST_CODEGEN_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(TEST_CODEGEN_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_MACHINE_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(MACHINE_OBJS) $(TEST_MACHINE_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_RUNTIME_BIN): $(RUNTIME_OBJS) $(TEST_RUNTIME_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_ASM_EMIT_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(TEST_ASM_EMIT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_BUILD_NATIVE_BIN): $(TEST_BUILD_NATIVE_OBJS) $(BUILD_NATIVE_BIN)
	$(CC) $(CFLAGS) $(TEST_BUILD_NATIVE_OBJS) -o $@

$(TEST_CALYNDA_CLI_BIN): $(TEST_CALYNDA_CLI_OBJS) $(CALYNDA_BIN)
	$(CC) $(CFLAGS) $(TEST_CALYNDA_CLI_OBJS) -o $@

$(TEST_CAR_BIN): $(CAR_OBJS) $(TEST_CAR_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(DUMP_AST_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(AST_DUMP_OBJS) $(DUMP_AST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(DUMP_SEMANTICS_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(SEMANTIC_DUMP_OBJS) $(DUMP_SEMANTICS_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(EMIT_ASM_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(EMIT_ASM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(EMIT_BYTECODE_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(EMIT_BYTECODE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/calynda_runtime.a: $(RUNTIME_OBJS)
	ar rcs $@ $^

$(BUILD_NATIVE_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(RUNTIME_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(CAR_OBJS) $(BUILD_NATIVE_OBJS) $(BUILD_DIR)/calynda_runtime.a
	$(CC) $(CFLAGS) $^ -o $@

$(CALYNDA_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(RUNTIME_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(CAR_OBJS) $(CALYNDA_CLI_OBJS) $(BUILD_DIR)/calynda_runtime.a
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
	./$(TEST_CAR_BIN)

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

# --- ARM64 cross-compiled runtime library ---
CROSS_CC_AARCH64 = aarch64-linux-gnu-gcc
CROSS_CFLAGS_AARCH64 = -Wall -Wextra -pedantic -std=c11 -g -I$(RUNTIME_DIR)

RUNTIME_AARCH64_OBJS = $(BUILD_DIR)/runtime_aarch64.o \
                       $(BUILD_DIR)/runtime_names_aarch64.o \
                       $(BUILD_DIR)/runtime_format_aarch64.o \
                       $(BUILD_DIR)/runtime_ops_aarch64.o \
                       $(BUILD_DIR)/runtime_union_aarch64.o

$(BUILD_DIR)/runtime_aarch64.o: $(RUNTIME_DIR)/runtime.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_AARCH64) $(CROSS_CFLAGS_AARCH64) -c $< -o $@

$(BUILD_DIR)/runtime_names_aarch64.o: $(RUNTIME_DIR)/runtime_names.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_AARCH64) $(CROSS_CFLAGS_AARCH64) -c $< -o $@

$(BUILD_DIR)/runtime_format_aarch64.o: $(RUNTIME_DIR)/runtime_format.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_AARCH64) $(CROSS_CFLAGS_AARCH64) -c $< -o $@

$(BUILD_DIR)/runtime_ops_aarch64.o: $(RUNTIME_DIR)/runtime_ops.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_AARCH64) $(CROSS_CFLAGS_AARCH64) -c $< -o $@

$(BUILD_DIR)/runtime_union_aarch64.o: $(RUNTIME_DIR)/runtime_union.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_AARCH64) $(CROSS_CFLAGS_AARCH64) -c $< -o $@

$(BUILD_DIR)/calynda_runtime_aarch64.a: $(RUNTIME_AARCH64_OBJS)
	ar rcs $@ $^

runtime-aarch64: $(BUILD_DIR)/calynda_runtime_aarch64.a
