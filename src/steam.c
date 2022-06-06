#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#include "steam.h"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

/**
 * Returns a heap allocated path to the main steam directory
 * If a problem occurs returns NULL
 */
char* getSteamDir()
{
#if defined(__linux__)
    char* home = getenv("HOME");

    if (!home || !isDir(home))
        return NULL;

    size_t len = strlen(home) + strlen(FLATPAK_DIR);
    char* path = malloc(len+1);

    strncpy(path, home, len);
    strncat(path, STEAM_DIR, len-strlen(path));

    if (isDir(path))
        return path;

    strncpy(path, home, len);
    strncat(path, FLATPAK_DIR, len-strlen(path));

    if (isDir(path))
        return path;

    free(path);
#elif defined(_WIN32)
    size_t size = PATH_MAX;
    char* path = malloc(size+1);
    if (!path)
        return NULL;

    LSTATUS res = RegGetValueA(HKEY_LOCAL_MACHINE, REG_PATH, "InstallPath", RRF_RT_REG_SZ, NULL, path, (LPDWORD)&size);

    if (res == ERROR_SUCCESS && isDir(path))
        return path;
    
    strncpy(path, STEAM_PGRM_64, size);
    path[size] = '\0';

    if (isDir(path))
        return path;

    strncpy(path, STEAM_PGRM_86, size);
    path[size] = '\0';

    if (isDir(path))
        return path;

    free(path);

#else
    #error No Implementation
#endif
    return NULL;
}

/**
 * Returns a heap allocated path to the sourcemod dirctory
 * If a problem occurs returns NULL
 */
char* getSourcemodDir()
{
    char* steam = getSteamDir();
    if (!steam)
        return NULL;

    steam = realloc(steam, strlen(steam) + strlen(OS_PATH_SEP) + strlen(SOURCEMOD_DIR) + 1);
    strcat(steam, OS_PATH_SEP);
    strcat(steam, SOURCEMOD_DIR);

    return steam;
}

char* getOpenFortressDir()
{
    char* sm_dir = getSourcemodDir();
    if (!sm_dir)
        return NULL;

    sm_dir = realloc(sm_dir, strlen(sm_dir) + strlen(OPEN_FORTRESS_DIR) + 1);
    strcat(sm_dir, OPEN_FORTRESS_DIR);

    return sm_dir;
}

/**
 * function to fetch the PID of a running Steam process.
 * If none were found returns -1
 */
long getSteamPID()
{
#ifdef __linux__
    long pid;
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

            if (stat && (fscanf(stat, "%li (%[^)])", &pid, buf)) == 2)
            {                
                if (!strcmp(buf, STEAM_PROC))
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
        if (!strcmp(pe32.szExeFile, STEAM_PROC))
            return (long)pe32.th32ProcessID;
    }
#else
    #error No Implementation
#endif
    return -1;
}

#define STEAM_LAUNCH

int runOpenFortress()
{
#ifdef STEAM_LAUNCH
    #ifdef _WIN32
        return system("start steam://rungameid/11677091221058336806");
    #else
        return system("xdg-open steam://rungameid/11677091221058336806");
    #endif

#else
    char* of_dir = getOpenFortressDir();
    char* steam = getSteamDir();
    steam = realloc(steam, strlen(steam) + strlen(OS_PATH_SEP) + strlen(STEAM_BIN) + 1);
    strcat(steam, OS_PATH_SEP);
    strcat(steam, STEAM_BIN);

    return execl(steam, steam, "-applaunch", "243750", "-game", of_dir, "-secure", "-steam", NULL);
#endif
}
