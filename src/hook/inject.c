#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <unistd.h>
#include <elf.h>

#include "inject.h"
#include "memory.h"
#include "module.h"
#include "arch.h"

#define Elf64W(type) Elf64_ ## type
#define Elf32W(type) Elf32_ ## type

#define Elf64Ehdr Elf64W(Ehdr)
#define Elf32Ehdr Elf32W(Ehdr)

#define Elf64Shdr Elf64W(Shdr)
#define Elf32Shdr Elf32W(Shdr)

#define Elf64Sym Elf64W(Sym)
#define Elf32Sym Elf32W(Sym)

static uintptr_t getSymbolOffset(const char* elf_path, const char* symbol_name)
{
    unsigned char bits = getElfBits(elf_path);

    FILE* fd = fopen(elf_path, "rb");
    fseek(fd, 0, SEEK_END);
    size_t elf_size = (size_t)  ftell(fd);
    rewind(fd);

    char* elf_body = malloc(elf_size);
    fread(elf_body, 1, elf_size, fd);
    fclose(fd);

    if (bits == 64)
    {
        Elf64Ehdr* header = (Elf64Ehdr*)elf_body;

        Elf64Shdr* section = (Elf64Shdr*)(elf_body + header->e_shoff);
        Elf64Shdr* symtab_section = NULL;
        for (uintptr_t i = 0; i <= header->e_shnum; i++)
        {
            if (i == header->e_shnum)
                return 0;

            if (section[i].sh_type == SHT_SYMTAB)
            {
                symtab_section = section+i;
                break; 
            }
        }

        Elf64Sym* symtab = (Elf64Sym*)(elf_body + symtab_section->sh_offset);
        size_t symbol_num = symtab_section->sh_size / symtab_section->sh_entsize;
        char *symbol_names = (char *)(elf_body + section[symtab_section->sh_link].sh_offset);

        for (size_t j = 0; j < symbol_num; ++j)
        {
            char* name = symbol_names + symtab[j].st_name;
            size_t symbol_name_len = strlen(symbol_name);
            if (strncmp(name, symbol_name, symbol_name_len))
                continue;
            if (name[symbol_name_len] != '\0' && name[symbol_name_len] != '@')
                continue;

            if (symtab[j].st_value > 0)
            {
                uintptr_t value = symtab[j].st_value;
                free(elf_body);
                return value;
            }
        }
    }
    else if (bits == 32)
    {
        Elf32Ehdr* header = (Elf32Ehdr*)elf_body;

        Elf32Shdr* section = (Elf32Shdr*)(elf_body + header->e_shoff);
        Elf32Shdr* symtab_section = NULL;
        for (uintptr_t i = 0; i <= header->e_shnum; i++)
        {
            if (i == header->e_shnum)
                return 0;

            if (section[i].sh_type == SHT_SYMTAB)
            {
                symtab_section = section+i;
                break; 
            }
        }

        Elf32Sym* symtab = (Elf32Sym*)(elf_body + symtab_section->sh_offset);
        size_t symbol_num = symtab_section->sh_size / symtab_section->sh_entsize;
        char *symbol_names = (char *)(elf_body + section[symtab_section->sh_link].sh_offset);

        for (size_t j = 0; j < symbol_num; ++j)
        {
            char* name = symbol_names + symtab[j].st_name;
            size_t symbol_name_len = strlen(symbol_name);
            if (strncmp(name, symbol_name, symbol_name_len))
                continue;
            if (name[symbol_name_len] != '\0' && name[symbol_name_len] != '@')
                continue;

            if (symtab[j].st_value > 0)
            {
                uintptr_t value = symtab[j].st_value;
                free(elf_body);
                return value;
            }
        }
    }

    free(elf_body);

    return 0;
}

void* inject_syscall(
    pid_t pid,
    uintptr_t syscall_n,
    void* arg0,
    void* arg1,
    void* arg2,
    void* arg3,
    void* arg4,
    void* arg5
){
    void* ret = (void*)-1;
    int status;
    struct user_regs_struct old_regs, regs;
    void* injection_addr = (void*)-1;

    //This buffer is our payload, which will run a syscall properly on x86/x64
    unsigned char injection_buf[] =
    {
        0xff, 0xff, // placerholder
        /* these nops are here because
         * we're going to write memory using
         * ptrace, and it always writes the size
         * of a word, which means we have to make
         * sure the buffer is long enough
        */
        0x90, //nop
        0x90, //nop
        0x90, //nop
        0x90, //nop
        0x90, //nop
        0x90  //nop
    };

    unsigned char bits = getProcessBits(pid);
    if (bits == 64)
    {
        //syscall
        injection_buf[0] = 0x0f;
        injection_buf[1] = 0x05;   
    }
    else if (bits == 32)
    {
        //int80 (syscall)
        injection_buf[0] = 0xcd;
        injection_buf[1] = 0x80;
    }
    else
    {
        return NULL;
    }

    //As ptrace will always write a uintptr_t, let's make sure we're using proper buffers
    uintptr_t old_data;
    uintptr_t injection_buffer;
    memcpy(&injection_buffer, injection_buf, sizeof(injection_buffer));

    //Attach to process using 'PTRACE_ATTACH'
    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    wait(&status);

    /* Get the current registers using 'PTRACE_GETREGS' so that
     * we can restore the execution later
     * and also modify the bytes of EIP/RIP
    */

    ptrace(PTRACE_GETREGS, pid, NULL, &old_regs);
    regs = old_regs;

    //Now, let's set up the registers that will be injected into the tracee

#if defined(__i386__)
    regs.eax = (uintptr_t)syscall_n;
    regs.ebx = (uintptr_t)arg0;
    regs.ecx = (uintptr_t)arg1;
    regs.edx = (uintptr_t)arg2;
    regs.esi = (uintptr_t)arg3;
    regs.edi = (uintptr_t)arg4;
    regs.ebp = (uintptr_t)arg5;
    injection_addr = (void*)regs.eip;
#elif defined(__x86_64__)
    if (bits == 64)
    {
        regs.rax = (uintptr_t)syscall_n;
        regs.rdi = (uintptr_t)arg0;
        regs.rsi = (uintptr_t)arg1;
        regs.rdx = (uintptr_t)arg2;
        regs.r10 = (uintptr_t)arg3;
        regs.r8  = (uintptr_t)arg4;
        regs.r9  = (uintptr_t)arg5;
        injection_addr = (void*)regs.rip;
    }
    else if (bits == 32)
    {
        regs.rax = (uintptr_t)syscall_n;
        regs.rbx = (uintptr_t)arg0;
        regs.rcx = (uintptr_t)arg1;
        regs.rdx = (uintptr_t)arg2;
        regs.rsi = (uintptr_t)arg3;
        regs.rdi = (uintptr_t)arg4;
        regs.rbp = (uintptr_t)arg5;
        injection_addr = (void*)regs.rip;
    }
#endif

    //Let's store the buffer at EIP/RIP that we're going to modify into 'old_data' using 'PTRACE_PEEKDATA'
    old_data = (uintptr_t)ptrace(PTRACE_PEEKDATA, pid, injection_addr, NULL);

    //Let's write our payload into the EIP/RIP of the target process using 'PTRACE_POKEDATA'
    ptrace(PTRACE_POKEDATA, pid, injection_addr, injection_buffer);

    //Let's inject our modified registers into the target process using 'PTRACE_SETREGS'
    ptrace(PTRACE_SETREGS, pid, NULL, &regs);

    //Let's run a single step in the target process (execute one assembly instruction)
    ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
    waitpid(pid, &status, WSTOPPED); //Wait for the instruction to run

    //Let's get the registers after the syscall to store the return value
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
#if defined(__i386__)
    ret = (void*)regs.eax;
#elif defined(__x86_64__)
    ret = (void*)regs.rax;
#endif

    long long ret_int = (long long)ret;

    if (ret_int < 0)
        fprintf(stderr, "syscall error: %s\n", strerror((int)-ret_int));

    //Let's write the old data at EIP/RIP
    ptrace(PTRACE_POKEDATA, pid, (void*)injection_addr, old_data);

    //Let's restore the old registers to continue the normal execution
    ptrace(PTRACE_SETREGS, pid, NULL, &old_regs);
    ptrace(PTRACE_DETACH, pid, NULL, NULL); //Detach and continue the execution

    return ret;
}


/**
 * Return values:
 *  1 already loaded
 *  2 arch error
 */
int load_library(pid_t pid, char* lib_path)
{
    /* Let's get the address of the 'libc_dlopen_mode' of the target process
     * and store it on 'dlopen_ex' by loading the LIBC of the target process
     * on here and then getting the offset of its own '__libc_dlopen_mode'.
     * Then we sum this offset to the base of the external LIBC module
     */
    struct module_s lib_mod = getModule(pid, lib_path);
    if (lib_mod.size)
        return 1;

    struct module_s libc_ex = getModule(pid, "/libc.so");
    uintptr_t offset = getSymbolOffset(libc_ex.path, "__libc_dlopen_mode");

    // fallback
    if (!offset)
        offset = getSymbolOffset(libc_ex.path, "dlopen");

    fprintf(stderr, "%li\n", offset);

    //Get the external '__libc_dlopen_mode' by summing the offset to the libc_ex.base
    void* dlopen_ex = (void*)((uintptr_t)libc_ex.base + offset);

    freeModule(&libc_ex);

    //--- Now let's go to the injection part

    int status;
    struct user_regs_struct old_regs, regs;
    unsigned char inj_buf_x64[] =
    {
        /* On 'x64', we dont have to pass anything to the stack, as we're only
         * using 2 parameters, which will be stored on RDI (library path address) and
         * RSI (flags, in this case RTLD_LAZY).
         * This means we just have to call the __libc_dlopen_mode function, which
         * will be on RAX.
         */

        0xFF, 0xD0, //call rax
        0xCC,       //int3 (SIGTRAP)
    };

    unsigned char inj_buf_x86[] =
    {
        /* We have to pass the parameters to the stack (in reversed order)
         * The register 'ebx' will store the library path address and the
         * register 'ecx' will store the flag (RTLD_LAZY)
         * After pushing the parameters to the stack, we will call EAX, which
         * will store the address of '__libc_dlopen_mode'
         */
        0x51,       //push ecx
        0x53,       //push ebx
        0xFF, 0xD0, //call eax
        0xCC,       //int3 (SIGTRAP)
    };

    unsigned char* inj_buf = NULL;
    size_t sizeof_inj_buf = 0;

    unsigned char bits = getProcessBits(pid);
    if (bits == 64)
    {
        inj_buf = inj_buf_x64;
        sizeof_inj_buf = sizeof(inj_buf_x64);
    }
    else if (bits == 32)
    {
        inj_buf = inj_buf_x86;
        sizeof_inj_buf = sizeof(inj_buf_x86);
    }
    else
    {
        fprintf(stderr, "Could not figure out what injection buffer to use\n");
        return 2;
    }

    //Let's allocate memory for the payload and the library path
    size_t lib_path_len = strlen(lib_path) + 1;
    size_t inj_size = sizeof_inj_buf + lib_path_len;
    void* inj_addr = allocate_memory(pid, inj_size, PROT_EXEC | PROT_READ | PROT_WRITE);
    void* path_addr = (void*)((uintptr_t)inj_addr + sizeof_inj_buf);

    //Write the memory to our allocated address
    write_memory(pid, inj_addr, inj_buf, sizeof_inj_buf);
    write_memory(pid, path_addr, (void*)lib_path, lib_path_len);

    //Attach to the target process
    ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    wait(&status);

    //Get the current registers to restore later
    ptrace(PTRACE_GETREGS, pid, NULL, &old_regs);
    regs = old_regs;

    //Let's setup the registers according to our payload
#   if defined(__i386__)
    regs.eax = (long)dlopen_ex;
    regs.ebx = (long)path_addr;
    regs.ecx = (long)RTLD_LAZY;
    regs.eip = (long)inj_addr; //The execution will continue from 'inj_addr' (EIP)
#   elif defined(__x86_64__)
    if (bits == 64)
    {
        regs.rax = (uintptr_t)dlopen_ex;
        regs.rdi = (uintptr_t)path_addr;
        regs.rsi = (uintptr_t)RTLD_LAZY;
        regs.rip = (uintptr_t)inj_addr; //The execution will continue from 'inj_addr' (RIP)
    }
    else if (bits == 32)
    {
        regs.rax = (uintptr_t)dlopen_ex;
        regs.rbx = (uintptr_t)path_addr;
        regs.rcx = (uintptr_t)RTLD_LAZY;
        regs.rip = (uintptr_t)inj_addr; //The execution will continue from 'inj_addr' (RIP)
    }
#   endif

    //Inject the modified registers to the target process
    ptrace(PTRACE_SETREGS, pid, NULL, &regs);

    //Continue the execution
    ptrace(PTRACE_CONT, pid, NULL, NULL);

    //Wait for the int3 (SIGTRAP) breakpoint
    waitpid(pid, &status, WSTOPPED);

    //Set back the old registers
    ptrace(PTRACE_SETREGS, pid, NULL, &old_regs);

    //Detach from the process and continue the execution
    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    //Deallocate the memory we allocated for the injection buffer and the library path
    deallocate_memory(pid, inj_addr, inj_size);

    return 0;
}



