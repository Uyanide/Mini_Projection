#include <QApplication>
#include <QMessageBox>

#include "globalconfig.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    if (!GlobalConfig::init(&w, "config.ini")) {
        QMessageBox::critical(&w, "Error", "Failed to load config.ini.");
        return 1;
    }
    return a.exec();
}
