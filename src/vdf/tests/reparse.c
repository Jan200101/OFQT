/**
 * a command line utility that parses a given
 * vdf file and and output it pretty printed
 * onto stdout or a specified file
 *
 * Used for testing to ensure we parse a VDF
 * file correctly and can output the same
 */
#include <stdio.h>

#include "vdf.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        puts("vdf_reparse file [output]");
        return 1;
    }
    else if (argc > 2)
    {
        freopen(argv[2], "w", stdout);
    }

    struct vdf_object* o = vdf_parse_file(argv[1]);

    if (!o)
    {
        puts("Invalid File");
        return 1;
    }
    vdf_print_object(o);

    vdf_free_object(o);
}