#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#include "vdf.h"
#include "steam.h"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

/**
 * Returns a heap allocated path to the main steam directory
 * If a problem occurs returns NULL
 */
char* getSteamDir(void)
{
#if defined(__linux__) || defined(__FreeBSD__)
    char* home = getenv("HOME");

    if (!home || !isDir(home))
        return NULL;

    size_t len = strlen(home);
#ifdef FLATPAK_DIR
    len += strlen(FLATPAK_DIR);
#else
    len += strlen(STEAM_DIR);
#endif
    char* path = malloc(len+1);

    strncpy(path, home, len);
    strncat(path, STEAM_DIR, len-strlen(path));

    if (isDir(path))
        return path;

#ifdef FLATPAK_DIR
    strncpy(path, home, len);
    strncat(path, FLATPAK_DIR, len-strlen(path));

    if (isDir(path))
        return path;
#endif

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
char* getSourcemodDir(void)
{
    char* steam = getSteamDir();
    if (!steam)
        return NULL;

    steam = realloc(steam, strlen(steam) + strlen(OS_PATH_SEP) + strlen(SOURCEMOD_DIR) + 1);
    strcat(steam, OS_PATH_SEP);
    strcat(steam, SOURCEMOD_DIR);

    return steam;
}

char* getOpenFortressDir(void)
{
    char* sm_dir = getSourcemodDir();
    if (!sm_dir)
        return NULL;

    sm_dir = realloc(sm_dir, strlen(sm_dir) + strlen(OS_PATH_SEP) + strlen(OPEN_FORTRESS_DIR) + 1);
    strcat(sm_dir, OS_PATH_SEP);
    strcat(sm_dir, OPEN_FORTRESS_DIR);

    return sm_dir;
}

char* getSourceSDK2013MpDir(void)
{
    char* librayfolders = getSteamDir();
    if (!librayfolders)
        return NULL;

    librayfolders = realloc(librayfolders, strlen(librayfolders) + strlen(OS_PATH_SEP) + strlen(STEAMAPPS) + strlen(OS_PATH_SEP) + strlen(SOURCESDK_MANIFEST) + 1);

    strcat(librayfolders, OS_PATH_SEP);
    strcat(librayfolders, STEAMAPPS);
    strcat(librayfolders, OS_PATH_SEP);
    strcat(librayfolders, LIBARYFOLDERS_VDF);

    struct vdf_object* o = vdf_parse_file(librayfolders);
    free(librayfolders);

    char* sdkdir = NULL;

    if (o)
    {
        for (size_t i = 0; i < vdf_object_get_array_length(o); ++i)
        {
            struct vdf_object* library = vdf_object_index_array(o, i);
            struct vdf_object* apps = vdf_object_index_array_str(library, "apps");

            if (vdf_object_index_array_str(apps, SOURCESDK_APPID))
            {
                struct vdf_object* path = vdf_object_index_array_str(library, "path");

                size_t path_len = path->data.data_string.len + strlen(OS_PATH_SEP) + strlen(STEAMAPPS) + strlen(OS_PATH_SEP) + strlen(SOURCESDK_MANIFEST) + 1;
                char* path_str = malloc(path_len);

                snprintf(path_str, path_len, "%s%s%s%s%s", vdf_object_get_string(path), OS_PATH_SEP, STEAMAPPS, OS_PATH_SEP, SOURCESDK_MANIFEST);

                struct vdf_object* k = vdf_parse_file(path_str);
                free(path_str);

                if (k)
                {
                    struct vdf_object* installdir = vdf_object_index_array_str(k, "installdir");

                    path_len = path->data.data_string.len + strlen(OS_PATH_SEP) + strlen(STEAMAPPS_COMMON) + strlen(OS_PATH_SEP) + installdir->data.data_string.len + 1;
                    path_str = malloc(path_len);
                    snprintf(path_str, path_len, "%s%s%s%s%s", vdf_object_get_string(path), OS_PATH_SEP, STEAMAPPS_COMMON, OS_PATH_SEP, vdf_object_get_string(installdir));

                    sdkdir = path_str;

                    vdf_free_object(k);
                }
            }
        }

        vdf_free_object(o);
    }

    return sdkdir;
}

/**
 * function to fetch the PID of a running Steam process.
 * If none were found returns -1
 */
long getSteamPID(void)
{
#if defined(__linux__) || defined(__FreeBSD__)

#ifdef __FreeBSD__
    // on FreeBSD /proc is not mounted by default
    if (!isDir("/proc"))
        return -1;
#endif

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

int runOpenFortress(char** args, size_t arg_count)
{
#ifdef STEAM_DIRECT_LAUNCH
#if defined(__linux__) || defined(__FreeBSD__)
    // fork so we don't have to stay alive for the game
    if (fork()) return 0;
#endif

    char* game = getSourceSDK2013MpDir();
    if (!game)
        exit(0);

    char* of_dir = getOpenFortressDir();
    if (!of_dir)
    {
        free(game);
        exit(0);
    }

    game = realloc(game, strlen(game) + strlen(OS_PATH_SEP) + strlen(HL2_EXE) + 1);
    strcat(game, OS_PATH_SEP);
    strcat(game, HL2_EXE);

#if defined(__linux__) || defined(__FreeBSD__)
    // we need to be in the steam environment to get the right locales 
    setenv("SteamEnv", "1", 1);
#endif

    char** argv = malloc(sizeof(char*) * (arg_count + 6));

#ifdef _WIN32
    size_t of_dir_len = strlen(of_dir); 
    of_dir = realloc(of_dir, of_dir_len + 3);
    memmove(of_dir+1, of_dir, of_dir_len);
    of_dir[0] = '"';
    of_dir[of_dir_len+1] = '"';
    of_dir[of_dir_len+2] = '\0';
#endif

    argv[0] = game;
    argv[1] = "-game";
    argv[2] = of_dir;
    argv[3] = "-secure";
    argv[4] = "-steam";
    for (size_t i = 0; i < arg_count; ++i)
        argv[5+i] = args[i];
    argv[5+arg_count] = NULL;

    execv(game, argv);

    free(game);
    free(of_dir);

    exit(0);

#else
    #ifdef STEAM_NAIVE_LAUNCH
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

        char** argv = malloc(sizeof(char*) * (arg_count + 8));

#ifdef _WIN32
    size_t of_dir_len = strlen(of_dir); 
    of_dir = realloc(of_dir, of_dir_len + 3);
    memmove(of_dir+1, of_dir, of_dir_len);
    of_dir[0] = '"';
    of_dir[of_dir_len+1] = '"';
    of_dir[of_dir_len+2] = '\0';
#endif

        argv[0] = steam;
        argv[1] = "-applaunch";
        argv[2] = "243750";
        argv[3] = "-game";
        argv[4] = of_dir;
        argv[5] = "-secure";
        argv[6] = "-steam";
        for (size_t i = 0; i < arg_count; ++i)
            argv[7+i] = args[i];
        argv[7+arg_count] = NULL;

        return execv(steam, argv);
    #endif
#endif
}
    