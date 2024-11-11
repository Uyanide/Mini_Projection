#include <QApplication>

#include "globalconfig.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    GlobalConfig::init(&w);
    return a.exec();
}
