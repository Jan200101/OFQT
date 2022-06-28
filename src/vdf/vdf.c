#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "vdf.h"

#define CHAR_SPACE ' '
#define CHAR_TAB '\t'
#define CHAR_NEWLINE '\n'
#define CHAR_DOUBLE_QUOTE '"'
#define CHAR_OPEN_CURLY_BRACKET '{'
#define CHAR_CLOSED_CURLY_BRACKET '}'
#define CHAR_FRONTSLASH '/'
#define CHAR_BACKSLASH '\\'

#define FMT_UNKNOWN_CHAR "Encountered Unknown Character %c (%li)\n"


struct vdf_object* vdf_parse_buffer(const char* buffer, size_t size)
{
    if (!buffer)
        return NULL;

    struct vdf_object* root_object = malloc(sizeof(struct vdf_object));
    root_object->key = NULL;
    root_object->parent = NULL;
    root_object->type = VDF_TYPE_NONE;

    struct vdf_object* o = root_object;

    const char* head = buffer;
    const char* tail = head;

    const char* end = buffer + size;

    const char* buf = NULL;

    while (end > tail)
    {
        switch (*tail)
        {
            case CHAR_DOUBLE_QUOTE:
                if (tail > buffer && *(tail-1) == CHAR_BACKSLASH)
                    break;

                if (!buf)
                {
                    buf = tail+1;
                }
                else if (o->key)
                {
                    size_t len = tail - buf;
                    size_t digits = 0;
                    size_t chars = 0;

                    for (size_t i = 0; i < len; ++i)
                    {
                        if (isdigit(buf[i]))
                            digits++;

                        if (isalpha(buf[i]))
                            chars++;
                    }

                    if (len && digits == len)
                    {
                        o->type = VDF_TYPE_INT;
                    }
                    else if (len > 6 && len < 9 && (chars+digits) == len-1 && buf[0] == '#')
                    {
                        // TODO
                        //o->type = VDF_TYPE_COLOR;
                        o->type = VDF_TYPE_STRING;
                    }
                    else
                    {
                        o->type = VDF_TYPE_STRING;
                    }

                    switch (o->type)
                    {
                        case VDF_TYPE_INT:
                            o->data.data_int = strtol(buf, NULL, 10);
                            break;

                        case VDF_TYPE_STRING:
                            o->data.data_string.str = malloc(len+1);
                            o->data.data_string.len = len;
                            strncpy(o->data.data_string.str, buf, len);
                            o->data.data_string.str[len] = '\0';
                            break;

                        default:
                            assert(0);
                            break;
                    }

                    buf = NULL;

                    if (o->parent && o->parent->type == VDF_TYPE_ARRAY)
                    {
                        o = o->parent;
                        assert(o->type == VDF_TYPE_ARRAY);

                        o->data.data_array.len++;
                        o->data.data_array.data_value = realloc(o->data.data_array.data_value, (sizeof(void*)) * (o->data.data_array.len + 1));
                        o->data.data_array.data_value[o->data.data_array.len] = malloc(sizeof(struct vdf_object)),
                        o->data.data_array.data_value[o->data.data_array.len]->parent = o;

                        o = o->data.data_array.data_value[o->data.data_array.len];
                        o->key = NULL;
                        o->type = VDF_TYPE_NONE;
                    }
                }
                else
                {
                    size_t len = tail - buf;
                    o->key = malloc(len+1);
                    strncpy(o->key, buf, len);
                    o->key[len] = '\0';
                    buf = NULL;
                }
                break;

            case CHAR_OPEN_CURLY_BRACKET:
                assert(!buf);
                assert(o->type == VDF_TYPE_NONE);

                if (o->parent && o->parent->type == VDF_TYPE_ARRAY)
                    o->parent->data.data_array.len++;

                o->type = VDF_TYPE_ARRAY;
                o->data.data_array.len = 0;
                o->data.data_array.data_value = malloc((sizeof(void*)) * (o->data.data_array.len + 1));
                o->data.data_array.data_value[o->data.data_array.len] = malloc(sizeof(struct vdf_object)),
                o->data.data_array.data_value[o->data.data_array.len]->parent = o;

                o = o->data.data_array.data_value[o->data.data_array.len];
                o->key = NULL;
                o->type = VDF_TYPE_NONE;
                break;

            case CHAR_CLOSED_CURLY_BRACKET:
                assert(!buf);

                o = o->parent;
                assert(o);
                if (o->parent)
                {
                    o = o->parent;
                    assert(o->type == VDF_TYPE_ARRAY);

                    o->data.data_array.data_value = realloc(o->data.data_array.data_value, (sizeof(void*)) * (o->data.data_array.len + 1));
                    o->data.data_array.data_value[o->data.data_array.len] = malloc(sizeof(struct vdf_object)),
                    o->data.data_array.data_value[o->data.data_array.len]->parent = o;

                    o = o->data.data_array.data_value[o->data.data_array.len];
                    o->key = NULL;
                    o->type = VDF_TYPE_NONE;
                }

                break;

            case CHAR_FRONTSLASH:
                if (!buf)
                    while (*tail != '\0' && *tail != CHAR_NEWLINE)
                        ++tail;

                break;

            default:
            case CHAR_NEWLINE:
            case CHAR_SPACE:
            case CHAR_TAB:
                break;
        }
        ++tail;
    }
    return root_object;
}

struct vdf_object* vdf_parse_file(const char* path)
{
    struct vdf_object* o = NULL;
    if (!path)
        return o;

    FILE* fd = fopen(path, "r");
    if (!fd)
        return o;

    fseek(fd, 0L, SEEK_END);
    size_t file_size = ftell(fd);
    rewind(fd);

    if (file_size)
    {
        char* buffer = malloc(file_size);
        fread(buffer, sizeof(*buffer), file_size, fd);

        o = vdf_parse_buffer(buffer, file_size);
        free(buffer);
    }

    fclose(fd);

    return o;
}


size_t vdf_object_get_array_length(struct vdf_object* o)
{
    assert(o);
    assert(o->type == VDF_TYPE_ARRAY);

    return o->data.data_array.len;
}

struct vdf_object* vdf_object_index_array(struct vdf_object* o, size_t index)
{
    assert(o);
    assert(o->type == VDF_TYPE_ARRAY);
    assert(o->data.data_array.len > index);

    return o->data.data_array.data_value[index];
}

struct vdf_object* vdf_object_index_array_str(struct vdf_object* o, char* str)
{
    assert(o);
    assert(str);
    assert(o->type == VDF_TYPE_ARRAY);

    for (size_t i = 0; i < o->data.data_array.len; ++i)
    {
        struct vdf_object* k = o->data.data_array.data_value[i];
        if (!strcmp(k->key, str))
            return k;
    }
    return NULL;
}

const char* vdf_object_get_string(struct vdf_object* o)
{
    assert(o->type == VDF_TYPE_STRING);

    return o->data.data_string.str;
}

int vdf_object_get_int(struct vdf_object* o)
{
    assert(o->type == VDF_TYPE_INT);

    return o->data.data_int;
}

static void vdf_print_object_indent(struct vdf_object* o, int l)
{
    if (!o)
        return;

    char* spacing = "\t";

    for (int k = 0; k < l; ++k)
        printf("%s", spacing);

    printf("\"%s\"", o->key);
    switch (o->type)
    {
        case VDF_TYPE_ARRAY:
            puts("");
            for (int k = 0; k < l; ++k)
                printf("%s", spacing);
            puts("{");
            for (size_t i = 0; i < o->data.data_array.len; ++i)
                vdf_print_object_indent(o->data.data_array.data_value[i], l+1);

            for (int k = 0; k < l; ++k)
                printf("%s", spacing);
            puts("}");
            break;

        default:
        case VDF_TYPE_NONE:
        case VDF_TYPE_FLOAT:
        case VDF_TYPE_PTR: // ?
        case VDF_TYPE_COLOR:
            assert(0);
            break;

        case VDF_TYPE_INT:
            printf("\t\t\"%i\"\n", o->data.data_int);
            break;

        case VDF_TYPE_STRING:
            printf("\t\t\"%s\"\n", o->data.data_string.str);
            break;

        case VDF_TYPE_WSTRING:
            assert(0);
            break;

    }
}

void vdf_print_object(struct vdf_object* o)
{
    vdf_print_object_indent(o, 0);
}

void vdf_free_object(struct vdf_object* o)
{
    if (!o)
        return;

    switch (o->type)
    {
        case VDF_TYPE_ARRAY:
            for (size_t i = 0; i <= o->data.data_array.len; ++i)
            {
                vdf_free_object(o->data.data_array.data_value[i]);
            }
            free(o->data.data_array.data_value);
            break;

        default:
        case VDF_TYPE_NONE:
        case VDF_TYPE_INT:
        case VDF_TYPE_FLOAT:
        case VDF_TYPE_PTR: // ?
        case VDF_TYPE_COLOR:
            break;

        case VDF_TYPE_STRING:
            if (o->data.data_string.str)
                free(o->data.data_string.str);
            break;

        case VDF_TYPE_WSTRING:
            if (o->data.data_wstring.str)
                free(o->data.data_wstring.str);
            break;

    }

    if (o->key)
        free(o->key);
    free(o);
}
