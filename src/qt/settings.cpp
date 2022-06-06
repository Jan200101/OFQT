#include <QMessageBox>
#include <iostream>

#include "workers.hpp"
#include "settings.hpp"
#include "./ui_settings.h"

Settings::Settings(Worker* pworker, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Settings)
{
    this->worker = pworker;
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(applySettings()));
    connect(ui->verifyButton, SIGNAL(clicked()), this, SLOT(verify()));
    connect(this, &Settings::workerOperate, worker, &Worker::doWork);
}

void Settings::refresh()
{
    ui->serverEdit->setText(this->worker->getRemote());

    ui->revisionLabel->setText(QString("%1").arg(this->worker->getRevision()));
    ui->installLabel->setText(this->worker->getOfDir());
}

void Settings::applySettings()
{
    worker->setRemote(ui->serverEdit->text());
    this->hide();
}

void Settings::verify()
{
    workerOperate(Worker::TASK_INSTALL);
    QMessageBox::information(this, windowTitle(), "Verification Started" );
}

Settings::~Settings()
{
    delete ui;
}

