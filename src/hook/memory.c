#define _GNU_SOURCE

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <errno.h>

#include "memory.h"
#include "inject.h"
#include "arch.h"

#define mmap_x64 9
#define munmap_x64 11
#define mmap2_x86 192
#define munmap_x86 91

void read_memory(pid_t pid, void* src, void* dst, size_t size)
{
    /*
    pid  = target process id
    src  = address to read from on the target process
    dst  = address to write to on the caller process
    size = size of the buffer that will be read
    */

    struct iovec iosrc;
    struct iovec iodst;
    iodst.iov_base = dst;
    iodst.iov_len  = size;
    iosrc.iov_base = src;
    iosrc.iov_len  = size;

    if (process_vm_readv(pid, &iodst, 1, &iosrc, 1, 0) == -1)
        fprintf(stderr, "process_vm_readv: %s\n", strerror(errno));
}

void write_memory(pid_t pid, void* dst, void* src, size_t size)
{
    /*
    pid  = target process id
    dst  = address to write to on the target process
    src  = address to read from on the caller process
    size = size of the buffer that will be read
    */

    struct iovec iosrc;
    struct iovec iodst;
    iosrc.iov_base = src;
    iosrc.iov_len  = size;
    iodst.iov_base = dst;
    iodst.iov_len  = size;

    if(process_vm_writev(pid, &iosrc, 1, &iodst, 1, 0) == -1)
        fprintf(stderr, "process_vm_writev: %s\n", strerror(errno));
}

void* allocate_memory(pid_t pid, size_t size, int protection)
{
    //mmap template:
    //void *mmap (void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset);

    void* ret = (void*)-1;

    unsigned char bits = getProcessBits(pid);

    if (bits == 32)
    {
        ret = inject_syscall(
            pid,
            mmap2_x86,
            //arguments
            (void*)0,
            (void*)size,
            (void*)(uintptr_t)protection,
            (void*)(MAP_ANON | MAP_PRIVATE),
            (void*)-1,
            (void*)0
        );
    }
    else if (bits == 64)
    {
        ret = inject_syscall(
            pid,
            mmap_x64,
            //arguments
            (void*)0,
            (void*)size,
            (void*)(uintptr_t)protection,
            (void*)(MAP_ANON | MAP_PRIVATE),
            (void*)-1,
            (void*)0
        );
    }

    return ret;
}

void deallocate_memory(pid_t pid, void* src, size_t size)
{
    unsigned char bits = getProcessBits(pid);
    if (bits == 64)
        inject_syscall(pid, munmap_x64, src, (void*)size, NULL, NULL, NULL, NULL);
    else if (bits == 32)
        inject_syscall(pid, munmap_x86, src, (void*)size, NULL, NULL, NULL, NULL);
}

void* protect_memory(pid_t pid, void* src, size_t size, int protection)
{
    //mprotect template
    //int mprotect (void *__addr, size_t __len, int __prot);
    return inject_syscall(pid, __NR_mprotect, src, (void*)size, (void*)(uintptr_t)protection, NULL, NULL, NULL);
}
