    if (!symbols || !name) {
        return 0;
    }
    root = symbol_table_root_scope(symbols);
    sym = scope_lookup_local(root, name);
    if (!sym || sym->kind != SYMBOL_KIND_LAYOUT || !sym->declaration) {
        return 0;
    }
    layout = (const AstLayoutDecl *)sym->declaration;
    for (i = 0; i < layout->field_count; i++) {
        const AstType *ft = &layout->fields[i].field_type;
        if (ft->kind == AST_TYPE_PRIMITIVE) {
            total += hr_primitive_byte_size(ft->primitive);
        } else {
            total += 8; /* fallback word size for non-primitive field types */
        }
    }
    return total;
}
