#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

#include "proc.h"

pid_t getPid(const char* process_name)
{
#if defined(__linux__) || defined(__FreeBSD__)

#ifdef __FreeBSD__
    // on FreeBSD /proc is not mounted by default
    if (!isDir("/proc"))
        return -1;
#endif

    pid_t pid;
    char buf[PATH_MAX];
    struct dirent* ent;
    DIR* proc = opendir("/proc");
    FILE* stat;

    if (proc)
    {
        while ((ent = readdir(proc)) != NULL)
        {
            long lpid = atol(ent->d_name);
            if (!lpid) continue;

            snprintf(buf, sizeof(buf), "/proc/%ld/stat", lpid);
            stat = fopen(buf, "r");

            if (stat && (fscanf(stat, "%i (%[^)])", &pid, buf)) == 2)
            {                
                if (!strcmp(buf, process_name))
                {
                    fclose(stat);
                    closedir(proc);
                    return pid;
                }
                fclose(stat);
            }
        }

        closedir(proc);
    }

#elif _WIN32
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32 = {0};
    pe32.dwSize = sizeof(PROCESSENTRY32); 
    Process32First(hSnap,&pe32);

    while(Process32Next(hSnap,&pe32))
    {
        if (!strcmp(pe32.szExeFile, process_name))
            return (long)pe32.th32ProcessID;
    }
#else
    #error No Implementation
#endif
    return -1;
}
