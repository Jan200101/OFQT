#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net.h"
#include "commands.h"

static void help(void)
{
    fprintf(stderr, "OFCL <command>\n"); \

    size_t longestStr;
    size_t length;

    if (commands_size)
    {
        longestStr = 0;

        for (size_t i = 0; i < commands_size; ++i)
        {
            length = strlen(commands[i].name);

            if (length > longestStr) longestStr = length;
        }

        fprintf(stderr, "\nList of commands:\n");
        for (size_t i = 0; i < commands_size; ++i)
        {
            fprintf(stderr, "\t%-*s\t %s\n", (int)longestStr, commands[i].name, commands[i].description);
        }
    }
}

int main(int argc, char** argv)
{
    net_init();
    atexit(net_deinit);

    for (int i = 1; i < argc; ++i)
    {
        for (size_t j = 0; j < commands_size; ++j)
        {
            if (!strcmp(commands[j].name, argv[i]))
                return commands[j].func(argc-i, argv+i);
        }
    }

    help();
    return EXIT_SUCCESS;
}