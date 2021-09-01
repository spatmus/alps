#include "mainwindow.h"
#include <QApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString cfg = QDir::current().path() + QDir::separator() + "config.ini";

    MainWindow w;
    w.loadConfiguration(argc > 1 ? argv[1] : cfg.toLatin1().data());
    w.loadWave();
    w.initSound();
    w.show();

    return a.exec();
}
