#include <iostream>
#include <limits.h>
#include <pthread.h>

#include "net.h"
#include "steam.h"
#include "toast.h"
#include "pool.h"

#include "./ui_mainwindow.h"
#include "workers.hpp"

struct thread_object_info {
    QString infoText;
    char* of_dir;
    char* remote;
    struct revision_t* rev;
    size_t index;

    Worker* worker;
};

static void* thread_download(void* pinfo)
{
    struct thread_object_info* info = (struct thread_object_info*)pinfo;
    Worker* worker = info->worker;
    int* do_work = &worker->do_work;

    if (info && *do_work)
    {
        char* of_dir = info->of_dir;
        char* remote = info->remote;
        struct revision_t* rev = info->rev;
        size_t i = info->index;

        struct file_info* file = &rev->files[i];
        if (file->type == TYPE_WRITE && *do_work)
        {
            info->infoText = QString("Verifying %1").arg(file->object);
            if (verifyFileHash(of_dir, file) && *do_work)
            {
                info->infoText = QString("Downloading %1").arg(file->object);
                downloadObject(of_dir, remote, file);
            }
        }

        QString* threadString = &info->infoText;
        if (!threadString->isEmpty() && *do_work)
        {
            int progress = (int)(((info->index * 100) + 1) / rev->file_count);
            if (progress > worker->progress)
                worker->progress = progress;

            worker->setInfoText(*threadString);
        }
    }
    return NULL;
}

Worker::Worker()
{
    net_init();
    of_dir = NULL;
    remote = NULL;
    textMutex = PTHREAD_MUTEX_INITIALIZER;
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

QString Worker::getArguments()
{
    return settings.value("launchArguments", QString()).toString();
}

void Worker::setInfoText(QString infoTextArg)
{
    pthread_mutex_lock(&this->textMutex);
    this->infoText = infoTextArg;
    emit this->resultReady(Worker::RESULT_UPDATE_TEXT);
    pthread_mutex_unlock(&this->textMutex);
}

void Worker::setArguments(QString argumentstr)
{
    settings.setValue("launchArguments", argumentstr);
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
    do_work = 0;
}

int Worker::update_setup(int local_rev, int remote_rev)
{
    if (!of_dir) return 1;
    else if (update_in_progress) return 0;

    update_in_progress = true;

    int retval = 0;
    struct revision_t* rev = fastFowardRevisions(remote, local_rev, remote_rev);

    if (rev)
    {
        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (leavesRelativePath(file->path))
            {
                infoText = QString("Revision contains invalid path '%1'").arg(file->path);
                emit resultReady(RESULT_UPDATE_TEXT);

                update_in_progress = false;
                freeRevision(rev);
                return 1;
            }
        }

        struct thread_object_info* thread_info = new struct thread_object_info[rev->file_count];
        struct pool_t* pool = pool_init();
        pool->condition = &do_work;

        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            struct thread_object_info* info = &thread_info[i];

            info->of_dir = of_dir;
            info->remote = remote;
            info->rev = rev;
            info->index = i;
            info->worker = this;

            pool_submit(pool, thread_download, info);
        }

        pool_complete(pool);
        pool_free(pool);
        delete[] thread_info;

        progress = 0;
        this->setInfoText("Processing");

        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (file->type != TYPE_DELETE) continue;
            size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
            char* buf = new char[len];
            snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
            if (isFile(buf))
            {
                remove(buf);
            }
            delete[] buf;

        }

        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (file->type != TYPE_MKDIR) continue;
            size_t len = strlen(of_dir) + strlen(OS_PATH_SEP) + strlen(file->path) + 1;
            char* buf = new char[len];
            snprintf(buf, len, "%s%s%s", of_dir, OS_PATH_SEP, file->path);
            makeDir(buf);
            delete[] buf;
        }

        for (size_t i = 0; i < rev->file_count && do_work; ++i)
        {
            struct file_info* file = &rev->files[i];

            if (file->type != TYPE_WRITE) continue;
            applyObject(of_dir, file);
        }

        if (do_work)
        {
            removeObjects(of_dir);
            setLocalRemote(of_dir, remote);
            setLocalRevision(of_dir, remote_rev);
        }

        freeRevision(rev);
    }

    this->setInfoText("");
    update_in_progress = false;

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
            if (result == RESULT_EXIT)
            {
                auto arg_list = getArguments().split(' ');

                auto argv = (char**)malloc(sizeof(char*) * arg_list.size());
                for (int i = 0; i < arg_list.size(); ++i)
                    argv[i] = strdup(arg_list[i].toStdString().c_str());

                runOpenFortress(argv, arg_list.size());

                for (int i = 0; i < arg_list.size(); ++i)
                    free(argv[i]);
                free(argv);
            }
            break;

        default:
            assert(0);
            break;
    }

    emit resultReady(result);
}
