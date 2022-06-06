#ifndef FS_H
#define FS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define OS_PATH_SEP "\\"
#else
#define OS_PATH_SEP "/"
#endif

int isFile(const char*);
int isDir(const char*);

int makeDir(const char*);
int removeDir(const char*);

#ifdef __cplusplus
}
#endif

#endif