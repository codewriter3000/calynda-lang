    new_gtag = CALYNDA_RT_TYPE_RAW_WORD;
    memset(&new_desc, 0, sizeof(new_desc));
    new_desc.name = old_arr->type_desc ? old_arr->type_desc->name : "arr";
    new_desc.generic_param_count = 1;
    new_desc.generic_param_tags = &new_gtag;
    new_desc.variant_count = n + 1;
    new_desc.variant_payload_tags = new_vtags;

    result = __calynda_rt_hetero_array_new(&new_desc, n + 1, new_elems);
    free(new_elems);
    free(new_vtags);
    return result;
}

CalyndaRtWord __calynda_rt_hetero_array_cdr(CalyndaRtWord arr_word) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(arr_word);
    const CalyndaRtHeteroArray *old_arr;
    CalyndaRtTypeDescriptor new_desc;
    CalyndaRtTypeTag new_gtag;
    CalyndaRtTypeTag *new_vtags;
    CalyndaRtWord *new_elems;
    size_t n;
    CalyndaRtWord result;

    if (!header || header->kind != CALYNDA_RT_OBJECT_HETERO_ARRAY) {
        fprintf(stderr, "runtime: __calynda_rt_hetero_array_cdr expects a hetero array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    old_arr = (const CalyndaRtHeteroArray *)(const void *)header;
    n = old_arr->count;

    if (n == 0) {
        fprintf(stderr, "runtime: cdr() called on an empty hetero array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    if (n == 1) {
        /* Return an empty hetero array */
        memset(&new_desc, 0, sizeof(new_desc));
        new_desc.name = old_arr->type_desc ? old_arr->type_desc->name : "arr";
        return __calynda_rt_hetero_array_new(&new_desc, 0, NULL);
    }

    new_elems = calloc(n - 1, sizeof(*new_elems));
    if (!new_elems) {
        fprintf(stderr, "runtime: out of memory in hetero_array_cdr\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    if (old_arr->elements) {
        memcpy(new_elems, old_arr->elements + 1, (n - 1) * sizeof(*new_elems));
    }

    new_vtags = calloc(n - 1, sizeof(*new_vtags));
    if (!new_vtags) {
        free(new_elems);
        fprintf(stderr, "runtime: out of memory in hetero_array_cdr\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    if (old_arr->type_desc && old_arr->type_desc->variant_payload_tags) {
        memcpy(new_vtags, old_arr->type_desc->variant_payload_tags + 1,
               (n - 1) * sizeof(*new_vtags));
    }

    new_gtag = CALYNDA_RT_TYPE_RAW_WORD;
    memset(&new_desc, 0, sizeof(new_desc));
    new_desc.name = old_arr->type_desc ? old_arr->type_desc->name : "arr";
    new_desc.generic_param_count = 1;
    new_desc.generic_param_tags = &new_gtag;
    new_desc.variant_count = n - 1;
    new_desc.variant_payload_tags = new_vtags;

    result = __calynda_rt_hetero_array_new(&new_desc, n - 1, new_elems);
    free(new_elems);
    free(new_vtags);
    return result;
}CalyndaRtWord __calynda_rt_hetero_array_prepend(CalyndaRtWord element, CalyndaRtWord arr_word) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(arr_word);
    const CalyndaRtHeteroArray *old_arr;
    CalyndaRtTypeDescriptor new_desc;
    CalyndaRtTypeTag new_gtag;
    CalyndaRtTypeTag *new_vtags;
    CalyndaRtWord *new_elems;
    size_t n;
    CalyndaRtWord result;

    if (!header || header->kind != CALYNDA_RT_OBJECT_HETERO_ARRAY) {
        fprintf(stderr, "runtime: __calynda_rt_hetero_array_prepend expects a hetero array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    old_arr = (const CalyndaRtHeteroArray *)(const void *)header;
    n = old_arr->count;

    new_elems = calloc(n + 1, sizeof(*new_elems));
    if (!new_elems) {
        fprintf(stderr, "runtime: out of memory in hetero_array_prepend\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    new_elems[0] = element;
    if (n > 0 && old_arr->elements) {
        memcpy(new_elems + 1, old_arr->elements, n * sizeof(*new_elems));
    }

    new_vtags = calloc(n + 1, sizeof(*new_vtags));
    if (!new_vtags) {
        free(new_elems);
        fprintf(stderr, "runtime: out of memory in hetero_array_prepend\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    new_vtags[0] = rt_tag_of_word(element);
    if (n > 0 && old_arr->type_desc && old_arr->type_desc->variant_payload_tags) {
        memcpy(new_vtags + 1, old_arr->type_desc->variant_payload_tags, n * sizeof(*new_vtags));
    }

    new_gtag = CALYNDA_RT_TYPE_RAW_WORD;
    memset(&new_desc, 0, sizeof(new_desc));
    new_desc.name = old_arr->type_desc ? old_arr->type_desc->name : "arr";
    new_desc.generic_param_count = 1;
    new_desc.generic_param_tags = &new_gtag;
    new_desc.variant_count = n + 1;
    new_desc.variant_payload_tags = new_vtags;

    result = __calynda_rt_hetero_array_new(&new_desc, n + 1, new_elems);
    free(new_elems);
    free(new_vtags);
    return result;
}
