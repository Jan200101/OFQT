#ifndef INJECT_H
#define INJECT_H

#include <stdint.h>
#include <sys/types.h>

void* inject_syscall(pid_t pid, uintptr_t syscall_n, void*, void*, void*, void*, void*, void*);
int load_library(pid_t pid, char* lib_path);

#endif