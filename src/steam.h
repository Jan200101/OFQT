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
#define STEAM_PROC "steam"
#define STEAM_BIN "steam.sh"
#endif

#define STEAM_APPID "243750"

#ifdef _WIN32
#define _STEAM_NAME "Steam"
#define STEAM_PGRM_64 "C:\\Program Files (x86)\\" _STEAM_NAME
#define STEAM_PGRM_86 "C:\\Program Files\\" _STEAM_NAME
// TODO check if this is the right registry path for x86
#define REG_PATH "SOFTWARE\\Wow6432Node\\Valve\\Steam"
#else // _WIN32
#define STEAM_DIR "/.local/share/Steam"
#define FLATPAK_DIR "/.var/app/com.valvesoftware.Steam" STEAM_DIR
#endif

#define SOURCEMOD_DIR "steamapps" OS_PATH_SEP "sourcemods" OS_PATH_SEP
#define OPEN_FORTRESS_DIR "open_fortress"

char* getSteamDir();
char* getSourcemodDir();
char* getOpenFortressDir();
long getSteamPID();
int runOpenFortress();

#ifdef __cplusplus
}
#endif

#endif