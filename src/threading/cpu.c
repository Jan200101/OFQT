#ifdef _WIN32
#include <windows.h>
#else
#include <sys/sysinfo.h>
#endif

int get_core_count()
{
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );

    return sysinfo.dwNumberOfProcessors;
#else
    return get_nprocs();
#endif
}
