#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <json.h>
#include <md5.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "fs.h"
#include "net.h"
#include "toast.h"

#ifdef TOAST_DEFAULT_REMOTE
#define TOAST_DEFAULT_REMOTE "http://toast.openfortress.fun/toast"
#endif

#define TVN_DIR ".tvn"
#define LOCAL_REMOTE "remote"
#define LOCAL_REVISION "revision"
#define LOCAL_OBJECTS "objects"
#define LOCAL_REMOTE_PATH TVN_DIR OS_PATH_SEP LOCAL_REMOTE
#define LOCAL_REVISION_PATH TVN_DIR OS_PATH_SEP LOCAL_REVISION
#define LOCAL_OBJECTS_PATH TVN_DIR OS_PATH_SEP LOCAL_OBJECTS
#define OLD_LOCAL_REVISION_PATH ".revision"

#define OBJECTS_ENDPOINT "objects"
#define REVISIONS_ENDPOINT "revisions"
#define LATEST_ENDPOINT REVISIONS_ENDPOINT "/latest"

const char* TYPE_STRINGS[] = {
    "Add/Modify",
    "Create Directory",
    "Delete"
};

static int fileHash(char*, char*);

char* getToastDir(char* dir)
{
    if (!dir)
        return NULL;

    size_t len = strlen(dir) + strlen(OS_PATH_SEP) + strlen(TVN_DIR) + 1;
    char* tvn_dir = malloc(len + 1);
    if (!tvn_dir)
        return NULL;

    tvn_dir[0] = '\0';
    snprintf(tvn_dir, len, "%s%s%s", dir, OS_PATH_SEP, TVN_DIR);

    if (!isDir(tvn_dir))
    {
        makeDir(tvn_dir);
#if defined(_WIN32)
        SetFileAttributesA(tvn_dir, FILE_ATTRIBUTE_HIDDEN);
#endif
    }


    return tvn_dir;
}

/**
 * Returns a the latest Revision available on the server.
 * If none could be fetched returns -1.
 */
int getLatestRemoteRevision(char* url)
{
    assert(url);
    int revision = -1;
    size_t len = strlen(url) + 1 + strlen(LATEST_ENDPOINT)+1;

    char* latest_url = malloc(len);
    if (!latest_url)
        return revision;

    snprintf(latest_url, len, "%s/%s", url, LATEST_ENDPOINT);

    struct MemoryStruct* latest_data = downloadToRam(latest_url);
    free(latest_url);

    if (latest_data)
    {
        latest_data->memory = realloc(latest_data->memory, latest_data->size+1);
        latest_data->memory[latest_data->size] = '\0';

        revision = atoi((char*)latest_data->memory);
        freeDownload(latest_data);
    }

    return revision;
}

/**
 * Returns the current revision the local install is on
 * If none could be found returns -1.
 */
int getLocalRevision(char* dir)
{
    int revision = -1;

    if (!dir || !isDir(dir))
        return revision;

    size_t len = strlen(dir) + strlen(OS_PATH_SEP) + strlen(LOCAL_REVISION_PATH);
    char* revision_path = malloc(len + 1);
    if (!revision_path)
        return revision;

    // LEGACY BEHAVIOR
    strncpy(revision_path, dir, len);
    strncat(revision_path, OS_PATH_SEP, len - strlen(revision_path));
    strncat(revision_path, OLD_LOCAL_REVISION_PATH, len - strlen(revision_path));

    if (!isFile(revision_path))
    {
        revision_path[len-strlen(LOCAL_REVISION_PATH)] = '\0';
        strncat(revision_path, LOCAL_REVISION_PATH, len - strlen(revision_path));
        if (!isFile(revision_path))
        {
            free(revision_path);
            return revision;
        }
    }

    FILE* fd = fopen(revision_path, "r");
    free(revision_path);

    if (!fd)
        return revision;

    fseek(fd, 0L, SEEK_END);
    size_t fd_size = (size_t)ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    char* buf = malloc(fd_size+1);
    fread(buf, sizeof(char), fd_size, fd);
    buf[fd_size] = '\0';

    fclose(fd);

    revision = atoi(buf);
    free(buf);

    return revision;
}

/**
 * Write a revision number to disk
 */
void setLocalRevision(char* dir, int rev)
{
    char* revision_path = getToastDir(dir);
    if (!revision_path)
        return;

    // cleanup legacy behavior
    {    
        char* old_revision_path = malloc(strlen(revision_path) + strlen(OLD_LOCAL_REVISION_PATH));
        strcpy(old_revision_path, revision_path);

        char* op = old_revision_path + strlen(old_revision_path);
        while (*op != *OS_PATH_SEP) --op;
        *++op = '\0';

        strcat(old_revision_path, OLD_LOCAL_REVISION_PATH);

        if (isFile(old_revision_path))
            remove(old_revision_path);

        free(old_revision_path);
    }

    size_t len = strlen(revision_path) + strlen(OS_PATH_SEP) + strlen(LOCAL_REVISION) + 1;
    revision_path = realloc(revision_path, len);

    if (!isDir(revision_path))
        makeDir(revision_path);

    strncat(revision_path, OS_PATH_SEP, len - strlen(revision_path));
    strncat(revision_path, LOCAL_REVISION, len - strlen(revision_path));

    size_t rev_len = 1 + 1;
    {

        int rev_copy = rev;
        while (rev_copy >= 10)
        {
            rev_copy /= 10;
            rev_len++;
        }
    }
    char* revision_string = malloc(rev_len);
    if (!revision_string)
        return;

    snprintf(revision_string, rev_len, "%i", rev);

    FILE* fd = fopen(revision_path, "w");
    free(revision_path);
    if (!fd)
        return;

    fwrite(revision_string, sizeof(char), rev_len-1, fd);
    fclose(fd);

    free(revision_string);
}

/**
 * Get the remote URL for a local install
 * If remote cannot be found returns TOAST_DEFAULT_REMOTE
 *
 * Results need to be freed
 */
char* getLocalRemote(char* dir)
{
    if (!dir || !isDir(dir))
        return strdup(TOAST_DEFAULT_REMOTE);

    size_t len = strlen(dir) + strlen(OS_PATH_SEP) +  strlen(LOCAL_REMOTE_PATH);
    char* remote_path = malloc(len + 1);
    if (!remote_path)
        return strdup(TOAST_DEFAULT_REMOTE);

    strncpy(remote_path, dir, len);
    strncat(remote_path, OS_PATH_SEP, len - strlen(remote_path));
    strncat(remote_path, LOCAL_REMOTE_PATH, len - strlen(remote_path));

    if (!isFile(remote_path))
    {
        free(remote_path);
        return strdup(TOAST_DEFAULT_REMOTE);
    }

    FILE* fd = fopen(remote_path, "rb");
    free(remote_path);

    if (!fd)
        return strdup(TOAST_DEFAULT_REMOTE);

    fseek(fd, 0L, SEEK_END);
    size_t fd_size = (size_t)ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    char* buf = malloc(fd_size+1);
    fread(buf, sizeof(char), fd_size, fd);
    buf[fd_size] = '\0';

    char* bufp = buf;
    while (*bufp != '\0')
    {
        if (*bufp == '\n')
            *bufp = '\0';
        ++bufp;
    }
    if (buf[strlen(buf)] == '/')
        buf[strlen(buf)] = '\0';

    fclose(fd);

    return buf;
}

/**
 * Write a remote URL to disk
 */
void setLocalRemote(char* dir, char* remote)
{
    char* remote_path = getToastDir(dir);
    if (!remote_path)
        return;

    size_t len = strlen(remote_path) + strlen(OS_PATH_SEP) + strlen(LOCAL_REMOTE) + 1;
    remote_path = realloc(remote_path, len);

    if (!isDir(remote_path))
        makeDir(remote_path);

    strncat(remote_path, OS_PATH_SEP, len - strlen(remote_path));
    strncat(remote_path, LOCAL_REMOTE, len - strlen(remote_path));

    FILE* fd = fopen(remote_path, "w");
    free(remote_path);

    fwrite(remote, sizeof(char), strlen(remote), fd);

    fclose(fd);
}

/**
 * Gets revision data at the specified API url for the specified revision number
 * Returns NULL on error
 *
 * Results need to be freed
 */
struct revision_t* getRevisionData(char* url, int rev)
{
    if (!url)
        return NULL;

    size_t rev_len = 1;
    {

        int rev_copy = rev;
        while (rev_copy > 10)
        {
            rev_copy /= 10;
            rev_len++;
        }
    }

    size_t len = strlen(url) + 1 + strlen(REVISIONS_ENDPOINT) + 1 + rev_len + 1;
    char* buf = malloc(len);
    snprintf(buf, len, "%s/%s/%i", url, REVISIONS_ENDPOINT, rev);

    struct json_object* revision_list = fetchJSON(buf);
    free(buf);

    if (!revision_list)
        return NULL;

    struct revision_t* revision = malloc(sizeof(struct revision_t));
    if (!revision)
    {
        json_object_put(revision_list);
        return NULL;
    }

    revision->file_count = (size_t)json_object_array_length(revision_list);
    revision->files = malloc(sizeof(struct file_info) * revision->file_count);

    struct json_object* temp;
    for (size_t i = 0; i < revision->file_count; ++i)
    {
        struct json_object* file = json_object_array_get_idx(revision_list, i);

        json_object_object_get_ex(file, "type", &temp);
        assert(temp);
        revision->files[i].type = json_object_get_int(temp);

        json_object_object_get_ex(file, "path", &temp);
        assert(temp);
        revision->files[i].path = strdup(json_object_get_string(temp));

        json_object_object_get_ex(file, "hash", &temp);
        if (temp)
            revision->files[i].hash = strdup(json_object_get_string(temp));
        else
            revision->files[i].hash = NULL;

        json_object_object_get_ex(file, "object", &temp);
        if (temp)
            revision->files[i].object = strdup(json_object_get_string(temp));
        else
            revision->files[i].object = NULL;
    }

    json_object_put(revision_list);

    return revision;
}

/**
 * Get all revisions between two numbers and merges them together as far as possible
 * Returns NULL on error
 *
 * Results need to be freed
 */
struct revision_t* fastFowardRevisions(char* url, int from, int to)
{
    struct revision_t* rev = NULL;

    for (int rev_num = from; rev_num <= to; ++rev_num)
    {
        struct revision_t* cur_rev = getRevisionData(url, rev_num);

        if (!rev)
        {
            rev = cur_rev;
            continue;
        }

        for (size_t j = 0; j < cur_rev->file_count; ++j)
        {
            for (size_t i = 0; i < rev->file_count; ++i)
            {
                if (!strcmp(rev->files[i].path, cur_rev->files[j].path))
                {
                    rev->files[i].type = cur_rev->files[j].type;

                    if (rev->files[i].hash)
                        free(rev->files[i].hash);

                    if (cur_rev->files[j].hash)
                        rev->files[i].hash = strdup(cur_rev->files[j].hash);
                    else
                        rev->files[i].hash = NULL;

                    if (rev->files[i].object)
                        free(rev->files[i].object);

                    if (cur_rev->files[j].object)
                        rev->files[i].object = strdup(cur_rev->files[j].object);
                    else
                        rev->files[i].object = NULL;
                }
            }

            rev->files = realloc(rev->files, sizeof(struct file_info) * (rev->file_count+1));

            rev->files[rev->file_count].type = cur_rev->files[j].type;

            if (cur_rev->files[j].path)
                rev->files[rev->file_count].path = strdup(cur_rev->files[j].path);
            else
                rev->files[rev->file_count].path = NULL;


            if (cur_rev->files[j].hash)
                rev->files[rev->file_count].hash = strdup(cur_rev->files[j].hash);
            else
                rev->files[rev->file_count].hash = NULL;

            if (cur_rev->files[j].object)
                rev->files[rev->file_count].object = strdup(cur_rev->files[j].object);
            else
                rev->files[rev->file_count].object = NULL;

            rev->file_count++;
        }

        freeRevision(cur_rev);
    }
    return rev;
}

/**
 * Frees a revision from the heap
 */
void freeRevision(struct revision_t* rev)
{
    if (!rev) return;

    for (size_t i = 0; i < rev->file_count; ++i)
    {
        free(rev->files[i].path);
        free(rev->files[i].hash);
        free(rev->files[i].object);
    }
    free(rev->files);
    free(rev);
}


/**
 * Downloads an object from the specified API url to the specified game dir
 *
 * Returns bytes written
 */
size_t downloadObject(char* dir, char* url, struct file_info* info)
{
    size_t retval = 0;
    if (!info)
        return retval;

    char* object = info->object;

    char* buf_path = getToastDir(dir);
    if (!buf_path)
        return retval;

    size_t len = strlen(buf_path) + strlen(OS_PATH_SEP) + strlen(LOCAL_OBJECTS) + strlen(OS_PATH_SEP) + strlen(object) + 1;
    buf_path = realloc(buf_path, len);

    strncat(buf_path, OS_PATH_SEP, len - strlen(buf_path));
    strncat(buf_path, LOCAL_OBJECTS, len - strlen(buf_path));
    strncat(buf_path, OS_PATH_SEP, len - strlen(buf_path));

    if (!isDir(buf_path))
        makeDir(buf_path);

    strcat(buf_path, object);

    // the object has the right hash so we ignore
    if (isFile(buf_path) && !fileHash(buf_path, info->hash))
    {
        free(buf_path);
        return 0;
    }

    len = strlen(url) + 1 + strlen(OBJECTS_ENDPOINT) + 1 + strlen(object) + 1;
    char* buf_url = malloc(len);
    snprintf(buf_url, len, "%s/%s/%s", url, OBJECTS_ENDPOINT, object);

    retval = downloadToFile(buf_url, buf_path);

    free(buf_url);
    free(buf_path);

    return retval;
}

/**
 * Moves an object from the temporary object directory to its proper place
 *
 * Returns
 *  0 on success
 *  1 on missing object
 *  2 on general failure
 */
int applyObject(char* path, struct file_info* info)
{
    if (!info)
        return 2;

    char* file = info->path;
    char* object = info->object;

    if (!path || !isDir(path))
        return 2;
    else if (!object)
        return 0;

    size_t len = strlen(path) + strlen(OS_PATH_SEP) + strlen(LOCAL_OBJECTS_PATH) + strlen(OS_PATH_SEP) + strlen(object) + 1;
    char* buf_obj = malloc(len);
    snprintf(buf_obj, len, "%s%s%s%s%s", path, OS_PATH_SEP, LOCAL_OBJECTS_PATH, OS_PATH_SEP, object);

    if (!isFile(buf_obj))
    {
        free(buf_obj);
        len = strlen(path) + strlen(OS_PATH_SEP) + strlen(info->path) + 1;
        buf_obj = malloc(len);
        snprintf(buf_obj, len, "%s%s%s", path, OS_PATH_SEP, info->path);

        int exists = isFile(buf_obj);
        free(buf_obj);

        return !exists;
    }

    len = strlen(path) + strlen(OS_PATH_SEP) + strlen(file) + 1;
    char* buf_file = malloc(len);
    snprintf(buf_file, len, "%s%s%s", path, OS_PATH_SEP, file);

    rename(buf_obj, buf_file);

    free(buf_obj);
    free(buf_file);

    return 0;
}

/**
 * Removes temporarily stored objects
 */
void removeObjects(char* path)
{
    if (!path || !isDir(path))
        return;

    size_t len = strlen(path) + strlen(OS_PATH_SEP) + strlen(LOCAL_OBJECTS_PATH) + strlen(OS_PATH_SEP) + 1;
    char* buf = malloc(len);
    snprintf(buf, len, "%s%s%s%s", path, OS_PATH_SEP, LOCAL_OBJECTS_PATH, OS_PATH_SEP);

    removeDir(buf);

    free(buf);
}

/**
 * Internal function to generate check if a file matches a hash
 */
static int fileHash(char* path, char* hash)
{
    FILE* fd = fopen(path, "rb");
    if (!fd)
        return 1;

    fseek(fd, 0L, SEEK_END);
    size_t fd_size = (size_t)ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    MD5_CTX context;
    MD5Init(&context);

    if (fd_size)
    {
        char* buf = malloc(fd_size);
        if (!buf)
        {
            fclose(fd);
            return 1;
        }

        fread(buf, sizeof(char), fd_size, fd);

        MD5Update(&context, buf, fd_size);
        free(buf);
    }
    fclose(fd);

    unsigned char digest[16];
    MD5Final(digest, &context);

    char md5string[33];
    for(int i = 0; i < 16; ++i)
        snprintf(&md5string[i*2], 3, "%02x", (unsigned int)digest[i]);

    if (!strncmp(md5string, hash, sizeof(md5string)))
        return 0;

    return 1;
}

/**
 * Verifies the (md5) hash of a given file
 *
 * Returns
 *  0 on success
 *  1 on failure
 */
int verifyFileHash(char* path, struct file_info* info)
{
    if (!info)
        return 1;

    char* file = info->path;
    char* hash = info->hash;

    if (!path || !isDir(path))
        return 1;

    size_t len = strlen(path) + strlen(OS_PATH_SEP) + strlen(file) + 1;
    char* buf = malloc(len);
    if (!buf)
        return 1;
    snprintf(buf, len, "%s%s%s", path, OS_PATH_SEP, file);

    int hash_matches = fileHash(buf, hash);
    free(buf);

    return hash_matches;
}
