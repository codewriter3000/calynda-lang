#include "runtime_internal.h"

const char *calynda_rt_object_kind_name(CalyndaRtObjectKind kind) {
    switch (kind) {
    case CALYNDA_RT_OBJECT_STRING:
        return "string";
    case CALYNDA_RT_OBJECT_ARRAY:
        return "array";
    case CALYNDA_RT_OBJECT_CLOSURE:
        return "closure";
    case CALYNDA_RT_OBJECT_PACKAGE:
        return "package";
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE:
        return "extern-callable";
    case CALYNDA_RT_OBJECT_UNION:
        return "union";
    case CALYNDA_RT_OBJECT_HETERO_ARRAY:
        return "hetero-array";
    }

    return "unknown";
}

const char *calynda_rt_type_tag_name(CalyndaRtTypeTag tag) {
    switch (tag) {
    case CALYNDA_RT_TYPE_VOID:
        return "void";
    case CALYNDA_RT_TYPE_BOOL:
        return "bool";
    case CALYNDA_RT_TYPE_INT32:
        return "int32";
    case CALYNDA_RT_TYPE_INT64:
        return "int64";
    case CALYNDA_RT_TYPE_STRING:
        return "string";
    case CALYNDA_RT_TYPE_ARRAY:
        return "array";
    case CALYNDA_RT_TYPE_CLOSURE:
        return "closure";
    case CALYNDA_RT_TYPE_EXTERNAL:
        return "external";
    case CALYNDA_RT_TYPE_RAW_WORD:
        return "raw-word";
    case CALYNDA_RT_TYPE_UNION:
        return "union";
    case CALYNDA_RT_TYPE_HETERO_ARRAY:
        return "hetero-array";
    }

    return "unknown";
}

bool calynda_rt_dump_layout(FILE *out) {
    if (!out) {
        return false;
    }

    fprintf(out,
            "RuntimeLayout word=uint64 raw-scalar-or-object-handle\n"
            "  ObjectHeader size=%zu magic=0x%08X fields=[magic:uint32, kind:uint32]\n"
            "  String size=%zu payload=[length:size_t, bytes:char*]\n"
            "  Array size=%zu payload=[count:size_t, elements:uint64*]\n"
            "  Closure size=%zu payload=[entry:void*, capture_count:size_t, captures:uint64*]\n"
            "  Package size=%zu payload=[name:char*]\n"
            "  ExternCallable size=%zu payload=[kind:uint32, name:char*]\n"
            "  TemplatePart size=%zu payload=[tag:uint64, payload:uint64]\n"
            "  TemplateTags text=%d value=%d\n"
            "  Union size=%zu payload=[type_desc:TypeDescriptor*, tag:uint32, payload:uint64]\n"
            "  HeteroArray size=%zu payload=[type_desc:TypeDescriptor*, count:size_t, elements:uint64*]\n"
            "  TypeDescriptor fields=[name:char*, generic_param_count:size_t, generic_param_tags:uint32*, variant_count:size_t, variant_names:char**, variant_payload_tags:uint32*]\n"
            "  Builtins package=stdlib member=print\n",
            sizeof(CalyndaRtWord),
            CALYNDA_RT_OBJECT_MAGIC,
            sizeof(CalyndaRtString),
            sizeof(CalyndaRtArray),
            sizeof(CalyndaRtClosure),
            sizeof(CalyndaRtPackage),
            sizeof(CalyndaRtExternCallable),
            sizeof(CalyndaRtTemplatePart),
            CALYNDA_RT_TEMPLATE_TAG_TEXT,
            CALYNDA_RT_TEMPLATE_TAG_VALUE,
            sizeof(CalyndaRtUnion),
            sizeof(CalyndaRtHeteroArray));
    return !ferror(out);
}

char *calynda_rt_dump_layout_to_string(void) {
    FILE *stream;
    char *buffer;
    long size;
    size_t read_size;

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }

    if (!calynda_rt_dump_layout(stream) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }

    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}
