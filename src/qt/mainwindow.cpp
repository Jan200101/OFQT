#include <QApplication>
#include <QMessageBox>
#include <QFontDatabase>
#include <QDesktopServices>
#include <QUrl>

#include <limits.h>
#include <iostream>

#include "steam.h"

#include "mainwindow.hpp"
#include "./ui_mainwindow.h"
#include "workers.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);

    qRegisterMetaType<Worker::Tasks_t>("Task_t");
    qRegisterMetaType<Worker::Results_t>("Results_t");

    QFontDatabase::addApplicationFont (":/font/assets/tf2build.ttf");
    QFont playFont("TF2 Build", 20, QFont::Bold);
    QFont progressFont("TF2 Build", 10, QFont::Normal);

    ui->mainButton->setFont(playFont);
    ui->progressBar->setFont(progressFont);
    ui->statusLabel->setFont(progressFont);
    ui->infoLabel->setFont(progressFont);

    QPixmap bkgnd(":/background/assets/background.bmp");
    bkgnd = bkgnd.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPalette palette;
    palette.setBrush(QPalette::Window, bkgnd);
    this->setPalette(palette);

    worker = new Worker();
    worker->moveToThread(&thread);

    connect(&thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &MainWindow::workerOperate, worker, &Worker::doWork);
    connect(worker, &Worker::resultReady, this, &MainWindow::workerResult);

    thread.start();

    //operateSVN(svnWorker::SVN_INSTALL);

    connect(ui->settingsButton, SIGNAL(clicked()), this, SLOT(settingsWindow()));

    settings = new Settings(worker, this);
    settings->setModal(true);
    //connect(settings, SIGNAL(visibleChanged()), this, SLOT(enable()));

    ui->mainButton->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->mainButton, SIGNAL(clicked()), this, SLOT(updateButton()));
    connect(ui->mainButton, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showButtonContext(const QPoint&)));
    connect(ui->buttonDiscord, SIGNAL(clicked()), this, SLOT(openDiscordInvite()));
    connect(ui->buttonWebsite, SIGNAL(clicked()), this, SLOT(openWebsite()));

    ui->mainButton->setText("...");
    ui->statusLabel->setText("...");
    ui->infoLabel->setText("...");

    in_progress = false;
    installed = false;
    uptodate = false;
    workerOperate(Worker::TASK_INIT);
}

void MainWindow::workerResult(const enum Worker::Results_t& result)
{
    switch (result)
    {

        case Worker::RESULT_UNINSTALL_COMPLETE:
        case Worker::RESULT_UNINSTALL_FAILURE:
        // TODO find better use for these
        case Worker::RESULT_NONE:
            resetProgress();
            break;

        case Worker::RESULT_EXIT:
            QCoreApplication::quit();
            break;

        case Worker::RESULT_UPDATE_TEXT:
            ui->progressBar->setValue(worker->progress);
            ui->infoLabel->setText(worker->infoText);
            worker->infoText.clear();
            break;

        case Worker::RESULT_IS_INSTALLED:
            installed = true;
            workerOperate(Worker::TASK_IS_UPTODATE);
            break;

        case Worker::RESULT_IS_NOT_INSTALLED:
            ui->mainButton->setText("Install");
            installed = false;
            break;

        case Worker::RESULT_IS_UPTODATE:
            uptodate = true;
            ui->mainButton->setText("Play");
            ui->statusLabel->setText("Up to Date");
            break;

        case Worker::RESULT_IS_OUTDATED:
            uptodate = false;
            ui->mainButton->setText("Update");
            ui->statusLabel->setText(QString("Revision %1 is available").arg(worker->getRemoteRevision()));
            break;

        case Worker::RESULT_INIT_COMPLETE:
            ui->statusLabel->setText("");
            ui->infoLabel->setText("");
            workerOperate(Worker::TASK_IS_INSTALLED);
            break;

        case Worker::RESULT_INIT_FAILURE:
            QMessageBox::information(this, windowTitle(), "Could not find install location.\nIs Steam installed?");
            QCoreApplication::quit();
            break;

        case Worker::RESULT_INSTALL_COMPLETE:
            resetProgress();
            ui->statusLabel->setText("Installed");
            workerOperate(Worker::TASK_IS_INSTALLED);
            break;

        case Worker::RESULT_INSTALL_FAILURE:
            ui->progressBar->setFormat("Install failed");
            break;

        case Worker::RESULT_UPDATE_COMPLETE:
            resetProgress();
            ui->statusLabel->setText("Updated");
            workerOperate(Worker::TASK_IS_UPTODATE);
            break;

        case Worker::RESULT_UPDATE_RUN:
            ui->statusLabel->setText("Launching");
            workerOperate(Worker::TASK_RUN);
            break;

        case Worker::RESULT_UPDATE_FAILURE:
            ui->statusLabel->setText("Update failed");
            break;

        case Worker::RESULT_NO_STEAM:
            resetProgress();
            QMessageBox::information(this, windowTitle(), "Steam is not running" );
            break;

    }

    in_progress = false;
}

void MainWindow::settingsWindow()
{
    settings->refresh();
    settings->show();
    //this->setEnabled(false);
}

void MainWindow::setupButton()
{
    workerOperate(Worker::TASK_IS_INSTALLED);
}

void MainWindow::updateButton()
{
    if (in_progress)
        return;

    if (installed)
    {
        if (!uptodate)
        {
            workerOperate(Worker::TASK_UPDATE);
            ui->statusLabel->setText("Updating (may take a while)");
        }
        else
        {
            workerOperate(Worker::TASK_RUN);
        }
    }
    else
    {
        workerOperate(Worker::TASK_INSTALL);
        ui->statusLabel->setText("Installing (may take a while)");
    }

    in_progress = true;
}

void MainWindow::showButtonContext(const QPoint& pos)
{
    QPoint absPos = ui->mainButton->mapToGlobal(pos); 

    QMenu ctxMenu;
    ctxMenu.addAction("Run without Update");

    QAction* selectedItem = ctxMenu.exec(absPos);

    if (selectedItem)
    {
        workerOperate(Worker::TASK_RUN);
    }
}


void MainWindow::openDiscordInvite()
{
  QDesktopServices::openUrl(QUrl("https://discord.gg/mKjW2ACCrm", QUrl::TolerantMode));
}

void MainWindow::openWebsite()
{
  QDesktopServices::openUrl(QUrl("https://openfortress.fun/", QUrl::TolerantMode));
}

void MainWindow::resetProgress()
{
    ui->progressBar->setFormat("%p%");
    ui->progressBar->setValue(-1);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete settings;

    worker->stop_work();
    thread.quit();
    thread.wait();
}

