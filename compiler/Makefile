CC      = gcc

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

# Source subdirectory layout
AST_DIR                  = src/ast
AST_DUMP_DIR             = src/ast/dump
ASM_EMIT_DIR             = src/backend/asm_emit
MACHINE_DIR              = src/backend/machine
RUNTIME_ABI_DIR          = src/backend/runtime_abi
TARGET_DIR               = src/backend/target
BYTECODE_DIR             = src/bytecode
CAR_DIR                  = src/car
CLI_DIR                  = src/cli
CODEGEN_DIR              = src/codegen
HIR_DIR                  = src/hir
HIR_DUMP_DIR             = src/hir/dump
HIR_LOWER_DIR            = src/hir/lower
LIR_DIR                  = src/lir
MIR_DIR                  = src/mir
MIR_DUMP_DIR             = src/mir/dump
MIR_LOWER_DIR            = src/mir/lower
PARSER_DIR               = src/parser
RUNTIME_DIR              = src/runtime
SEMANTIC_DUMP_DIR        = src/sema/semantic_dump
SYMBOL_TABLE_DIR         = src/sema/symbol_table
TYPE_CHECKER_DIR         = src/sema/type_checker
TYPE_CHECKER_EXPR_DIR    = src/sema/type_checker/expr
TYPE_RESOLUTION_DIR      = src/sema/type_resolution
TOKENIZER_DIR            = src/tokenizer

# Test subdirectory layout
TEST_BACKEND_CODEGEN_DIR = tests/backend/codegen
TEST_BACKEND_EMIT_DIR    = tests/backend/emit
TEST_BACKEND_TOOLS_DIR   = tests/backend/tools
TEST_FRONTEND_AST_DIR    = tests/frontend/ast
TEST_FRONTEND_PARSER_DIR = tests/frontend/parser
TEST_FRONTEND_TOK_DIR    = tests/frontend/tokenizer
TEST_IR_DIR              = tests/ir
TEST_SEMA_CHECKER_DIR    = tests/sema/checker
TEST_SEMA_RESOLUTION_DIR = tests/sema/resolution
TEST_SEMA_SYMBOLS_DIR    = tests/sema/symbols

INCLUDES = -I$(AST_DIR) \
           -I$(AST_DUMP_DIR) \
           -I$(ASM_EMIT_DIR) \
           -I$(MACHINE_DIR) \
           -I$(RUNTIME_ABI_DIR) \
           -I$(TARGET_DIR) \
           -I$(BYTECODE_DIR) \
           -I$(CAR_DIR) \
           -I$(CLI_DIR) \
           -I$(CODEGEN_DIR) \
           -I$(HIR_DIR) \
           -I$(HIR_DUMP_DIR) \
           -I$(LIR_DIR) \
           -I$(MIR_DIR) \
           -I$(MIR_DUMP_DIR) \
           -I$(PARSER_DIR) \
           -I$(RUNTIME_DIR) \
           -I$(SEMANTIC_DUMP_DIR) \
           -I$(SYMBOL_TABLE_DIR) \
           -I$(TYPE_CHECKER_DIR) \
           -I$(TYPE_RESOLUTION_DIR) \
           -I$(TOKENIZER_DIR)

CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g $(INCLUDES)
PTHREAD_FLAGS = -pthread

# Source files
SRCS = $(AST_DIR)/ast.c $(AST_DIR)/ast_declarations.c $(AST_DIR)/ast_expressions.c $(AST_DIR)/ast_statements.c $(AST_DUMP_DIR)/ast_dump.c $(AST_DUMP_DIR)/ast_dump_decl.c $(AST_DUMP_DIR)/ast_dump_expr.c $(AST_DUMP_DIR)/ast_dump_names.c $(AST_DUMP_DIR)/ast_dump_stmt.c $(AST_DUMP_DIR)/ast_dump_types.c $(ASM_EMIT_DIR)/asm_emit.c $(ASM_EMIT_DIR)/asm_emit_entry.c $(ASM_EMIT_DIR)/aarch64/asm_emit_entry_aarch64.c $(ASM_EMIT_DIR)/riscv64/asm_emit_entry_riscv64.c $(ASM_EMIT_DIR)/asm_emit_helpers.c $(ASM_EMIT_DIR)/aarch64/asm_emit_helpers_aarch64.c $(ASM_EMIT_DIR)/riscv64/asm_emit_helpers_riscv64.c $(ASM_EMIT_DIR)/asm_emit_instr.c $(ASM_EMIT_DIR)/aarch64/asm_emit_instr_aarch64.c $(ASM_EMIT_DIR)/riscv64/asm_emit_instr_riscv64.c $(ASM_EMIT_DIR)/asm_emit_operand.c $(ASM_EMIT_DIR)/aarch64/asm_emit_operand_aarch64.c $(ASM_EMIT_DIR)/riscv64/asm_emit_operand_riscv64.c $(ASM_EMIT_DIR)/asm_emit_operand_ext.c $(ASM_EMIT_DIR)/aarch64/asm_emit_operand_ext_aarch64.c $(ASM_EMIT_DIR)/riscv64/asm_emit_operand_ext_riscv64.c $(ASM_EMIT_DIR)/asm_emit_sections.c $(ASM_EMIT_DIR)/asm_emit_symbols.c $(ASM_EMIT_DIR)/asm_emit_type_desc.c $(MACHINE_DIR)/machine.c $(MACHINE_DIR)/machine_build.c $(MACHINE_DIR)/machine_dump.c $(MACHINE_DIR)/machine_emit.c $(MACHINE_DIR)/machine_helpers.c $(MACHINE_DIR)/machine_instr.c $(MACHINE_DIR)/machine_instr_rt.c $(MACHINE_DIR)/machine_instr_rt_ext.c $(MACHINE_DIR)/machine_layout.c $(MACHINE_DIR)/machine_operand.c $(MACHINE_DIR)/machine_operand_preserve.c $(MACHINE_DIR)/machine_ops.c $(MACHINE_DIR)/machine_ops_unary.c $(RUNTIME_ABI_DIR)/runtime_abi.c $(RUNTIME_ABI_DIR)/runtime_abi_dump.c $(RUNTIME_ABI_DIR)/runtime_abi_names.c $(TARGET_DIR)/target.c $(TARGET_DIR)/target_aarch64.c $(TARGET_DIR)/target_riscv64.c $(TARGET_DIR)/target_x86_64.c $(BYTECODE_DIR)/bytecode.c $(BYTECODE_DIR)/bytecode_constants.c $(BYTECODE_DIR)/bytecode_dump.c $(BYTECODE_DIR)/bytecode_dump_helpers.c $(BYTECODE_DIR)/bytecode_dump_instr.c $(BYTECODE_DIR)/bytecode_dump_names.c $(BYTECODE_DIR)/bytecode_helpers.c $(BYTECODE_DIR)/bytecode_lower.c $(BYTECODE_DIR)/bytecode_lower_instr.c $(BYTECODE_DIR)/bytecode_memory.c $(CAR_DIR)/car.c $(CAR_DIR)/car_dir.c $(CAR_DIR)/car_read.c $(CAR_DIR)/car_write.c $(CLI_DIR)/build_native.c $(CLI_DIR)/build_native_utils.c $(CLI_DIR)/calynda.c $(CLI_DIR)/calynda_car.c $(CLI_DIR)/calynda_car_helpers.c $(CLI_DIR)/calynda_commands.c $(CLI_DIR)/calynda_compile.c $(CLI_DIR)/calynda_utils.c $(CLI_DIR)/dump_ast.c $(CLI_DIR)/dump_semantics.c $(CLI_DIR)/emit_asm.c $(CLI_DIR)/emit_bytecode.c $(CODEGEN_DIR)/codegen.c $(CODEGEN_DIR)/codegen_build.c $(CODEGEN_DIR)/codegen_dump.c $(CODEGEN_DIR)/codegen_dump_helpers.c $(CODEGEN_DIR)/codegen_helpers.c $(CODEGEN_DIR)/codegen_infer.c $(CODEGEN_DIR)/codegen_names.c $(HIR_DUMP_DIR)/hir_dump.c $(HIR_DUMP_DIR)/hir_dump_expr.c $(HIR_DUMP_DIR)/hir_dump_expr_ext.c $(HIR_DUMP_DIR)/hir_dump_helpers.c $(HIR_DIR)/hir.c $(HIR_DIR)/hir_helpers.c $(HIR_DIR)/hir_memory.c $(HIR_DIR)/hir_memory_alloc.c $(HIR_LOWER_DIR)/hir_lower.c $(HIR_LOWER_DIR)/hir_lower_decls.c $(HIR_LOWER_DIR)/hir_lower_decls_bind.c $(HIR_LOWER_DIR)/hir_lower_expr.c $(HIR_LOWER_DIR)/hir_lower_expr_ext.c $(HIR_LOWER_DIR)/hir_lower_expr_memory.c $(HIR_LOWER_DIR)/hir_lower_stmt.c $(LIR_DIR)/lir.c $(LIR_DIR)/lir_dump.c $(LIR_DIR)/lir_dump_helpers.c $(LIR_DIR)/lir_dump_instr.c $(LIR_DIR)/lir_helpers.c $(LIR_DIR)/lir_lower.c $(LIR_DIR)/lir_lower_instr.c $(LIR_DIR)/lir_lower_instr_ext.c $(LIR_DIR)/lir_lower_instr_stores.c $(LIR_DIR)/lir_memory.c $(MIR_DUMP_DIR)/mir_dump.c $(MIR_DUMP_DIR)/mir_dump_helpers.c $(MIR_DUMP_DIR)/mir_dump_instr.c $(MIR_LOWER_DIR)/mir_assign.c $(MIR_LOWER_DIR)/mir_control.c $(MIR_LOWER_DIR)/mir_expr.c $(MIR_LOWER_DIR)/mir_expr_call.c $(MIR_LOWER_DIR)/mir_expr_helpers.c $(MIR_LOWER_DIR)/mir_expr_memory.c $(MIR_LOWER_DIR)/mir_lambda.c $(MIR_LOWER_DIR)/mir_lower.c $(MIR_LOWER_DIR)/mir_lvalue.c $(MIR_LOWER_DIR)/mir_manual_cleanup.c $(MIR_LOWER_DIR)/mir_start.c $(MIR_LOWER_DIR)/mir_store.c $(MIR_LOWER_DIR)/mir_union.c $(MIR_DIR)/mir.c $(MIR_DIR)/mir_build.c $(MIR_DIR)/mir_builders.c $(MIR_DIR)/mir_capture.c $(MIR_DIR)/mir_capture_analysis.c $(MIR_DIR)/mir_capture_analysis_expr.c $(MIR_DIR)/mir_cleanup.c $(MIR_DIR)/mir_value.c $(PARSER_DIR)/parser.c $(PARSER_DIR)/parser_binary.c $(PARSER_DIR)/parser_decl.c $(PARSER_DIR)/parser_decl_ext.c $(PARSER_DIR)/parser_expr.c $(PARSER_DIR)/parser_literals.c $(PARSER_DIR)/parser_lookahead.c $(PARSER_DIR)/parser_postfix.c $(PARSER_DIR)/parser_primary.c $(PARSER_DIR)/parser_stmt.c $(PARSER_DIR)/parser_types.c $(PARSER_DIR)/parser_layout.c $(PARSER_DIR)/parser_union.c $(PARSER_DIR)/parser_utils.c $(RUNTIME_DIR)/runtime.c $(RUNTIME_DIR)/runtime_bounds.c $(RUNTIME_DIR)/runtime_format.c $(RUNTIME_DIR)/runtime_manual.c $(RUNTIME_DIR)/runtime_names.c $(RUNTIME_DIR)/runtime_ops.c $(RUNTIME_DIR)/runtime_threads.c $(RUNTIME_DIR)/runtime_union.c $(SEMANTIC_DUMP_DIR)/semantic_dump.c $(SEMANTIC_DUMP_DIR)/semantic_dump_builder.c $(SEMANTIC_DUMP_DIR)/semantic_dump_scope.c $(SEMANTIC_DUMP_DIR)/semantic_dump_symbols.c $(SEMANTIC_DUMP_DIR)/semantic_dump_types.c $(SYMBOL_TABLE_DIR)/symbol_table.c $(SYMBOL_TABLE_DIR)/symbol_table_analyze.c $(SYMBOL_TABLE_DIR)/symbol_table_analyze_expr.c $(SYMBOL_TABLE_DIR)/symbol_table_core.c $(SYMBOL_TABLE_DIR)/symbol_table_imports.c $(SYMBOL_TABLE_DIR)/symbol_table_predecl.c $(SYMBOL_TABLE_DIR)/symbol_table_query.c $(SYMBOL_TABLE_DIR)/symbol_table_registry.c $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr.c $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_array.c $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_ext.c $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_memory.c $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_more.c $(TYPE_CHECKER_DIR)/type_checker.c $(TYPE_CHECKER_DIR)/type_checker_block.c $(TYPE_CHECKER_DIR)/type_checker_check.c $(TYPE_CHECKER_DIR)/type_checker_convert.c $(TYPE_CHECKER_DIR)/type_checker_lambda.c $(TYPE_CHECKER_DIR)/type_checker_names.c $(TYPE_CHECKER_DIR)/type_checker_ops.c $(TYPE_CHECKER_DIR)/type_checker_resolve.c $(TYPE_CHECKER_DIR)/type_checker_resolve_binding.c $(TYPE_CHECKER_DIR)/type_checker_types.c $(TYPE_CHECKER_DIR)/type_checker_util.c $(TYPE_RESOLUTION_DIR)/type_resolution.c $(TYPE_RESOLUTION_DIR)/type_resolution_expr.c $(TYPE_RESOLUTION_DIR)/type_resolution_helpers.c $(TYPE_RESOLUTION_DIR)/type_resolution_resolve.c $(TOKENIZER_DIR)/tokenizer.c $(TOKENIZER_DIR)/tokenizer_keywords.c $(TOKENIZER_DIR)/tokenizer_literals.c $(TOKENIZER_DIR)/tokenizer_punctuation.c $(TOKENIZER_DIR)/tokenizer_type_names.c
TEST_SRCS = $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump.c $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump_2.c $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump_3.c $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump_4.c $(TEST_BACKEND_CODEGEN_DIR)/test_machine_dump.c $(TEST_BACKEND_CODEGEN_DIR)/test_machine_dump_2.c $(TEST_BACKEND_CODEGEN_DIR)/test_machine_dump_3.c $(TEST_BACKEND_EMIT_DIR)/test_asm_emit.c $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_2.c $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_3.c $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_4.c $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_cross.c $(TEST_BACKEND_EMIT_DIR)/test_build_native.c $(TEST_BACKEND_EMIT_DIR)/test_build_native_2.c $(TEST_BACKEND_EMIT_DIR)/test_build_native_3.c $(TEST_BACKEND_EMIT_DIR)/test_build_native_4.c $(TEST_BACKEND_EMIT_DIR)/test_build_native_5.c $(TEST_BACKEND_EMIT_DIR)/test_build_native_6.c $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli.c $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli_2.c $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli_3.c $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli_4.c $(TEST_BACKEND_TOOLS_DIR)/test_car.c $(TEST_BACKEND_TOOLS_DIR)/test_car_2.c $(TEST_BACKEND_TOOLS_DIR)/test_car_3.c $(TEST_BACKEND_TOOLS_DIR)/test_car_4.c $(TEST_BACKEND_TOOLS_DIR)/test_runtime.c $(TEST_FRONTEND_AST_DIR)/test_ast.c $(TEST_FRONTEND_AST_DIR)/test_ast_2.c $(TEST_FRONTEND_AST_DIR)/test_ast_3.c $(TEST_FRONTEND_AST_DIR)/test_ast_4.c $(TEST_FRONTEND_AST_DIR)/test_ast_5.c $(TEST_FRONTEND_AST_DIR)/test_ast_dump.c $(TEST_FRONTEND_AST_DIR)/test_ast_dump_2.c $(TEST_FRONTEND_AST_DIR)/test_ast_dump_3.c $(TEST_FRONTEND_PARSER_DIR)/test_parser.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_2.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_3.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_4.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_5.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_6.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_7.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_8.c $(TEST_FRONTEND_PARSER_DIR)/test_parser_9.c $(TEST_FRONTEND_TOK_DIR)/test_tokenizer.c $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_2.c $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_3.c $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_4.c $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_5.c $(TEST_IR_DIR)/test_bytecode_dump.c $(TEST_IR_DIR)/test_bytecode_dump_2.c $(TEST_IR_DIR)/test_bytecode_dump_3.c $(TEST_IR_DIR)/test_hir_dump.c $(TEST_IR_DIR)/test_hir_dump_2.c $(TEST_IR_DIR)/test_hir_dump_3.c $(TEST_IR_DIR)/test_hir_dump_4.c $(TEST_IR_DIR)/test_lir_dump.c $(TEST_IR_DIR)/test_lir_dump_2.c $(TEST_IR_DIR)/test_lir_dump_3.c $(TEST_IR_DIR)/test_mir_dump.c $(TEST_IR_DIR)/test_mir_dump_2.c $(TEST_IR_DIR)/test_mir_dump_3.c $(TEST_IR_DIR)/test_mir_dump_4.c $(TEST_IR_DIR)/test_mir_dump_5.c $(TEST_IR_DIR)/test_mir_dump_6.c $(TEST_IR_DIR)/test_mir_dump_7.c $(TEST_IR_DIR)/test_mir_dump_8.c $(TEST_IR_DIR)/test_mir_dump_9.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_10.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_11.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_12.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_13.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_14.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_2.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_3.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_4.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_5.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_6.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_7.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_8.c $(TEST_SEMA_CHECKER_DIR)/test_type_checker_9.c $(TEST_SEMA_RESOLUTION_DIR)/test_type_resolution.c $(TEST_SEMA_RESOLUTION_DIR)/test_type_resolution_2.c $(TEST_SEMA_RESOLUTION_DIR)/test_type_resolution_3.c $(TEST_SEMA_SYMBOLS_DIR)/test_semantic_dump.c $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table.c $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_2.c $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_3.c $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_4.c $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_5.c $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_6.c

# Object files
TOKENIZER_OBJS           = $(BUILD_DIR)/tokenizer.o $(BUILD_DIR)/tokenizer_keywords.o $(BUILD_DIR)/tokenizer_literals.o $(BUILD_DIR)/tokenizer_punctuation.o $(BUILD_DIR)/tokenizer_type_names.o
AST_OBJS                 = $(BUILD_DIR)/ast.o $(BUILD_DIR)/ast_declarations.o $(BUILD_DIR)/ast_expressions.o $(BUILD_DIR)/ast_statements.o
PARSER_OBJS              = $(BUILD_DIR)/parser.o $(BUILD_DIR)/parser_binary.o $(BUILD_DIR)/parser_decl.o $(BUILD_DIR)/parser_decl_ext.o $(BUILD_DIR)/parser_expr.o $(BUILD_DIR)/parser_layout.o $(BUILD_DIR)/parser_literals.o $(BUILD_DIR)/parser_lookahead.o $(BUILD_DIR)/parser_postfix.o $(BUILD_DIR)/parser_primary.o $(BUILD_DIR)/parser_stmt.o $(BUILD_DIR)/parser_types.o $(BUILD_DIR)/parser_union.o $(BUILD_DIR)/parser_utils.o
AST_DUMP_OBJS            = $(BUILD_DIR)/ast_dump.o $(BUILD_DIR)/ast_dump_decl.o $(BUILD_DIR)/ast_dump_expr.o $(BUILD_DIR)/ast_dump_names.o $(BUILD_DIR)/ast_dump_stmt.o $(BUILD_DIR)/ast_dump_types.o
SYMBOL_TABLE_OBJS        = $(BUILD_DIR)/symbol_table.o $(BUILD_DIR)/symbol_table_analyze.o $(BUILD_DIR)/symbol_table_analyze_expr.o $(BUILD_DIR)/symbol_table_core.o $(BUILD_DIR)/symbol_table_imports.o $(BUILD_DIR)/symbol_table_predecl.o $(BUILD_DIR)/symbol_table_query.o $(BUILD_DIR)/symbol_table_registry.o
TYPE_RESOLUTION_OBJS     = $(BUILD_DIR)/type_resolution.o $(BUILD_DIR)/type_resolution_expr.o $(BUILD_DIR)/type_resolution_helpers.o $(BUILD_DIR)/type_resolution_resolve.o
TYPE_CHECKER_OBJS        = $(BUILD_DIR)/type_checker.o $(BUILD_DIR)/type_checker_block.o $(BUILD_DIR)/type_checker_check.o $(BUILD_DIR)/type_checker_convert.o $(BUILD_DIR)/type_checker_expr.o $(BUILD_DIR)/type_checker_expr_array.o $(BUILD_DIR)/type_checker_expr_ext.o $(BUILD_DIR)/type_checker_expr_memory.o $(BUILD_DIR)/type_checker_expr_more.o $(BUILD_DIR)/type_checker_lambda.o $(BUILD_DIR)/type_checker_names.o $(BUILD_DIR)/type_checker_ops.o $(BUILD_DIR)/type_checker_resolve.o $(BUILD_DIR)/type_checker_resolve_binding.o $(BUILD_DIR)/type_checker_types.o $(BUILD_DIR)/type_checker_util.o
SEMANTIC_DUMP_OBJS       = $(BUILD_DIR)/semantic_dump.o $(BUILD_DIR)/semantic_dump_builder.o $(BUILD_DIR)/semantic_dump_scope.o $(BUILD_DIR)/semantic_dump_symbols.o $(BUILD_DIR)/semantic_dump_types.o
HIR_OBJS                 = $(BUILD_DIR)/hir.o $(BUILD_DIR)/hir_helpers.o $(BUILD_DIR)/hir_lower.o $(BUILD_DIR)/hir_lower_decls.o $(BUILD_DIR)/hir_lower_decls_bind.o $(BUILD_DIR)/hir_lower_expr.o $(BUILD_DIR)/hir_lower_expr_ext.o $(BUILD_DIR)/hir_lower_expr_memory.o $(BUILD_DIR)/hir_lower_stmt.o $(BUILD_DIR)/hir_memory.o $(BUILD_DIR)/hir_memory_alloc.o
HIR_DUMP_OBJS            = $(BUILD_DIR)/hir_dump.o $(BUILD_DIR)/hir_dump_expr.o $(BUILD_DIR)/hir_dump_expr_ext.o $(BUILD_DIR)/hir_dump_helpers.o
MIR_OBJS                 = $(BUILD_DIR)/mir.o $(BUILD_DIR)/mir_assign.o $(BUILD_DIR)/mir_build.o $(BUILD_DIR)/mir_builders.o $(BUILD_DIR)/mir_capture.o $(BUILD_DIR)/mir_capture_analysis.o $(BUILD_DIR)/mir_capture_analysis_expr.o $(BUILD_DIR)/mir_cleanup.o $(BUILD_DIR)/mir_control.o $(BUILD_DIR)/mir_expr.o $(BUILD_DIR)/mir_expr_call.o $(BUILD_DIR)/mir_expr_helpers.o $(BUILD_DIR)/mir_expr_memory.o $(BUILD_DIR)/mir_lambda.o $(BUILD_DIR)/mir_lower.o $(BUILD_DIR)/mir_lvalue.o $(BUILD_DIR)/mir_manual_cleanup.o $(BUILD_DIR)/mir_start.o $(BUILD_DIR)/mir_store.o $(BUILD_DIR)/mir_union.o $(BUILD_DIR)/mir_value.o
MIR_DUMP_OBJS            = $(BUILD_DIR)/mir_dump.o $(BUILD_DIR)/mir_dump_helpers.o $(BUILD_DIR)/mir_dump_instr.o
BYTECODE_OBJS            = $(BUILD_DIR)/bytecode.o $(BUILD_DIR)/bytecode_constants.o $(BUILD_DIR)/bytecode_helpers.o $(BUILD_DIR)/bytecode_lower.o $(BUILD_DIR)/bytecode_lower_instr.o $(BUILD_DIR)/bytecode_memory.o
BYTECODE_DUMP_OBJS       = $(BUILD_DIR)/bytecode_dump.o $(BUILD_DIR)/bytecode_dump_helpers.o $(BUILD_DIR)/bytecode_dump_instr.o $(BUILD_DIR)/bytecode_dump_names.o
LIR_OBJS                 = $(BUILD_DIR)/lir.o $(BUILD_DIR)/lir_helpers.o $(BUILD_DIR)/lir_lower.o $(BUILD_DIR)/lir_lower_instr.o $(BUILD_DIR)/lir_lower_instr_ext.o $(BUILD_DIR)/lir_lower_instr_stores.o $(BUILD_DIR)/lir_memory.o
LIR_DUMP_OBJS            = $(BUILD_DIR)/lir_dump.o $(BUILD_DIR)/lir_dump_helpers.o $(BUILD_DIR)/lir_dump_instr.o
CODEGEN_OBJS             = $(BUILD_DIR)/codegen.o $(BUILD_DIR)/codegen_build.o $(BUILD_DIR)/codegen_helpers.o $(BUILD_DIR)/codegen_infer.o $(BUILD_DIR)/codegen_names.o
CODEGEN_DUMP_OBJS        = $(BUILD_DIR)/codegen_dump.o $(BUILD_DIR)/codegen_dump_helpers.o
TARGET_OBJS              = $(BUILD_DIR)/target.o $(BUILD_DIR)/target_aarch64.o $(BUILD_DIR)/target_riscv64.o $(BUILD_DIR)/target_x86_64.o
RUNTIME_ABI_OBJS         = $(BUILD_DIR)/runtime_abi.o $(BUILD_DIR)/runtime_abi_dump.o $(BUILD_DIR)/runtime_abi_names.o
RUNTIME_OBJS             = $(BUILD_DIR)/runtime.o $(BUILD_DIR)/runtime_bounds.o $(BUILD_DIR)/runtime_format.o $(BUILD_DIR)/runtime_format_hetero.o $(BUILD_DIR)/runtime_manual.o $(BUILD_DIR)/runtime_names.o $(BUILD_DIR)/runtime_ops.o $(BUILD_DIR)/runtime_threads.o $(BUILD_DIR)/runtime_union.o
MACHINE_OBJS             = $(BUILD_DIR)/machine.o $(BUILD_DIR)/machine_build.o $(BUILD_DIR)/machine_dump.o $(BUILD_DIR)/machine_emit.o $(BUILD_DIR)/machine_helpers.o $(BUILD_DIR)/machine_instr.o $(BUILD_DIR)/machine_instr_rt.o $(BUILD_DIR)/machine_instr_rt_ext.o $(BUILD_DIR)/machine_layout.o $(BUILD_DIR)/machine_operand.o $(BUILD_DIR)/machine_operand_preserve.o $(BUILD_DIR)/machine_ops.o $(BUILD_DIR)/machine_ops_unary.o
ASM_EMIT_OBJS            = $(BUILD_DIR)/asm_emit.o $(BUILD_DIR)/asm_emit_entry.o $(BUILD_DIR)/asm_emit_entry_aarch64.o $(BUILD_DIR)/asm_emit_entry_riscv64.o $(BUILD_DIR)/asm_emit_helpers.o $(BUILD_DIR)/asm_emit_helpers_aarch64.o $(BUILD_DIR)/asm_emit_helpers_riscv64.o $(BUILD_DIR)/asm_emit_instr.o $(BUILD_DIR)/asm_emit_instr_aarch64.o $(BUILD_DIR)/asm_emit_instr_riscv64.o $(BUILD_DIR)/asm_emit_operand.o $(BUILD_DIR)/asm_emit_operand_aarch64.o $(BUILD_DIR)/asm_emit_operand_riscv64.o $(BUILD_DIR)/asm_emit_operand_ext.o $(BUILD_DIR)/asm_emit_operand_ext_aarch64.o $(BUILD_DIR)/asm_emit_operand_ext_riscv64.o $(BUILD_DIR)/asm_emit_sections.o $(BUILD_DIR)/asm_emit_symbols.o $(BUILD_DIR)/asm_emit_type_desc.o
DUMP_AST_OBJS            = $(BUILD_DIR)/dump_ast.o
DUMP_SEMANTICS_OBJS      = $(BUILD_DIR)/dump_semantics.o
EMIT_ASM_OBJS            = $(BUILD_DIR)/emit_asm.o
EMIT_BYTECODE_OBJS       = $(BUILD_DIR)/emit_bytecode.o
BUILD_NATIVE_OBJS        = $(BUILD_DIR)/build_native.o $(BUILD_DIR)/build_native_utils.o
CALYNDA_CLI_OBJS         = $(BUILD_DIR)/calynda.o $(BUILD_DIR)/calynda_car.o $(BUILD_DIR)/calynda_car_helpers.o $(BUILD_DIR)/calynda_commands.o $(BUILD_DIR)/calynda_compile.o $(BUILD_DIR)/calynda_utils.o
CAR_OBJS                 = $(BUILD_DIR)/car.o $(BUILD_DIR)/car_dir.o $(BUILD_DIR)/car_read.o $(BUILD_DIR)/car_write.o
TEST_TOKENIZER_OBJS      = $(BUILD_DIR)/test_tokenizer.o $(BUILD_DIR)/test_tokenizer_2.o $(BUILD_DIR)/test_tokenizer_3.o $(BUILD_DIR)/test_tokenizer_4.o $(BUILD_DIR)/test_tokenizer_5.o
TEST_AST_OBJS            = $(BUILD_DIR)/test_ast.o $(BUILD_DIR)/test_ast_2.o $(BUILD_DIR)/test_ast_3.o $(BUILD_DIR)/test_ast_4.o $(BUILD_DIR)/test_ast_5.o
TEST_PARSER_OBJS         = $(BUILD_DIR)/test_parser.o $(BUILD_DIR)/test_parser_2.o $(BUILD_DIR)/test_parser_3.o $(BUILD_DIR)/test_parser_4.o $(BUILD_DIR)/test_parser_5.o $(BUILD_DIR)/test_parser_6.o $(BUILD_DIR)/test_parser_7.o $(BUILD_DIR)/test_parser_8.o $(BUILD_DIR)/test_parser_9.o
TEST_AST_DUMP_OBJS       = $(BUILD_DIR)/test_ast_dump.o $(BUILD_DIR)/test_ast_dump_2.o $(BUILD_DIR)/test_ast_dump_3.o
TEST_SYMBOL_TABLE_OBJS   = $(BUILD_DIR)/test_symbol_table.o $(BUILD_DIR)/test_symbol_table_2.o $(BUILD_DIR)/test_symbol_table_3.o $(BUILD_DIR)/test_symbol_table_4.o $(BUILD_DIR)/test_symbol_table_5.o $(BUILD_DIR)/test_symbol_table_6.o
TEST_TYPE_RESOLUTION_OBJS = $(BUILD_DIR)/test_type_resolution.o $(BUILD_DIR)/test_type_resolution_2.o $(BUILD_DIR)/test_type_resolution_3.o
TEST_TYPE_CHECKER_OBJS   = $(BUILD_DIR)/test_type_checker.o $(BUILD_DIR)/test_type_checker_10.o $(BUILD_DIR)/test_type_checker_11.o $(BUILD_DIR)/test_type_checker_12.o $(BUILD_DIR)/test_type_checker_13.o $(BUILD_DIR)/test_type_checker_14.o $(BUILD_DIR)/test_type_checker_2.o $(BUILD_DIR)/test_type_checker_3.o $(BUILD_DIR)/test_type_checker_4.o $(BUILD_DIR)/test_type_checker_5.o $(BUILD_DIR)/test_type_checker_6.o $(BUILD_DIR)/test_type_checker_7.o $(BUILD_DIR)/test_type_checker_8.o $(BUILD_DIR)/test_type_checker_9.o
TEST_SEMANTIC_DUMP_OBJS  = $(BUILD_DIR)/test_semantic_dump.o
TEST_HIR_DUMP_OBJS       = $(BUILD_DIR)/test_hir_dump.o $(BUILD_DIR)/test_hir_dump_2.o $(BUILD_DIR)/test_hir_dump_3.o $(BUILD_DIR)/test_hir_dump_4.o
TEST_MIR_DUMP_OBJS       = $(BUILD_DIR)/test_mir_dump.o $(BUILD_DIR)/test_mir_dump_2.o $(BUILD_DIR)/test_mir_dump_3.o $(BUILD_DIR)/test_mir_dump_4.o $(BUILD_DIR)/test_mir_dump_5.o $(BUILD_DIR)/test_mir_dump_6.o $(BUILD_DIR)/test_mir_dump_7.o $(BUILD_DIR)/test_mir_dump_8.o $(BUILD_DIR)/test_mir_dump_9.o
TEST_BYTECODE_DUMP_OBJS  = $(BUILD_DIR)/test_bytecode_dump.o
TEST_BYTECODE_DUMP_2_OBJS = $(BUILD_DIR)/test_bytecode_dump_2.o
TEST_BYTECODE_DUMP_3_OBJS = $(BUILD_DIR)/test_bytecode_dump_3.o
TEST_LIR_DUMP_OBJS       = $(BUILD_DIR)/test_lir_dump.o $(BUILD_DIR)/test_lir_dump_2.o $(BUILD_DIR)/test_lir_dump_3.o $(BUILD_DIR)/test_lir_dump_4.o
TEST_CODEGEN_DUMP_OBJS   = $(BUILD_DIR)/test_codegen_dump.o $(BUILD_DIR)/test_codegen_dump_2.o $(BUILD_DIR)/test_codegen_dump_3.o $(BUILD_DIR)/test_codegen_dump_4.o
TEST_MACHINE_DUMP_OBJS   = $(BUILD_DIR)/test_machine_dump.o $(BUILD_DIR)/test_machine_dump_2.o $(BUILD_DIR)/test_machine_dump_3.o
TEST_RUNTIME_OBJS        = $(BUILD_DIR)/test_runtime.o $(BUILD_DIR)/test_runtime_2.o
TEST_ASM_EMIT_OBJS       = $(BUILD_DIR)/test_asm_emit.o $(BUILD_DIR)/test_asm_emit_2.o $(BUILD_DIR)/test_asm_emit_3.o $(BUILD_DIR)/test_asm_emit_4.o $(BUILD_DIR)/test_asm_emit_cross.o
TEST_BUILD_NATIVE_OBJS   = $(BUILD_DIR)/test_build_native.o $(BUILD_DIR)/test_build_native_2.o $(BUILD_DIR)/test_build_native_3.o $(BUILD_DIR)/test_build_native_4.o $(BUILD_DIR)/test_build_native_5.o $(BUILD_DIR)/test_build_native_6.o
TEST_CALYNDA_CLI_OBJS    = $(BUILD_DIR)/test_calynda_cli.o $(BUILD_DIR)/test_calynda_cli_2.o $(BUILD_DIR)/test_calynda_cli_3.o $(BUILD_DIR)/test_calynda_cli_4.o
TEST_CAR_OBJS            = $(BUILD_DIR)/test_car.o $(BUILD_DIR)/test_car_2.o $(BUILD_DIR)/test_car_3.o $(BUILD_DIR)/test_car_4.o

# Test binaries
TEST_TOKENIZER_BIN       = $(BUILD_DIR)/test_tokenizer
TEST_AST_BIN             = $(BUILD_DIR)/test_ast
TEST_PARSER_BIN          = $(BUILD_DIR)/test_parser
TEST_AST_DUMP_BIN        = $(BUILD_DIR)/test_ast_dump
TEST_SYMBOL_TABLE_BIN    = $(BUILD_DIR)/test_symbol_table
TEST_TYPE_RESOLUTION_BIN = $(BUILD_DIR)/test_type_resolution
TEST_TYPE_CHECKER_BIN    = $(BUILD_DIR)/test_type_checker
TEST_SEMANTIC_DUMP_BIN   = $(BUILD_DIR)/test_semantic_dump
TEST_HIR_DUMP_BIN        = $(BUILD_DIR)/test_hir_dump
TEST_MIR_DUMP_BIN        = $(BUILD_DIR)/test_mir_dump
TEST_BYTECODE_DUMP_BIN   = $(BUILD_DIR)/test_bytecode_dump
TEST_BYTECODE_DUMP_2_BIN = $(BUILD_DIR)/test_bytecode_dump_2
TEST_BYTECODE_DUMP_3_BIN = $(BUILD_DIR)/test_bytecode_dump_3
TEST_LIR_DUMP_BIN        = $(BUILD_DIR)/test_lir_dump
TEST_CODEGEN_DUMP_BIN    = $(BUILD_DIR)/test_codegen_dump
TEST_MACHINE_DUMP_BIN    = $(BUILD_DIR)/test_machine_dump
TEST_RUNTIME_BIN         = $(BUILD_DIR)/test_runtime
TEST_ASM_EMIT_BIN        = $(BUILD_DIR)/test_asm_emit
TEST_BUILD_NATIVE_BIN    = $(BUILD_DIR)/test_build_native
TEST_CALYNDA_CLI_BIN     = $(BUILD_DIR)/test_calynda_cli
TEST_CAR_BIN             = $(BUILD_DIR)/test_car
TEST_BINS                = $(TEST_TOKENIZER_BIN) $(TEST_AST_BIN) $(TEST_PARSER_BIN) $(TEST_AST_DUMP_BIN) $(TEST_SYMBOL_TABLE_BIN) $(TEST_TYPE_RESOLUTION_BIN) $(TEST_TYPE_CHECKER_BIN) $(TEST_SEMANTIC_DUMP_BIN) $(TEST_HIR_DUMP_BIN) $(TEST_MIR_DUMP_BIN) $(TEST_BYTECODE_DUMP_BIN) $(TEST_BYTECODE_DUMP_2_BIN) $(TEST_BYTECODE_DUMP_3_BIN) $(TEST_LIR_DUMP_BIN) $(TEST_CODEGEN_DUMP_BIN) $(TEST_MACHINE_DUMP_BIN) $(TEST_RUNTIME_BIN) $(TEST_ASM_EMIT_BIN) $(TEST_BUILD_NATIVE_BIN) $(TEST_CALYNDA_CLI_BIN) $(TEST_CAR_BIN)

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

# --- Source file compile rules ---

$(BUILD_DIR)/ast.o: $(AST_DIR)/ast.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_declarations.o: $(AST_DIR)/ast_declarations.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_expressions.o: $(AST_DIR)/ast_expressions.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_statements.o: $(AST_DIR)/ast_statements.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump.o: $(AST_DUMP_DIR)/ast_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_decl.o: $(AST_DUMP_DIR)/ast_dump_decl.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_expr.o: $(AST_DUMP_DIR)/ast_dump_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_names.o: $(AST_DUMP_DIR)/ast_dump_names.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_stmt.o: $(AST_DUMP_DIR)/ast_dump_stmt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ast_dump_types.o: $(AST_DUMP_DIR)/ast_dump_types.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit.o: $(ASM_EMIT_DIR)/asm_emit.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_entry.o: $(ASM_EMIT_DIR)/asm_emit_entry.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_entry_aarch64.o: $(ASM_EMIT_DIR)/aarch64/asm_emit_entry_aarch64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_entry_riscv64.o: $(ASM_EMIT_DIR)/riscv64/asm_emit_entry_riscv64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_helpers.o: $(ASM_EMIT_DIR)/asm_emit_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_helpers_aarch64.o: $(ASM_EMIT_DIR)/aarch64/asm_emit_helpers_aarch64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_helpers_riscv64.o: $(ASM_EMIT_DIR)/riscv64/asm_emit_helpers_riscv64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_instr.o: $(ASM_EMIT_DIR)/asm_emit_instr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_instr_aarch64.o: $(ASM_EMIT_DIR)/aarch64/asm_emit_instr_aarch64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_instr_riscv64.o: $(ASM_EMIT_DIR)/riscv64/asm_emit_instr_riscv64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand.o: $(ASM_EMIT_DIR)/asm_emit_operand.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand_aarch64.o: $(ASM_EMIT_DIR)/aarch64/asm_emit_operand_aarch64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand_riscv64.o: $(ASM_EMIT_DIR)/riscv64/asm_emit_operand_riscv64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand_ext.o: $(ASM_EMIT_DIR)/asm_emit_operand_ext.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand_ext_aarch64.o: $(ASM_EMIT_DIR)/aarch64/asm_emit_operand_ext_aarch64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_operand_ext_riscv64.o: $(ASM_EMIT_DIR)/riscv64/asm_emit_operand_ext_riscv64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_sections.o: $(ASM_EMIT_DIR)/asm_emit_sections.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_symbols.o: $(ASM_EMIT_DIR)/asm_emit_symbols.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_emit_type_desc.o: $(ASM_EMIT_DIR)/asm_emit_type_desc.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine.o: $(MACHINE_DIR)/machine.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_build.o: $(MACHINE_DIR)/machine_build.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_dump.o: $(MACHINE_DIR)/machine_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_emit.o: $(MACHINE_DIR)/machine_emit.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_helpers.o: $(MACHINE_DIR)/machine_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_instr.o: $(MACHINE_DIR)/machine_instr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_instr_rt.o: $(MACHINE_DIR)/machine_instr_rt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_instr_rt_ext.o: $(MACHINE_DIR)/machine_instr_rt_ext.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_layout.o: $(MACHINE_DIR)/machine_layout.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_operand.o: $(MACHINE_DIR)/machine_operand.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_operand_preserve.o: $(MACHINE_DIR)/machine_operand_preserve.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_ops.o: $(MACHINE_DIR)/machine_ops.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/machine_ops_unary.o: $(MACHINE_DIR)/machine_ops_unary.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_abi.o: $(RUNTIME_ABI_DIR)/runtime_abi.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_abi_dump.o: $(RUNTIME_ABI_DIR)/runtime_abi_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_abi_names.o: $(RUNTIME_ABI_DIR)/runtime_abi_names.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/target.o: $(TARGET_DIR)/target.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/target_aarch64.o: $(TARGET_DIR)/target_aarch64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/target_x86_64.o: $(TARGET_DIR)/target_x86_64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/target_riscv64.o: $(TARGET_DIR)/target_riscv64.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode.o: $(BYTECODE_DIR)/bytecode.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_constants.o: $(BYTECODE_DIR)/bytecode_constants.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump.o: $(BYTECODE_DIR)/bytecode_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump_helpers.o: $(BYTECODE_DIR)/bytecode_dump_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump_instr.o: $(BYTECODE_DIR)/bytecode_dump_instr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_dump_names.o: $(BYTECODE_DIR)/bytecode_dump_names.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_helpers.o: $(BYTECODE_DIR)/bytecode_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_lower.o: $(BYTECODE_DIR)/bytecode_lower.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_lower_instr.o: $(BYTECODE_DIR)/bytecode_lower_instr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bytecode_memory.o: $(BYTECODE_DIR)/bytecode_memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car.o: $(CAR_DIR)/car.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car_dir.o: $(CAR_DIR)/car_dir.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car_read.o: $(CAR_DIR)/car_read.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/car_write.o: $(CAR_DIR)/car_write.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/build_native.o: $(CLI_DIR)/build_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/build_native_utils.o: $(CLI_DIR)/build_native_utils.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda.o: $(CLI_DIR)/calynda.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_car.o: $(CLI_DIR)/calynda_car.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_car_helpers.o: $(CLI_DIR)/calynda_car_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_commands.o: $(CLI_DIR)/calynda_commands.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_compile.o: $(CLI_DIR)/calynda_compile.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/calynda_utils.o: $(CLI_DIR)/calynda_utils.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_ast.o: $(CLI_DIR)/dump_ast.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dump_semantics.o: $(CLI_DIR)/dump_semantics.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/emit_asm.o: $(CLI_DIR)/emit_asm.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/emit_bytecode.o: $(CLI_DIR)/emit_bytecode.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen.o: $(CODEGEN_DIR)/codegen.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_build.o: $(CODEGEN_DIR)/codegen_build.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_dump.o: $(CODEGEN_DIR)/codegen_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_dump_helpers.o: $(CODEGEN_DIR)/codegen_dump_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_helpers.o: $(CODEGEN_DIR)/codegen_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_infer.o: $(CODEGEN_DIR)/codegen_infer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen_names.o: $(CODEGEN_DIR)/codegen_names.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump.o: $(HIR_DUMP_DIR)/hir_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_expr.o: $(HIR_DUMP_DIR)/hir_dump_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_expr_ext.o: $(HIR_DUMP_DIR)/hir_dump_expr_ext.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_dump_helpers.o: $(HIR_DUMP_DIR)/hir_dump_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir.o: $(HIR_DIR)/hir.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_helpers.o: $(HIR_DIR)/hir_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_memory.o: $(HIR_DIR)/hir_memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_memory_alloc.o: $(HIR_DIR)/hir_memory_alloc.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower.o: $(HIR_LOWER_DIR)/hir_lower.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_decls.o: $(HIR_LOWER_DIR)/hir_lower_decls.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_decls_bind.o: $(HIR_LOWER_DIR)/hir_lower_decls_bind.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_expr.o: $(HIR_LOWER_DIR)/hir_lower_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_expr_ext.o: $(HIR_LOWER_DIR)/hir_lower_expr_ext.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_expr_memory.o: $(HIR_LOWER_DIR)/hir_lower_expr_memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hir_lower_stmt.o: $(HIR_LOWER_DIR)/hir_lower_stmt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir.o: $(LIR_DIR)/lir.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_dump.o: $(LIR_DIR)/lir_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_dump_helpers.o: $(LIR_DIR)/lir_dump_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_dump_instr.o: $(LIR_DIR)/lir_dump_instr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_helpers.o: $(LIR_DIR)/lir_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower.o: $(LIR_DIR)/lir_lower.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower_instr.o: $(LIR_DIR)/lir_lower_instr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower_instr_ext.o: $(LIR_DIR)/lir_lower_instr_ext.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_lower_instr_stores.o: $(LIR_DIR)/lir_lower_instr_stores.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/lir_memory.o: $(LIR_DIR)/lir_memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_dump.o: $(MIR_DUMP_DIR)/mir_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_dump_helpers.o: $(MIR_DUMP_DIR)/mir_dump_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_dump_instr.o: $(MIR_DUMP_DIR)/mir_dump_instr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_assign.o: $(MIR_LOWER_DIR)/mir_assign.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_control.o: $(MIR_LOWER_DIR)/mir_control.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_expr.o: $(MIR_LOWER_DIR)/mir_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_expr_call.o: $(MIR_LOWER_DIR)/mir_expr_call.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_expr_helpers.o: $(MIR_LOWER_DIR)/mir_expr_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_expr_memory.o: $(MIR_LOWER_DIR)/mir_expr_memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_manual_cleanup.o: $(MIR_LOWER_DIR)/mir_manual_cleanup.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_lambda.o: $(MIR_LOWER_DIR)/mir_lambda.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_lower.o: $(MIR_LOWER_DIR)/mir_lower.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_lvalue.o: $(MIR_LOWER_DIR)/mir_lvalue.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_start.o: $(MIR_LOWER_DIR)/mir_start.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_store.o: $(MIR_LOWER_DIR)/mir_store.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_union.o: $(MIR_LOWER_DIR)/mir_union.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir.o: $(MIR_DIR)/mir.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_build.o: $(MIR_DIR)/mir_build.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_builders.o: $(MIR_DIR)/mir_builders.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_capture.o: $(MIR_DIR)/mir_capture.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_capture_analysis.o: $(MIR_DIR)/mir_capture_analysis.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_capture_analysis_expr.o: $(MIR_DIR)/mir_capture_analysis_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_cleanup.o: $(MIR_DIR)/mir_cleanup.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/mir_value.o: $(MIR_DIR)/mir_value.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: $(PARSER_DIR)/parser.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_binary.o: $(PARSER_DIR)/parser_binary.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_decl.o: $(PARSER_DIR)/parser_decl.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_decl_ext.o: $(PARSER_DIR)/parser_decl_ext.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_expr.o: $(PARSER_DIR)/parser_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_literals.o: $(PARSER_DIR)/parser_literals.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_lookahead.o: $(PARSER_DIR)/parser_lookahead.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_postfix.o: $(PARSER_DIR)/parser_postfix.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_primary.o: $(PARSER_DIR)/parser_primary.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_stmt.o: $(PARSER_DIR)/parser_stmt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_types.o: $(PARSER_DIR)/parser_types.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_layout.o: $(PARSER_DIR)/parser_layout.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_union.o: $(PARSER_DIR)/parser_union.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parser_utils.o: $(PARSER_DIR)/parser_utils.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime.o: $(RUNTIME_DIR)/runtime.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_format.o: $(RUNTIME_DIR)/runtime_format.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_format_hetero.o: $(RUNTIME_DIR)/runtime_format_hetero.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_names.o: $(RUNTIME_DIR)/runtime_names.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_ops.o: $(RUNTIME_DIR)/runtime_ops.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_union.o: $(RUNTIME_DIR)/runtime_union.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_manual.o: $(RUNTIME_DIR)/runtime_manual.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_bounds.o: $(RUNTIME_DIR)/runtime_bounds.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump.o: $(SEMANTIC_DUMP_DIR)/semantic_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_builder.o: $(SEMANTIC_DUMP_DIR)/semantic_dump_builder.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_scope.o: $(SEMANTIC_DUMP_DIR)/semantic_dump_scope.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_symbols.o: $(SEMANTIC_DUMP_DIR)/semantic_dump_symbols.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic_dump_types.o: $(SEMANTIC_DUMP_DIR)/semantic_dump_types.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table.o: $(SYMBOL_TABLE_DIR)/symbol_table.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_analyze.o: $(SYMBOL_TABLE_DIR)/symbol_table_analyze.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_analyze_expr.o: $(SYMBOL_TABLE_DIR)/symbol_table_analyze_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_core.o: $(SYMBOL_TABLE_DIR)/symbol_table_core.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_imports.o: $(SYMBOL_TABLE_DIR)/symbol_table_imports.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_predecl.o: $(SYMBOL_TABLE_DIR)/symbol_table_predecl.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_query.o: $(SYMBOL_TABLE_DIR)/symbol_table_query.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/symbol_table_registry.o: $(SYMBOL_TABLE_DIR)/symbol_table_registry.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr.o: $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_array.o: $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_array.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_ext.o: $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_ext.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_memory.o: $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_expr_more.o: $(TYPE_CHECKER_EXPR_DIR)/type_checker_expr_more.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker.o: $(TYPE_CHECKER_DIR)/type_checker.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_block.o: $(TYPE_CHECKER_DIR)/type_checker_block.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_check.o: $(TYPE_CHECKER_DIR)/type_checker_check.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_convert.o: $(TYPE_CHECKER_DIR)/type_checker_convert.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_lambda.o: $(TYPE_CHECKER_DIR)/type_checker_lambda.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_names.o: $(TYPE_CHECKER_DIR)/type_checker_names.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_ops.o: $(TYPE_CHECKER_DIR)/type_checker_ops.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_resolve.o: $(TYPE_CHECKER_DIR)/type_checker_resolve.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_resolve_binding.o: $(TYPE_CHECKER_DIR)/type_checker_resolve_binding.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_types.o: $(TYPE_CHECKER_DIR)/type_checker_types.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_checker_util.o: $(TYPE_CHECKER_DIR)/type_checker_util.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution.o: $(TYPE_RESOLUTION_DIR)/type_resolution.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_expr.o: $(TYPE_RESOLUTION_DIR)/type_resolution_expr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_helpers.o: $(TYPE_RESOLUTION_DIR)/type_resolution_helpers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/type_resolution_resolve.o: $(TYPE_RESOLUTION_DIR)/type_resolution_resolve.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer.o: $(TOKENIZER_DIR)/tokenizer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_keywords.o: $(TOKENIZER_DIR)/tokenizer_keywords.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_literals.o: $(TOKENIZER_DIR)/tokenizer_literals.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_punctuation.o: $(TOKENIZER_DIR)/tokenizer_punctuation.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tokenizer_type_names.o: $(TOKENIZER_DIR)/tokenizer_type_names.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Test file compile rules ---

$(BUILD_DIR)/test_codegen_dump.o: $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_codegen_dump_2.o: $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_codegen_dump_3.o: $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_codegen_dump_4.o: $(TEST_BACKEND_CODEGEN_DIR)/test_codegen_dump_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_machine_dump.o: $(TEST_BACKEND_CODEGEN_DIR)/test_machine_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_machine_dump_2.o: $(TEST_BACKEND_CODEGEN_DIR)/test_machine_dump_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_machine_dump_3.o: $(TEST_BACKEND_CODEGEN_DIR)/test_machine_dump_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_asm_emit.o: $(TEST_BACKEND_EMIT_DIR)/test_asm_emit.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_asm_emit_2.o: $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_asm_emit_3.o: $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_asm_emit_4.o: $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_asm_emit_cross.o: $(TEST_BACKEND_EMIT_DIR)/test_asm_emit_cross.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native.o: $(TEST_BACKEND_EMIT_DIR)/test_build_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native_2.o: $(TEST_BACKEND_EMIT_DIR)/test_build_native_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native_3.o: $(TEST_BACKEND_EMIT_DIR)/test_build_native_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native_4.o: $(TEST_BACKEND_EMIT_DIR)/test_build_native_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native_5.o: $(TEST_BACKEND_EMIT_DIR)/test_build_native_5.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_build_native_6.o: $(TEST_BACKEND_EMIT_DIR)/test_build_native_6.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_calynda_cli.o: $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_calynda_cli_2.o: $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_calynda_cli_3.o: $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_calynda_cli_4.o: $(TEST_BACKEND_TOOLS_DIR)/test_calynda_cli_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_car.o: $(TEST_BACKEND_TOOLS_DIR)/test_car.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_car_2.o: $(TEST_BACKEND_TOOLS_DIR)/test_car_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_car_3.o: $(TEST_BACKEND_TOOLS_DIR)/test_car_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_car_4.o: $(TEST_BACKEND_TOOLS_DIR)/test_car_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_runtime.o: $(TEST_BACKEND_TOOLS_DIR)/test_runtime.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_runtime_2.o: $(TEST_BACKEND_TOOLS_DIR)/test_runtime_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/runtime_threads.o: $(RUNTIME_DIR)/runtime_threads.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast.o: $(TEST_FRONTEND_AST_DIR)/test_ast.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_2.o: $(TEST_FRONTEND_AST_DIR)/test_ast_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_3.o: $(TEST_FRONTEND_AST_DIR)/test_ast_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_4.o: $(TEST_FRONTEND_AST_DIR)/test_ast_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_5.o: $(TEST_FRONTEND_AST_DIR)/test_ast_5.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_dump.o: $(TEST_FRONTEND_AST_DIR)/test_ast_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_dump_2.o: $(TEST_FRONTEND_AST_DIR)/test_ast_dump_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_ast_dump_3.o: $(TEST_FRONTEND_AST_DIR)/test_ast_dump_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_2.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_3.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_4.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_5.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_5.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_6.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_6.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_7.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_7.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_8.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_8.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_parser_9.o: $(TEST_FRONTEND_PARSER_DIR)/test_parser_9.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer.o: $(TEST_FRONTEND_TOK_DIR)/test_tokenizer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer_2.o: $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer_3.o: $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer_4.o: $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_tokenizer_5.o: $(TEST_FRONTEND_TOK_DIR)/test_tokenizer_5.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_bytecode_dump.o: $(TEST_IR_DIR)/test_bytecode_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_bytecode_dump_2.o: $(TEST_IR_DIR)/test_bytecode_dump_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_bytecode_dump_3.o: $(TEST_IR_DIR)/test_bytecode_dump_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_hir_dump.o: $(TEST_IR_DIR)/test_hir_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_hir_dump_2.o: $(TEST_IR_DIR)/test_hir_dump_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_hir_dump_3.o: $(TEST_IR_DIR)/test_hir_dump_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_hir_dump_4.o: $(TEST_IR_DIR)/test_hir_dump_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_lir_dump.o: $(TEST_IR_DIR)/test_lir_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_lir_dump_2.o: $(TEST_IR_DIR)/test_lir_dump_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_lir_dump_3.o: $(TEST_IR_DIR)/test_lir_dump_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_lir_dump_4.o: $(TEST_IR_DIR)/test_lir_dump_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump.o: $(TEST_IR_DIR)/test_mir_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_2.o: $(TEST_IR_DIR)/test_mir_dump_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_3.o: $(TEST_IR_DIR)/test_mir_dump_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_4.o: $(TEST_IR_DIR)/test_mir_dump_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_5.o: $(TEST_IR_DIR)/test_mir_dump_5.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_6.o: $(TEST_IR_DIR)/test_mir_dump_6.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_7.o: $(TEST_IR_DIR)/test_mir_dump_7.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_8.o: $(TEST_IR_DIR)/test_mir_dump_8.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_mir_dump_9.o: $(TEST_IR_DIR)/test_mir_dump_9.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_10.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_10.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_11.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_11.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_12.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_12.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_13.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_13.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_14.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_14.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_2.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_3.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_4.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_5.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_5.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_6.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_6.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_7.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_7.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_8.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_8.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_checker_9.o: $(TEST_SEMA_CHECKER_DIR)/test_type_checker_9.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_resolution.o: $(TEST_SEMA_RESOLUTION_DIR)/test_type_resolution.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_resolution_2.o: $(TEST_SEMA_RESOLUTION_DIR)/test_type_resolution_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_type_resolution_3.o: $(TEST_SEMA_RESOLUTION_DIR)/test_type_resolution_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_semantic_dump.o: $(TEST_SEMA_SYMBOLS_DIR)/test_semantic_dump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table.o: $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table_2.o: $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table_3.o: $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_3.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table_4.o: $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_4.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table_5.o: $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_5.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_symbol_table_6.o: $(TEST_SEMA_SYMBOLS_DIR)/test_symbol_table_6.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Link rules ---

$(TEST_TOKENIZER_BIN): $(TOKENIZER_OBJS) $(TEST_TOKENIZER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_AST_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(TEST_AST_OBJS)
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

$(TEST_BYTECODE_DUMP_2_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(TEST_BYTECODE_DUMP_2_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_BYTECODE_DUMP_3_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(TEST_BYTECODE_DUMP_3_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_LIR_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TEST_LIR_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_CODEGEN_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(TEST_CODEGEN_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_MACHINE_DUMP_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(MACHINE_OBJS) $(TEST_MACHINE_DUMP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_RUNTIME_BIN): $(RUNTIME_OBJS) $(TEST_RUNTIME_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(PTHREAD_FLAGS)

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
	$(CC) $(CFLAGS) $^ -o $@ $(PTHREAD_FLAGS)

$(CALYNDA_BIN): $(TOKENIZER_OBJS) $(AST_OBJS) $(PARSER_OBJS) $(SYMBOL_TABLE_OBJS) $(TYPE_RESOLUTION_OBJS) $(TYPE_CHECKER_OBJS) $(HIR_OBJS) $(MIR_OBJS) $(MIR_DUMP_OBJS) $(BYTECODE_OBJS) $(BYTECODE_DUMP_OBJS) $(LIR_OBJS) $(LIR_DUMP_OBJS) $(TARGET_OBJS) $(CODEGEN_OBJS) $(CODEGEN_DUMP_OBJS) $(RUNTIME_ABI_OBJS) $(RUNTIME_OBJS) $(MACHINE_OBJS) $(ASM_EMIT_OBJS) $(CAR_OBJS) $(CALYNDA_CLI_OBJS) $(BUILD_DIR)/calynda_runtime.a
	$(CC) $(CFLAGS) $^ -o $@ $(PTHREAD_FLAGS)

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
	./$(TEST_BYTECODE_DUMP_2_BIN)
	./$(TEST_BYTECODE_DUMP_3_BIN)
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
                       $(BUILD_DIR)/runtime_bounds_aarch64.o \
                       $(BUILD_DIR)/runtime_names_aarch64.o \
                       $(BUILD_DIR)/runtime_format_aarch64.o \
                       $(BUILD_DIR)/runtime_manual_aarch64.o \
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

$(BUILD_DIR)/runtime_manual_aarch64.o: $(RUNTIME_DIR)/runtime_manual.c $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_AARCH64) $(CROSS_CFLAGS_AARCH64) -c $< -o $@

$(BUILD_DIR)/runtime_bounds_aarch64.o: $(RUNTIME_DIR)/runtime_bounds.c $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_AARCH64) $(CROSS_CFLAGS_AARCH64) -c $< -o $@

$(BUILD_DIR)/calynda_runtime_aarch64.a: $(RUNTIME_AARCH64_OBJS)
	ar rcs $@ $^

runtime-aarch64: $(BUILD_DIR)/calynda_runtime_aarch64.a

# --- RISC-V 64 cross-compiled runtime library ---
CROSS_CC_RISCV64 = riscv64-linux-gnu-gcc
CROSS_CFLAGS_RISCV64 = -Wall -Wextra -pedantic -std=c11 -g -I$(RUNTIME_DIR)

RUNTIME_RISCV64_OBJS = $(BUILD_DIR)/runtime_riscv64.o \
                       $(BUILD_DIR)/runtime_bounds_riscv64.o \
                       $(BUILD_DIR)/runtime_names_riscv64.o \
                       $(BUILD_DIR)/runtime_format_riscv64.o \
                       $(BUILD_DIR)/runtime_manual_riscv64.o \
                       $(BUILD_DIR)/runtime_ops_riscv64.o \
                       $(BUILD_DIR)/runtime_union_riscv64.o

$(BUILD_DIR)/runtime_riscv64.o: $(RUNTIME_DIR)/runtime.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_RISCV64) $(CROSS_CFLAGS_RISCV64) -c $< -o $@

$(BUILD_DIR)/runtime_names_riscv64.o: $(RUNTIME_DIR)/runtime_names.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_RISCV64) $(CROSS_CFLAGS_RISCV64) -c $< -o $@

$(BUILD_DIR)/runtime_format_riscv64.o: $(RUNTIME_DIR)/runtime_format.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_RISCV64) $(CROSS_CFLAGS_RISCV64) -c $< -o $@

$(BUILD_DIR)/runtime_ops_riscv64.o: $(RUNTIME_DIR)/runtime_ops.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_RISCV64) $(CROSS_CFLAGS_RISCV64) -c $< -o $@

$(BUILD_DIR)/runtime_union_riscv64.o: $(RUNTIME_DIR)/runtime_union.c $(RUNTIME_DIR)/runtime_internal.h $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_RISCV64) $(CROSS_CFLAGS_RISCV64) -c $< -o $@

$(BUILD_DIR)/runtime_manual_riscv64.o: $(RUNTIME_DIR)/runtime_manual.c $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_RISCV64) $(CROSS_CFLAGS_RISCV64) -c $< -o $@

$(BUILD_DIR)/runtime_bounds_riscv64.o: $(RUNTIME_DIR)/runtime_bounds.c $(RUNTIME_DIR)/runtime.h | $(BUILD_DIR)
	$(CROSS_CC_RISCV64) $(CROSS_CFLAGS_RISCV64) -c $< -o $@

$(BUILD_DIR)/calynda_runtime_riscv64.a: $(RUNTIME_RISCV64_OBJS)
	ar rcs $@ $^

runtime-riscv64: $(BUILD_DIR)/calynda_runtime_riscv64.a
