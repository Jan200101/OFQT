#ifndef TOAST_H
#define TOAST_H

#ifdef __cplusplus
extern "C" {
#endif

#define TOAST_TVN_DIR ".tvn"
#define TOAST_LOCAL_REMOTE "remote"
#define TOAST_LOCAL_REVISION "revision"
#define TOAST_LOCAL_OBJECTS "objects"
#define TOAST_LOCAL_REMOTE_PATH TOAST_TVN_DIR OS_PATH_SEP TOAST_LOCAL_REMOTE
#define TOAST_LOCAL_REVISION_PATH TOAST_TVN_DIR OS_PATH_SEP TOAST_LOCAL_REVISION
#define TOAST_LOCAL_OBJECTS_PATH TOAST_TVN_DIR OS_PATH_SEP TOAST_LOCAL_OBJECTS
#define OLD_TOAST_LOCAL_REVISION_PATH ".revision"

#define TOAST_OBJECTS_ENDPOINT "objects"
#define TOAST_REVISIONS_ENDPOINT "revisions"
#define TOAST_LATEST_ENDPOINT TOAST_REVISIONS_ENDPOINT "/latest"

#include <stddef.h>

enum TYPE_ENUM {
    TYPE_WRITE = 0,
    TYPE_MKDIR = 1,
    TYPE_DELETE = 2,
};

extern const char* TYPE_STRINGS[];

struct file_info {
    enum TYPE_ENUM type;
    char* path;
    char* hash;
    char* object;
};

struct revision_t {
    struct file_info* files;
    size_t file_count;
};

char* getToastDir(char* dir);
int getLatestRemoteRevision(char*);
int getLocalRevision(char*);
void setLocalRevision(char*, int);
char* getLocalRemote(char*);
void setLocalRemote(char*, char*);

struct revision_t* getRevisionData(char*, int);
struct revision_t* fastFowardRevisions(char* url, int from, int to);
void freeRevision(struct revision_t* rev);

size_t downloadObject(char*, char*, struct file_info*);
int applyObject(char*, struct file_info*);
void removeObjects(char*);

int verifyFileHash(char*, struct file_info*);

#ifdef __cplusplus
}
#endif

#endif