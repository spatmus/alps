#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.loadConfiguration("config.ini");
    w.loadWave();
    w.initSound();
    w.show();

    return a.exec();
}
