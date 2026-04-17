#include "runtime_internal.h"

static const CalyndaRtTypeDescriptor *rt_copy_hetero_array_descriptor(
    const CalyndaRtTypeDescriptor *type_desc,
    size_t element_count) {
    CalyndaRtTypeDescriptor *copied_desc;
    CalyndaRtTypeTag *stored_generic_tags = NULL;
    CalyndaRtTypeTag *stored_tags = NULL;

    if (!type_desc || type_desc->variant_count != element_count) {
        return NULL;
    }
    copied_desc = calloc(1, sizeof(*copied_desc));
    if (!copied_desc) {
        return NULL;
    }
    *copied_desc = *type_desc;
    if (type_desc->generic_param_count > 0) {
        stored_generic_tags = calloc(type_desc->generic_param_count, sizeof(*stored_generic_tags));
        if (!stored_generic_tags || !type_desc->generic_param_tags) {
            free(stored_generic_tags);
            free(copied_desc);
            return NULL;
        }
        memcpy(stored_generic_tags,
               type_desc->generic_param_tags,
               type_desc->generic_param_count * sizeof(*stored_generic_tags));
        copied_desc->generic_param_tags = stored_generic_tags;
    }
    if (element_count > 0) {
        stored_tags = calloc(element_count, sizeof(*stored_tags));
        if (!stored_tags) {
            free(stored_generic_tags);
            free(copied_desc);
            return NULL;
        }
        if (!type_desc->variant_payload_tags) {
            free(stored_generic_tags);
            free(stored_tags);
            free(copied_desc);
            return NULL;
        }
        memcpy(stored_tags,
               type_desc->variant_payload_tags,
               element_count * sizeof(*stored_tags));
        copied_desc->variant_payload_tags = stored_tags;
    }
    return copied_desc;
}

static void rt_free_hetero_array_descriptor(const CalyndaRtTypeDescriptor *type_desc) {
    if (!type_desc) {
        return;
    }
    free((CalyndaRtTypeTag *)type_desc->generic_param_tags);
    free((CalyndaRtTypeTag *)type_desc->variant_payload_tags);
    free((CalyndaRtTypeDescriptor *)type_desc);
}

CalyndaRtWord __calynda_rt_union_new(const CalyndaRtTypeDescriptor *type_desc,
                                     uint32_t variant_tag,
                                     CalyndaRtWord payload) {
    CalyndaRtUnion *union_object;

    union_object = calloc(1, sizeof(*union_object));
    if (!union_object) {
        fprintf(stderr, "runtime: out of memory while creating union value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    union_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    union_object->header.kind = CALYNDA_RT_OBJECT_UNION;
    union_object->type_desc = type_desc;
    union_object->tag = variant_tag;
    union_object->payload = payload;
    if (!rt_register_object_pointer(union_object)) {
        free(union_object);
        fprintf(stderr, "runtime: out of memory while registering union object\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    return rt_make_object_word(union_object);
}

uint32_t __calynda_rt_union_get_tag(CalyndaRtWord value) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(value);

    if (!header || header->kind != CALYNDA_RT_OBJECT_UNION) {
        fprintf(stderr, "runtime: attempted tag access on non-union value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return ((const CalyndaRtUnion *)(const void *)header)->tag;
}

CalyndaRtWord __calynda_rt_union_get_payload(CalyndaRtWord value) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(value);

    if (!header || header->kind != CALYNDA_RT_OBJECT_UNION) {
        fprintf(stderr, "runtime: attempted payload access on non-union value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return ((const CalyndaRtUnion *)(const void *)header)->payload;
}

bool __calynda_rt_union_check_tag(CalyndaRtWord value, uint32_t expected_tag) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(value);

    if (!header || header->kind != CALYNDA_RT_OBJECT_UNION) {
        return false;
    }

    return ((const CalyndaRtUnion *)(const void *)header)->tag == expected_tag;
}

CalyndaRtWord __calynda_rt_hetero_array_new(const CalyndaRtTypeDescriptor *type_desc,
                                            size_t element_count,
                                            const CalyndaRtWord *elements) {
    CalyndaRtHeteroArray *array_object;

    array_object = calloc(1, sizeof(*array_object));
    if (!array_object) {
        fprintf(stderr, "runtime: out of memory while creating hetero array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    array_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    array_object->header.kind = CALYNDA_RT_OBJECT_HETERO_ARRAY;
    array_object->type_desc = rt_copy_hetero_array_descriptor(type_desc, element_count);
    if (!array_object->type_desc) {
        free(array_object);
        fprintf(stderr, "runtime: out of memory while creating hetero array metadata\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }
    array_object->count = element_count;
    if (element_count > 0) {
        array_object->elements = calloc(element_count, sizeof(*array_object->elements));
        if (!array_object->elements) {
            free(array_object->elements);
            rt_free_hetero_array_descriptor(array_object->type_desc);
            free(array_object);
            fprintf(stderr, "runtime: out of memory while creating hetero array elements\n");
            rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
        }
        if (elements) {
            memcpy(array_object->elements, elements, element_count * sizeof(*elements));
        }
    }
    if (!rt_register_object_pointer(array_object)) {
        free(array_object->elements);
        rt_free_hetero_array_descriptor(array_object->type_desc);
        free(array_object);
        fprintf(stderr, "runtime: out of memory while registering hetero array\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_OOM);
    }

    return rt_make_object_word(array_object);
}

CalyndaRtTypeTag __calynda_rt_hetero_array_get_tag(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);
    const CalyndaRtHeteroArray *arr;
    size_t offset;

    if (!header || header->kind != CALYNDA_RT_OBJECT_HETERO_ARRAY) {
        fprintf(stderr, "runtime: attempted hetero-array tag access on non-hetero-array value\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    arr = (const CalyndaRtHeteroArray *)(const void *)header;
    offset = (size_t)(int64_t)index;
    if (offset >= arr->count) {
        fprintf(stderr, "runtime: hetero-array tag index out of bounds (%zu)\n", offset);
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    if (!arr->type_desc || !arr->type_desc->variant_payload_tags ||
        offset >= arr->type_desc->variant_count) {
        fprintf(stderr, "runtime: hetero-array metadata was missing for tag lookup\n");
        rt_fatal_now(CALYNDA_RT_EXIT_RUNTIME_ERROR);
    }

    return arr->type_desc->variant_payload_tags[offset];
}
