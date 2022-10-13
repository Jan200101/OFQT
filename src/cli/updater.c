#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"
#include "toast.h"
#include "pool.h"

#include "updater.h"

#include <assert.h>

struct thread_object_info {
    char* of_dir;
    char* remote;
    struct revision_t* rev;
    size_t index;
};

static void* thread_download(void* pinfo)
{
    struct thread_object_info* info = pinfo;
    if (info)
    {
        char* of_dir = info->of_dir;
        char* remote = info->remote;
        struct revision_t* rev = info->rev;
        size_t i = info->index;

        struct file_info* file = &rev->files[i];
        if (file->type == TYPE_WRITE)
        {
            fprintf(stderr, "\rChecking    %zu/%zu (%s)", i+1, rev->file_count, file->object);

            if (verifyFileHash(of_dir, file))
            {
                fprintf(stderr, "\rDownloading %zu/%zu (%s)", i+1, rev->file_count, file->object);
                downloadObject(of_dir, remote, file);
            }
        }
    }
    return NULL;
}


void update_setup(char* of_dir, char* remote, int local_rev, int remote_rev)
{
    struct revision_t* rev = fastFowardRevisions(remote, local_rev, remote_rev);

    if (rev)
    {
        fprintf(stderr, "Validating revisions");
        for (size_t i = 0; i < rev->file_count; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (leavesRelativePath(file->path))
            {
                fprintf(stderr, "Revision contains invalid path '%s'\n", file->path);
                freeRevision(rev);
                return;
            }
        }

        struct thread_object_info* thread_info = malloc(sizeof(struct thread_object_info) * rev->file_count);
        struct pool_t* pool = pool_init();

        for (size_t i = 0; i < rev->file_count; ++i)
        {
            struct thread_object_info* info = &thread_info[i];

            info->of_dir = of_dir;
            info->remote = remote;
            info->rev = rev;
            info->index = i;

            pool_submit(pool, thread_download, info);
        }

        pool_complete(pool);
        pool_free(pool);
        free(thread_info);

        puts("");
        for (size_t i = 0; i < rev->file_count; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (file->type != TYPE_DELETE) continue;
            fprintf(stderr, "\rDeleting  %zu/%zu", i+1, rev->file_count);
            size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
            char* buf = malloc(len);
            snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
            if (isFile(buf) && remove(buf))
            {
                fprintf(stderr, "\rFailed to delete %s\n", file->path);
            }
            free(buf);
        }

        for (size_t i = 0; i < rev->file_count; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (file->type != TYPE_MKDIR) continue;
            size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
            char* buf = malloc(len);
            fprintf(stderr, "\rCreating %zu/%zu", i+1, rev->file_count);
            snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
            if (!isDir(buf) && makeDir(buf))
            {
                fprintf(stderr, "\nFailed to create %s\n", file->path);
            }
            free(buf);
        }

        for (size_t i = 0; i < rev->file_count; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (file->type != TYPE_WRITE) continue;
            fprintf(stderr, "\rInstalling  %zu/%zu (%s)", i+1, rev->file_count, file->object);
            if (applyObject(of_dir, file))
            {
                fprintf(stderr, "\nFailed to write %s\n", file->path);
            }
        }

        removeObjects(of_dir);
        setLocalRemote(of_dir, remote);
        setLocalRevision(of_dir, remote_rev);

        fprintf(stderr, "\nUpdated OpenFortress\n");
        freeRevision(rev);
    }
}
