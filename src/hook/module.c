#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>

#include "module.h"
#include "arch.h"

#define fprintf(...)

#define NOTFOUND (size_t)(-1)
#define PROCMAPS_LINE_MAX_LENGTH (PATH_MAX + 100) 

static void* strtoptr(pid_t pid, const char* nptr, char** endptr, int base)
{
    unsigned char bits = getProcessBits(pid);
    if (bits == 64)
        return (void*)strtoull(nptr, endptr, base);
    else if (bits == 32)
        return (void*)strtoul(nptr, endptr, base);

    return NULL;

}
static size_t find(const char* a, const char* b, uintptr_t offset)
{
    const char* pos = a + offset;

    while (*pos && strncmp(pos, b, strlen(b)))
        ++pos;

    if (!*pos)
        return NOTFOUND;

    return (size_t)(pos - a);
}

static size_t rfind(const char* a, const char* b, size_t offset)
{
    const char* pos = a + (strlen(a) - strlen(b) - offset);

    while (a <= pos && strncmp(pos, b, strlen(b)))
        --pos;

    if (pos < a)
        return NOTFOUND;

    return (size_t)(pos - a);
}

static char* substr(const char* str, size_t pos, size_t len)
{
    str = str + pos;

    if (len == NOTFOUND)
        len = strlen(str);

    char* buf = malloc(len+1);
    strncpy(buf, str, len);
    buf[len] = '\0';

    return buf;
}

struct module_s getModule(pid_t pid, const char* module_name)
{
    struct module_s mod = {0};

    char maps_file_path[PATH_MAX];
    snprintf(maps_file_path, sizeof(maps_file_path), "/proc/%i/maps", pid);
    fprintf(stderr, "Maps file path: %s\n", maps_file_path);


    FILE* maps_file_fs = fopen(maps_file_path, "rb");
    if (!maps_file_fs) return mod;

    char maps_file[PROCMAPS_LINE_MAX_LENGTH];
    while( !feof(maps_file_fs) ){
        if (fgets(maps_file, sizeof(maps_file), maps_file_fs) == NULL){
            fprintf(stderr,"fgets failed, %s\n",strerror(errno));
            return mod;
        }
        size_t module_path_pos = 0;
        size_t module_path_end = 0;

        //Get the first slash in the line of the module name
        module_path_pos = find(maps_file, module_name, 0);
        if (module_path_pos == NOTFOUND)
            continue;

        module_path_pos = find(maps_file, "/", 0);

        //Get the end of the line of the module name
        module_path_end = find(maps_file, "\n", module_path_pos);

        if(module_path_pos == NOTFOUND || module_path_end == NOTFOUND) continue;

        //Module path substring
        char* module_path_str = substr(maps_file, module_path_pos, module_path_end - module_path_pos);

        fprintf(stderr, "Module path string: %s\n", module_path_str);

        //--- Module name

        char* module_name_str = substr(module_path_str,
            rfind(module_path_str, "/", 0) + 1, //Substring from the last '/' to the end of the string
            NOTFOUND
        );

        fprintf(stderr, "Module name: %s\n", module_name_str);

        //--- Base Address

        size_t base_address_pos = rfind(maps_file, "\n", module_path_pos) + 1;
        size_t base_address_end = find(maps_file, "-", base_address_pos);
        if(base_address_pos == NOTFOUND || base_address_end == NOTFOUND) continue;
        char* base_address_str = substr(maps_file, base_address_pos, base_address_end - base_address_pos);
        void* base_address = (void*)strtoptr(pid, base_address_str, NULL, 16);;
        if (!base_address) return mod;

        fprintf(stderr, "Base Address: %p\n", base_address);

        //--- End Address
        size_t end_address_pos;
        size_t end_address_end;
        char* end_address_str;
        void* end_address;

        //Get end address pos
        end_address_pos = rfind(maps_file, module_path_str, 0);
        end_address_pos = rfind(maps_file, "\n", end_address_pos) + 1;
        end_address_pos = find(maps_file, "-", end_address_pos) + 1;

        //Find first space from end_address_pos
        end_address_end = find(maps_file, " ", end_address_pos);

        if(end_address_pos == NOTFOUND || end_address_end == NOTFOUND) continue;

        //End address substring
        end_address_str = substr(maps_file, end_address_pos, end_address_end - end_address_pos);
        end_address = (void*)strtoptr(pid, end_address_str, NULL, 16);
        free(end_address_str);

        fprintf(stderr, "End Address: %p\n", end_address);

        //--- Module size

        uintptr_t module_size = (uintptr_t)end_address - (uintptr_t)base_address;
        fprintf(stderr, "End Address: %li\n", module_size);

        mod.name = module_name_str;
        mod.path = module_path_str;
        mod.base = base_address;
        mod.size = module_size;
        mod.end  = end_address;

        break;
    }

    //--- Module Path


    //---

    //Now we put all the information we got into the mod structure


    fclose(maps_file_fs);

    return mod;
}

void freeModule(struct module_s* mod)
{
	free(mod->name);
	free(mod->path);
}
