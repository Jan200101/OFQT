#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QWidget>
#include <QDialog>

#include "workers.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class Settings; }
QT_END_NAMESPACE

class Settings : public QDialog
{
    Q_OBJECT

public:
    Settings(Worker* pworker, QWidget *parent = nullptr);
    ~Settings();

    void refresh();

private:
    Ui::Settings *ui;
    Worker* worker;

public slots:
    void applySettings();
    void verify();

signals:
    void workerOperate(const Worker::Tasks_t &);

};
#endif // SETTINGS_HPP
