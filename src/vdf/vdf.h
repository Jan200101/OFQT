#ifndef VDF_H
#define VDF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

enum vdf_data_type
{
    VDF_TYPE_NONE,
    VDF_TYPE_ARRAY,
    VDF_TYPE_STRING,
    VDF_TYPE_INT,
    VDF_TYPE_FLOAT,
    VDF_TYPE_PTR,
    VDF_TYPE_WSTRING,
    VDF_TYPE_COLOR
};

struct vdf_object;

union vdf_data {
    struct {
        struct vdf_object** data_value;
        size_t len;
    } data_array;

    struct {
        char* str;
        size_t len;
    } data_string;

    int data_int;
    float data_float;

    void* data_ptr; // TYPE?

    struct {
        wchar_t* str;
        size_t len;
    } data_wstring;

    uint32_t color; // RGBA
};

struct vdf_object
{
    char* key;
    struct vdf_object* parent;

    enum vdf_data_type type;
    union vdf_data data;
};

struct vdf_object* vdf_parse_file(const char* path);

size_t vdf_object_get_array_length(struct vdf_object* o);
struct vdf_object* vdf_object_index_array(struct vdf_object* o, size_t index);
struct vdf_object* vdf_object_index_array_str(struct vdf_object* o, char* str);

const char* vdf_object_get_string(struct vdf_object* o);

int vdf_object_get_int(struct vdf_object* o);

void vdf_print_object(struct vdf_object* o);
void vdf_free_object(struct vdf_object* o);

#ifdef __cplusplus
}
#endif

#endif