#include <stdio.h>
#include <limits.h>
#include <sys/types.h>

#include "arch.h"

unsigned char getProcessBits(pid_t pid)
{
    char exe_path[PATH_MAX];
    snprintf(exe_path, sizeof(exe_path), "/proc/%i/exe", pid);

    return getElfBits(exe_path);
}

unsigned char getElfBits(const char* path)
{
    if (!path)
        return 0;

    FILE* fd = fopen(path, "rb");
    if (!fd)
        return 0;

    char indent[5];
    fread(indent, sizeof(*indent), sizeof(indent), fd);
    fclose(fd);

    if (indent[0] != 0x7f)
        return 0;

    if (indent[1] != 'E' ||
        indent[2] != 'L' ||
        indent[3] != 'F')
        return 0;

    if (indent[4] == 1)
        return 32;
    else if (indent[4] == 2)
        return 64;

    return 0;
}