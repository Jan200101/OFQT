#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>

#define _DEBUG 1
#define Q_IsAbsolutePath V_IsAbsolutePath
#define Assert(f) assert(f)
#define CommandLine CommandLine_Tier0
#define Msg printf
#define V_strcpy_safe strcpy
#define Q_snprintf snprintf
#define _getcwd getcwd

typedef void* HMODULE;
typedef int Sys_Flags;
typedef void* CSysModule;

static bool (*V_IsAbsolutePath)(const char *pPath) = NULL;
static HMODULE (*Sys_LoadLibrary)(const char *pLibraryName, Sys_Flags flags) = NULL;
static char* (*CommandLine__ParmValue)(const char* param) = NULL;

CSysModule *Sys_LoadModule_replacement( const char *pModuleName, Sys_Flags flags /* = SYS_NOFLAGS (0) */ )
{
    // If using the Steam filesystem, either the DLL must be a minimum footprint
    // file in the depot (MFP) or a filesystem GetLocalCopy() call must be made
    // prior to the call to this routine.
    char szCwd[1024];
    HMODULE hDLL = NULL;

    if ( !Q_IsAbsolutePath( pModuleName ) )
    {
        char szAbsoluteModuleName[1024];

        // check if the sourcemod provides the library
        const char *pGameDir = CommandLine__ParmValue("-game");
        if ( pGameDir )
        {
            V_strcpy_safe( szCwd, pGameDir );
            if (szCwd[strlen(szCwd) - 1] == '/' || szCwd[strlen(szCwd) - 1] == '\\' )
            {
                szCwd[strlen(szCwd) - 1] = 0;
            }

            Q_snprintf( szAbsoluteModuleName, sizeof(szAbsoluteModuleName), "%s/bin/%s", szCwd, pModuleName );

            hDLL = Sys_LoadLibrary( szAbsoluteModuleName, flags );
        }

        // if the lib was not in the mod check the base game
        if (!hDLL)
        {
            // full path wasn't passed in, using the current working dir
            Assert( _getcwd( szCwd, sizeof( szCwd ) ) );

            if (szCwd[strlen(szCwd) - 1] == '/' || szCwd[strlen(szCwd) - 1] == '\\' )
            {
                szCwd[strlen(szCwd) - 1] = 0;
            }

            size_t cCwd = strlen( szCwd );
            if ( strstr( pModuleName, "bin/") == pModuleName || ( szCwd[ cCwd - 1 ] == 'n'  && szCwd[ cCwd - 2 ] == 'i' && szCwd[ cCwd - 3 ] == 'b' )  )
            {
                // don't make bin/bin path
                Q_snprintf( szAbsoluteModuleName, sizeof(szAbsoluteModuleName), "%s/%s", szCwd, pModuleName );          
            }
            else
            {
                Q_snprintf( szAbsoluteModuleName, sizeof(szAbsoluteModuleName), "%s/bin/%s", szCwd, pModuleName );
            }
            hDLL = Sys_LoadLibrary( szAbsoluteModuleName, flags );
        }
    }

    if ( !hDLL )
    {
        // full path failed, let LoadLibrary() try to search the PATH now
        hDLL = Sys_LoadLibrary( pModuleName, flags );
#if defined( _DEBUG )
        if ( !hDLL )
        {
            // So you can see what the error is in the debugger...
            Msg( "Failed to load %s: %s\n", pModuleName, dlerror() );
        }
#endif // DEBUG
    }

    return hDLL;
}


void __attribute__((constructor)) lib_entry()
{
    //It prints "Injected!" once the library gets loaded.
    fprintf(stderr, "Injected!\n");
}
