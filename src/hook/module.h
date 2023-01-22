#ifndef MODULE_H
#define MODULE_H

#include <sys/types.h>
#include <stdint.h>

struct module_s
{
    char* name;
    char* path;
    void* base;
    void* end;
    uintptr_t size;
    void* handle; //this will not be used for now, only internally with dlopen
};

struct module_s getModule(pid_t pid, const char* module_name);
void freeModule(struct module_s* mod);

#endif