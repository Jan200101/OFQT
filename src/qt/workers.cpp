#include <iostream>
#include <limits.h>
#include <pthread.h>

#include "net.h"
#include "steam.h"
#include "toast.h"

#include "./ui_mainwindow.h"
#include "workers.hpp"

#define THREAD_COUNT 4

struct thread_object_info {
    int working;

    QString* infoText;
    char* of_dir;
    char* remote;
    struct revision_t* rev;
    size_t index;
};

static void* thread_download(void* pinfo)
{
    struct thread_object_info* info = (struct thread_object_info*)pinfo;
    if (info)
    {
        QString* infoText = info->infoText;
        char* of_dir = info->of_dir;
        char* remote = info->remote;
        struct revision_t* rev = info->rev;
        size_t i = info->index;

        struct file_info* file = &rev->files[i];
        if (file->type == TYPE_WRITE)
        {
            *info->infoText = QString("Verifying %1").arg(file->object);
            if (verifyFileHash(of_dir, file))
            {
                *infoText = QString("Downloading %1").arg(file->object);
                downloadObject(of_dir, remote, file);
            }
        }
    }

    info->working = 0;
    pthread_exit(0);

    return NULL;
}

Worker::Worker()
{
    net_init();
    of_dir = NULL;
    remote = NULL;
}

Worker::~Worker()
{
    net_deinit();

    if (of_dir)
        free(of_dir);

    if (remote)
        free(remote);
}

QString Worker::getOfDir()
{
    return QString(of_dir);
}

QString Worker::getRemote()
{
    return QString(remote);
}

void Worker::setRemote(QString remotestr)
{
    if (remote)
        free(remote);

    remote = strdup(remotestr.toStdString().c_str());
    setLocalRemote(of_dir, remote);
}

int Worker::getRevision()
{
    return getLocalRevision(of_dir);
}

int Worker::getRemoteRevision()
{
    return getLatestRemoteRevision(remote);
}

bool Worker::isOutdated()
{
    return getRemoteRevision() > getRevision();
}

void Worker::stop_work()
{
    do_work = false;
}

int Worker::update_setup(int local_rev, int remote_rev)
{
    if (!of_dir) return 1;

    int retval = 0;


    struct revision_t* rev = fastFowardRevisions(remote, local_rev, remote_rev);

    if (rev)
    {
        pthread_t download_threads[THREAD_COUNT] = {0};
        struct thread_object_info thread_info[THREAD_COUNT] = {0, NULL, NULL, NULL, NULL, 0};
        size_t tindex = 0;
        QString infoStrings[THREAD_COUNT];

        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            while (thread_info[tindex].working)
            {
                tindex = (tindex+1) % THREAD_COUNT;
            }

            pthread_t* thread = &download_threads[tindex];
            struct thread_object_info* info = &thread_info[tindex];
            QString* threadString = &infoStrings[tindex];

            if (!threadString->isEmpty())
            {
                infoText = *threadString;
                emit resultReady(RESULT_UPDATE_TEXT);
            }

            info->working = 1;
            info->infoText = threadString;
            info->of_dir = of_dir;
            info->remote = remote;
            info->rev = rev;
            info->index = i;
            progress = (int)(((i * 100) + 1) / rev->file_count);

            emit resultReady(RESULT_UPDATE_TEXT);
            pthread_create(thread, NULL, thread_download, info);
        }

        for (size_t i = 0; i < THREAD_COUNT; ++i)
        {
            pthread_t* thread = &download_threads[i];
            if (*thread)
                pthread_join(*thread, NULL);
        }

        progress = 0;
        infoText = QString("Processing");
        emit resultReady(RESULT_UPDATE_TEXT);

        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            struct file_info* file = &rev->files[i];
            if (file->type != TYPE_MKDIR)
                continue;

            progress = (int)(((i * 100) + 1) / rev->file_count);
            emit resultReady(RESULT_UPDATE_TEXT);

            size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
            char* buf = (char*)malloc(len);
            snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
            makeDir(buf);
            free(buf);
        }

        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            struct file_info* file = &rev->files[i];

            progress = (int)(((i * 100) + 1) / rev->file_count);
            emit resultReady(RESULT_UPDATE_TEXT);

            switch (file->type)
            {
                case TYPE_WRITE:
                case TYPE_MKDIR:
                    {
                        retval += applyObject(of_dir, file);
                    }
                    break;

                case TYPE_DELETE:
                    {
                        size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
                        char* buf = (char*)malloc(len);
                        snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
                        if (isFile(buf))
                            retval += remove(buf);
                        free(buf);
                    }
                    break;
            }
        }

        if (do_work)
        {
            removeObjects(of_dir);
            setLocalRemote(of_dir, remote);
            setLocalRevision(of_dir, remote_rev);
        }

        progress = 0;
        infoText = QString("");
        emit resultReady(RESULT_UPDATE_TEXT);

        freeRevision(rev);
    }

    return retval;
}


void Worker::doWork(const enum Worker::Tasks_t &parameter) {
    Results_t result = RESULT_NONE;


    switch (parameter)
    {
        case TASK_INVALID:
            break;

        case TASK_INIT:
            result = RESULT_INIT_FAILURE;
            of_dir = getOpenFortressDir();

            if (of_dir)
            {
                of_dir_len = strlen(of_dir);
                remote = getLocalRemote(of_dir);

                if (remote)
                {
                    remote_len = strlen(remote);
                    result = RESULT_INIT_COMPLETE;
                }
                else
                {
                    free(of_dir);
                    of_dir = NULL;
                    remote = NULL;
                }
            }
            else
            {
                of_dir = NULL;
                remote = NULL;
            }

            break;

        case TASK_IS_INSTALLED:
            result = getRevision() > -1 ? RESULT_IS_INSTALLED : RESULT_IS_NOT_INSTALLED;
            break;

        case TASK_IS_UPTODATE:
            result = isOutdated() ? RESULT_IS_OUTDATED : RESULT_IS_UPTODATE;
            break;

        case TASK_UNINSTALL:
            result = RESULT_UNINSTALL_FAILURE;
            //if (direxists) result = svn_delete(mod) ? RESULT_UNINSTALL_FAILURE : RESULT_UNINSTALL_COMPLETE;
            break;

        case TASK_INSTALL:
            result = update_setup(0, getRemoteRevision()) > 0 ? RESULT_INSTALL_FAILURE : RESULT_INSTALL_COMPLETE;
            break;

        case TASK_UPDATE:
            result = update_setup(getRevision(), getRemoteRevision()) > 0 ? RESULT_UPDATE_FAILURE : RESULT_UPDATE_COMPLETE;
            break;

        case TASK_UPDATE_RUN:
            result = update_setup(getRevision(), getRemoteRevision()) > 0 ? RESULT_UPDATE_FAILURE : RESULT_UPDATE_COMPLETE;
            break;

        case TASK_RUN:
            result = getSteamPID() > -1 ? RESULT_EXIT : RESULT_NO_STEAM;
            if (result == RESULT_EXIT) runOpenFortress();
            break;
    }

    emit resultReady(result);
}