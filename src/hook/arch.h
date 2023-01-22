#ifndef ARCH_H
#define ARCH_H

#include <sys/types.h>

unsigned char getProcessBits(pid_t pid);
unsigned char getElfBits(const char* path);

#endif