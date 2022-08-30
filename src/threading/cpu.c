#include <sys/sysinfo.h>

int get_core_count()
{
	return get_nprocs();
}
