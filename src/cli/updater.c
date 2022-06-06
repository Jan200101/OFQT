#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "fs.h"
#include "toast.h"

#include "updater.h"

#include <assert.h>

#define THREAD_COUNT 8

struct thread_object_info {
    int working;

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
            fprintf(stderr, "\rChecking    %lu/%lu (%s)", i+1, rev->file_count, file->object);

            if (verifyFileHash(of_dir, file))
            {
                fprintf(stderr, "\rDownloading %lu/%lu (%s)", i+1, rev->file_count, file->object);
                downloadObject(of_dir, remote, file);
            }
        }
    }

    info->working = 0;
    pthread_exit(0);

    return NULL;
}


void update_setup(char* of_dir, char* remote, int local_rev, int remote_rev)
{
    struct revision_t* rev = fastFowardRevisions(remote, local_rev, remote_rev);

    if (rev)
    {
        pthread_t download_threads[THREAD_COUNT] = {0};
        struct thread_object_info thread_info[THREAD_COUNT] = {0};
        size_t tindex = 0;

        for (size_t i = 0; i < rev->file_count; ++i)
        {
            while (thread_info[tindex].working)
            {
                tindex = (tindex+1) % THREAD_COUNT;
            }

            pthread_t* thread = &download_threads[tindex];
            struct thread_object_info* info = &thread_info[tindex];

            info->working = 1;
            info->of_dir = of_dir;
            info->remote = remote;
            info->rev = rev;
            info->index = i;

            pthread_create(thread, NULL, thread_download, info);
        }

        for (size_t i = 0; i < THREAD_COUNT; ++i)
        {
            pthread_t* thread = &download_threads[i];
            if (*thread)
                pthread_join(*thread, NULL);
        }

        for (size_t i = 0; i < rev->file_count; ++i)
        {
            struct file_info* file = &rev->files[i];
            if (file->type != TYPE_MKDIR)
                continue;

            size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
            char* buf = malloc(len);
            snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
            makeDir(buf);
            free(buf);
        }

        fprintf(stderr, "\rInstalling...");
        for (size_t i = 0; i < rev->file_count; ++i)
        {
            struct file_info* file = &rev->files[i];

            switch (file->type)
            {
                case TYPE_WRITE:
                case TYPE_MKDIR:
                    {
                        int k = applyObject(of_dir, file);
                        if (k)
                        {
                            printf("Failed to write %s\n", file->path);
                        }
                    }
                    break;

                case TYPE_DELETE:
                    {
                        size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
                        char* buf = malloc(len);
                        snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
                        if (isFile(buf) && remove(buf))
                        {
                            printf("Failed to delete %s\n", file->path);
                        }
                        free(buf);
                    }
                    break;
            }
        }

        removeObjects(of_dir);
        setLocalRemote(of_dir, remote);
        setLocalRevision(of_dir, remote_rev);

        fprintf(stderr, "\rUpdated OpenFortress\n");
        freeRevision(rev);
    }
}
