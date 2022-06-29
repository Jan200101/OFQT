#ifndef STEAM_H
#define STEAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "fs.h"

#ifdef _WIN32
#define STEAM_PROC "steam.exe"
#define STEAM_BIN OS_PATH_SEP STEAM_PROC
#else
// Works on Linux and FreeBSD
#define STEAM_PROC "steam"
#define STEAM_BIN "steam.sh"
#endif

#define LIBARYFOLDERS_VDF "libraryfolders.vdf"
#define SOURCESDK_APPID "243750"
#define SOURCESDK_MANIFEST "appmanifest_" SOURCESDK_APPID ".acf"

#if defined(_WIN32)
#define HL2_EXE "hl2.exe"
#define STEAM_PGRM_64 "C:\\Program Files (x86)\\Steam"
#define STEAM_PGRM_86 "C:\\Program Files\\Steam"
// TODO check if this is the right registry path for x86
#define REG_PATH "SOFTWARE\\Wow6432Node\\Valve\\Steam"

#elif defined(__linux__)
#define HL2_EXE "hl2.sh"
#define STEAM_DIR "/.local/share/Steam"
#define FLATPAK_DIR "/.var/app/com.valvesoftware.Steam" STEAM_DIR

#elif defined(__FreeBSD__)
#define HL2_EXE "hl2.sh"
#define STEAM_DIR "/.steam/steam"

#endif

#define STEAMAPPS "steamapps"
#define STEAMAPPS_COMMON STEAMAPPS OS_PATH_SEP "common"
#define SOURCEMOD_DIR STEAMAPPS OS_PATH_SEP "sourcemods"
#define OPEN_FORTRESS_DIR "open_fortress"

#if defined(_WIN32)
#else
#endif

char* getSteamDir();
char* getSourcemodDir();
char* getOpenFortressDir();
char* getSourceSDK2013MpDir();
long getSteamPID();
int runOpenFortress(char**, size_t);

#ifdef __cplusplus
}
#endif

#endif