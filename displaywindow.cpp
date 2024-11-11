#include "displaywindow.h"

#include "ui_displaywindow.h"

DisplayWindow::DisplayWindow(const QString &title, QWidget *parent)
    : title(title), QMainWindow(parent), ui(new Ui::DisplayWindow) {
    ui->setupUi(this);
}

void DisplayWindow::closeEvent(QCloseEvent *event) {
    emit closed();
    QMainWindow::closeEvent(event);
}

DisplayWindow::~DisplayWindow() {
    if (timer1s.isActive()) {
        timer1s.stop();
    }
    delete ui;
}

bool DisplayWindow::showFrame(const QByteArray &frame) {
    if (timer1s.isActive()) {
        frameCount++;
    } else {
        timer1s.start(1000);
        connect(&timer1s, &QTimer::timeout, [this]() {
            setWindowTitle(title + " - " + QString::number(frameCount) + " FPS");
            frameCount = 0;
        });
    }
    if (!frame.isEmpty()) {
        if (screenImage.loadFromData(frame)) {
            QPixmap pixmap = QPixmap::fromImage(screenImage);
            pixmap.setDevicePixelRatio(1.0);
            pixmap = pixmap.scaled(ui->label_screen->size() * pixmap.devicePixelRatio(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->label_screen->setPixmap(pixmap);
            return true;
        }
    }
    return false;
}