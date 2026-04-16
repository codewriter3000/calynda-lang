#include "asm_emit_internal.h"

#include <stdlib.h>
#include <string.h>

AsmTypeDescriptorLiteral *ae_ensure_type_descriptor_literal(AsmEmitContext *context,
                                                            const char *encoded) {
    AsmTypeDescriptorLiteral literal;
    char *copy;
    char *cursor;
    char *generic_text;
    char *name;
    size_t i;

    if (!context || !encoded) {
        return NULL;
    }
    for (i = 0; i < context->type_descriptor_count; i++) {
        if (strcmp(context->type_descriptors[i].encoded, encoded) == 0) {
            return &context->type_descriptors[i];
        }
    }

    memset(&literal, 0, sizeof(literal));
    copy = ae_copy_text(encoded);
    if (!copy) {
        return NULL;
    }
    name = copy;
    generic_text = strchr(copy, '|');
    if (!generic_text) {
        free(copy);
        return NULL;
    }
    *generic_text++ = '\0';
    cursor = strchr(generic_text, '|');
    if (cursor) {
        *cursor++ = '\0';
    }
    literal.generic_param_count = (size_t)strtoull(generic_text, NULL, 10);
    if (literal.generic_param_count > 0) {
        literal.generic_param_tags = calloc(literal.generic_param_count,
                                            sizeof(*literal.generic_param_tags));
        if (!literal.generic_param_tags) {
            free(copy);
            return NULL;
        }
        for (i = 0; i < literal.generic_param_count; i++) {
            CalyndaRtTypeTag tag;
            char *generic_name;
            char *next = cursor ? strchr(cursor, '|') : NULL;

            if (next) {
                *next++ = '\0';
            }
            if (!cursor || !ae_parse_variant_spec(cursor, &generic_name, &tag) ||
                generic_name[0] != 'g') {
                free(copy);
                free(literal.generic_param_tags);
                return NULL;
            }
            literal.generic_param_tags[i] = tag;
            cursor = next;
        }
    }

    if (cursor) {
        char *probe = cursor;

        while (probe && *probe) {
            literal.variant_count++;
            probe = strchr(probe, '|');
            if (probe) {
                probe++;
            }
        }
    }

    if (literal.variant_count > 0) {
        literal.variant_name_labels = calloc(literal.variant_count,
                                             sizeof(*literal.variant_name_labels));
        literal.variant_payload_tags = calloc(literal.variant_count,
                                              sizeof(*literal.variant_payload_tags));
        if (!literal.variant_name_labels || !literal.variant_payload_tags) {
            free(copy);
            free(literal.generic_param_tags);
            free(literal.variant_name_labels);
            free(literal.variant_payload_tags);
            return NULL;
        }
    }

    {
        AsmByteLiteral *name_literal =
            ae_ensure_byte_literal(context, name, strlen(name), "typedesc");

        if (!name_literal) {
            free(copy);
            free(literal.generic_param_tags);
            free(literal.variant_name_labels);
            free(literal.variant_payload_tags);
            return NULL;
        }
        literal.name_label = name_literal->label;
    }

    for (i = 0; i < literal.variant_count; i++) {
        AsmByteLiteral *variant_literal;
        CalyndaRtTypeTag tag;
        char *variant_name;
        char *next = strchr(cursor, '|');

        if (next) {
            *next++ = '\0';
        }
        if (!ae_parse_variant_spec(cursor, &variant_name, &tag)) {
            free(copy);
            free(literal.generic_param_tags);
            free(literal.variant_name_labels);
            free(literal.variant_payload_tags);
            return NULL;
        }
        variant_literal =
            ae_ensure_byte_literal(context, variant_name, strlen(variant_name), "typedesc");
        if (!variant_literal) {
            free(copy);
            free(literal.generic_param_tags);
            free(literal.variant_name_labels);
            free(literal.variant_payload_tags);
            return NULL;
        }
        literal.variant_name_labels[i] = variant_literal->label;
        literal.variant_payload_tags[i] = tag;
        cursor = next;
    }

    literal.encoded = ae_copy_text(encoded);
    if (literal.generic_param_count > 0) {
        literal.generic_tags_label = ae_copy_format(".Ltd_gtags_%zu", context->next_label_id);
    }
    if (literal.variant_count > 0) {
        literal.variant_names_label =
            ae_copy_format(".Ltd_names_%zu", context->next_label_id);
        literal.variant_tags_label =
            ae_copy_format(".Ltd_tags_%zu", context->next_label_id);
    }
    literal.descriptor_label =
        ae_copy_format(".Ltd_desc_%zu", context->next_label_id++);
    if (!literal.encoded || !literal.descriptor_label ||
        (literal.generic_param_count > 0 && !literal.generic_tags_label) ||
        (literal.variant_count > 0 &&
         (!literal.variant_names_label || !literal.variant_tags_label)) ||
        !ae_reserve_items((void **)&context->type_descriptors,
                          &context->type_descriptor_capacity,
                          context->type_descriptor_count + 1,
                          sizeof(*context->type_descriptors))) {
        free(copy);
        free(literal.encoded);
        free(literal.generic_param_tags);
        free(literal.generic_tags_label);
        free(literal.variant_name_labels);
        free(literal.variant_payload_tags);
        free(literal.variant_names_label);
        free(literal.variant_tags_label);
        free(literal.descriptor_label);
        return NULL;
    }

    context->type_descriptors[context->type_descriptor_count] = literal;
    context->type_descriptor_count++;
    free(copy);
    return &context->type_descriptors[context->type_descriptor_count - 1];
}

bool ae_translate_type_descriptor_operand(AsmEmitContext *context,
                                          const char *operand_text,
                                          AsmOperand *operand) {
    AsmTypeDescriptorLiteral *literal;
    char *encoded;

    if (!context || !operand_text || !operand) {
        return false;
    }
    encoded = ae_between_parens(operand_text, "typedesc(");
    if (!encoded) {
        return false;
    }
    literal = ae_ensure_type_descriptor_literal(context, encoded);
    free(encoded);
    if (!literal) {
        return false;
    }
    operand->kind = ASM_OPERAND_ADDRESS;
    operand->text = ae_copy_text(literal->descriptor_label);
    return operand->text != NULL;
}

bool ae_emit_type_descriptors(FILE *out, const AsmEmitContext *context) {
    size_t i;

    if (!out || !context) {
        return false;
    }
    for (i = 0; i < context->type_descriptor_count; i++) {
        const AsmTypeDescriptorLiteral *literal = &context->type_descriptors[i];
        size_t j;

        if (literal->generic_param_count > 0 &&
            !ae_emit_type_tag_array(out, literal->generic_tags_label,
                                    literal->generic_param_tags,
                                    literal->generic_param_count)) {
            return false;
        }
        if (literal->variant_count > 0) {
            if (!ae_emit_line(out, "    .balign 8\n%s:\n", literal->variant_names_label)) {
                return false;
            }
            for (j = 0; j < literal->variant_count; j++) {
                if (!ae_emit_line(out, "    .quad %s\n", literal->variant_name_labels[j])) {
                    return false;
                }
            }
            if (!ae_emit_type_tag_array(out, literal->variant_tags_label,
                                        literal->variant_payload_tags,
                                        literal->variant_count)) {
                return false;
            }
        }
        if (!ae_emit_line(out,
                          "    .balign 8\n"
                          "%s:\n"
                          "    .quad %s\n"
                          "    .quad %zu\n"
                          "    .quad %s\n"
                          "    .quad %zu\n"
                          "    .quad %s\n"
                          "    .quad %s\n",
                          literal->descriptor_label,
                          literal->name_label,
                          literal->generic_param_count,
                          literal->generic_param_count > 0 ? literal->generic_tags_label : "0",
                          literal->variant_count,
                          literal->variant_count > 0 ? literal->variant_names_label : "0",
                          literal->variant_count > 0 ? literal->variant_tags_label : "0")) {
            return false;
        }
    }
    return true;
}