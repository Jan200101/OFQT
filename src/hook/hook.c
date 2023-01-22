#include <stdio.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <string.h>

#include "hook.h"
#include "inject.h"
#include "memory.h"
#include "module.h"

#include "payloads/libSysLoadLibrary_so.h"

// defined in tvn:proc.c
extern pid_t getPid(const char*);

static void writeToDisk(const char* path, const char* content, const size_t size)
{
    FILE* fd = fopen(path, "wb");
    fwrite(content, sizeof(*content), size, fd);
    fclose(fd);
}

int fix_SysLoadLibary()
{
    pid_t pid = getPid("loop_forever");
    printf("%i\n", pid);
    if (pid != -1)
    {
        char* path = "/tmp/SysLoadLibrary.so";

        fprintf(stderr, "[~] writing library to disk\n");
        writeToDisk(path, libSysLoadLibrary_so, libSysLoadLibrary_so_size);

        //inject_syscall(pid, 1, (void*)21, NULL, NULL, NULL, NULL, NULL); 

        fprintf(stderr, "[~] loading library into process\n");
        int ret = load_library(pid, path);  
        if (!ret)
            fprintf(stderr, "[*] Success\n");
        else if (ret == 1)
            fprintf(stderr, "[!] library already loaded\n");
        else
            fprintf(stderr, "[!] could not load libary\n");
    }
    return 0;
}