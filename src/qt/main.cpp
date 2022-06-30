#include "mainwindow.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("OFQT");
    QCoreApplication::setApplicationName("OFQT");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
