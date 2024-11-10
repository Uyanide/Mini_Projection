#include "displaywindow.h"

#include "ui_displaywindow.h"

DisplayWindow::DisplayWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::DisplayWindow) {
    ui->setupUi(this);
}

void DisplayWindow::closeEvent(QCloseEvent *event) {
    emit closed();
    QMainWindow::closeEvent(event);
}

DisplayWindow::~DisplayWindow() {
    delete ui;
}
