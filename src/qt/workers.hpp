#ifndef WORKERS_HPP
#define WORKERS_HPP

#include <QObject>
#include <limits.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Worker : public QObject
{
    Q_OBJECT

private:
    char* of_dir;
    size_t of_dir_len;

    char* remote;
    size_t remote_len;

    bool do_work = true;
    bool update_in_progress = false;

public:
    int progress = -1;
    QString infoText;

    Worker();
    ~Worker();

    QString getOfDir();
    QString getRemote();
    void setRemote(QString);

    int getRevision();
    int getRemoteRevision();
    bool isOutdated();

    void stop_work();

    int update_setup(int, int);

    enum Tasks_t
    {
        TASK_INVALID,

        TASK_IS_INSTALLED,
        TASK_IS_UPTODATE,

        TASK_INIT,
        TASK_INSTALL,
        TASK_UNINSTALL,
        TASK_UPDATE,
        TASK_UPDATE_RUN,
        TASK_RUN,
    };
    Q_ENUM(Tasks_t)

    enum Results_t
    {
        RESULT_NONE,
        RESULT_EXIT,

        RESULT_UPDATE_TEXT,

        RESULT_IS_INSTALLED,
        RESULT_IS_NOT_INSTALLED,
        RESULT_IS_UPTODATE,
        RESULT_IS_OUTDATED,

        RESULT_INIT_COMPLETE,
        RESULT_INIT_FAILURE,
        RESULT_INSTALL_COMPLETE,
        RESULT_INSTALL_FAILURE,
        RESULT_UNINSTALL_COMPLETE,
        RESULT_UNINSTALL_FAILURE,
        RESULT_UPDATE_COMPLETE,
        RESULT_UPDATE_FAILURE,
        RESULT_UPDATE_RUN,

        RESULT_NO_STEAM
    };
    Q_ENUM(Results_t)

public slots:
    void doWork(const Tasks_t &);

signals:
    void resultReady(const Results_t &);

};


#endif