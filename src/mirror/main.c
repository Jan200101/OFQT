#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "toast.h"
#include "fs.h"
#include "pool.h"
#include "net.h"

struct thread_object_info {
    int i;
    size_t j;

    char* output_dir;
    char* remote;
    struct revision_t* rev;
    struct file_info* file;
};

static void* thread_download(void* pinfo)
{
    struct thread_object_info* info = pinfo;
    if (info)
    {
        struct file_info* file = info->file;
        fprintf(stderr, "\r [%i][%li/%li] Processing %s", info->i, info->j+1, info->rev->file_count, file->object);

        // downloadObject checks if an object with the current hash is already present
        downloadObject(info->output_dir, info->remote, info->file);
    }
    return NULL;
}

int main(int argc, char** argv)
{
    int retval = 0;
    char* remote = NULL;
    char* output_dir = NULL;

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--remote"))
        {
            if (!argv[++i])
            {
                puts("No URL specified");
                return EXIT_FAILURE;
            }
            remote = strdup(argv[i]);
            printf("Using %s as the server\n", remote);
        }
        else if (!output_dir)
            output_dir = strdup(argv[i]);
        else
        {

            puts("Too many arguments");
            retval = 1;
            goto cleanup;
        }
    }

    if (!output_dir)
    {
        printf("%s [output_dir]\n", argv[0]);
        retval = 1;
        goto cleanup;
    }

    if (!remote)
        remote = getLocalRemote(output_dir);
    else
        setLocalRemote(output_dir, remote);


    // symlink objects directory
    size_t len = strlen(output_dir) + strlen(OS_PATH_SEP) + strlen(TOAST_LOCAL_OBJECTS) + 1;
    char* objects_dir = malloc(len);
    snprintf(objects_dir, len, "%s%s%s", output_dir, OS_PATH_SEP, TOAST_LOCAL_OBJECTS);

    if (!isDir(objects_dir))
    {
        symlink(TOAST_LOCAL_OBJECTS_PATH, objects_dir);
    }

    free(objects_dir);


    // setup revisions directory and a buffer to put the paths into
    len = strlen(output_dir) + strlen(OS_PATH_SEP) + strlen(TOAST_REVISIONS_ENDPOINT) + strlen(OS_PATH_SEP) + strlen("latest") + 1;
    char* revisions_dir = malloc(len);
    snprintf(revisions_dir, len, "%s%s%s", output_dir, OS_PATH_SEP, TOAST_REVISIONS_ENDPOINT);
    char* revisions_dir_end = revisions_dir+strlen(revisions_dir);

    if (!isDir(revisions_dir))
    {
        makeDir(revisions_dir);
    }

    // download objects
    printf("Downloading objects\n");
    int latest_rev = getLatestRemoteRevision(remote);
    size_t rev_len = 1;
    {

        int rev_copy = latest_rev;
        while (rev_copy >= 10)
        {
            rev_copy /= 10;
            rev_len++;
        }
    }

    for (int i = 0; i <= latest_rev; ++i)
    {
        fprintf(stderr, "\r [%i] Downloading", i);
        sprintf(revisions_dir_end, "%s%i", OS_PATH_SEP, i);

        len = strlen(remote) + 1 + strlen(TOAST_REVISIONS_ENDPOINT) + 1 + rev_len + 1;
        char* buf = malloc(len);
        snprintf(buf, len, "%s/%s/%i", remote, TOAST_REVISIONS_ENDPOINT, i);
        downloadToFile(buf, revisions_dir);
        free(buf);

        struct revision_t* rev = getRevisionData(remote, i);
        struct thread_object_info* thread_info = malloc(sizeof(struct thread_object_info) * rev->file_count);
        struct pool_t* pool = pool_init();

        fprintf(stderr, "\r [%i] Preparing  ", i);
        for (size_t j = 0; j < rev->file_count; ++j)
        {
            struct file_info* file = &rev->files[j];

            if (file->object)
            {
                struct thread_object_info* info = &thread_info[j];
                info->i = i;
                info->j = j;
                info->output_dir = output_dir;
                info->remote = remote;
                info->rev = rev;
                info->file = file;

                pool_submit(pool, thread_download, info);
            }
        }
        pool_complete(pool);
        printf("\n");
        pool_free(pool);
        freeRevision(rev);
        free(thread_info);
    }

    sprintf(revisions_dir_end, "%slatest", OS_PATH_SEP);
    setLocalRevision(output_dir, latest_rev);
    symlink(".." OS_PATH_SEP TOAST_LOCAL_REVISION_PATH, revisions_dir);
    free(revisions_dir);

    cleanup:

    if (remote)
        free(remote);
    if (output_dir)
        free(output_dir);

    return retval;
}