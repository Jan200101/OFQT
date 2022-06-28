/**
 * a command line utility that parses a given
 * vdf file and indexes requested values by key
 *
 * Returns 0 if value is a string otherwise 1
 *
 * Used for testing to ensure indexing works reliably
 * on a known file
 */
#include <stdio.h>

#include "vdf.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        puts("vdf_index file index [index...]");
        return 1;
    }

    struct vdf_object* o = vdf_parse_file(argv[1]);

    if (!o)
    {
        puts("Invalid File");
        return 1;
    }

    struct vdf_object* k = o;
    for (int i = 2; i < argc; ++i)
    {
        k = vdf_object_index_array_str(k, argv[i]);
        if (!k)
        {
            printf("Invalid index '%s'\n", argv[i]);
            break;
        }
    }

    int retval = k->type != VDF_TYPE_STRING;

    if (retval)
        vdf_print_object(k);
    else
        puts(k->data.data_string.str);

    vdf_free_object(o);

    return retval;
}