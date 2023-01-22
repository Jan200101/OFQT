#ifndef MEMORY_H
#define MEMORY_H 

#include <sys/types.h>

void read_memory(pid_t pid, void* src, void* dst, size_t size);
void write_memory(pid_t pid, void* dst, void* src, size_t size);

void* allocate_memory(pid_t pid, size_t size, int protection);
void deallocate_memory(pid_t pid, void* src, size_t size);
void* protect_memory(pid_t pid, void* src, size_t size, int protection);

#endif