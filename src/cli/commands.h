#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>

struct Command {
    char* name;
    int (*func)(int, char**);
    char* description;
};

extern const struct Command commands[];
extern const size_t commands_size;

#endif