#include "displaywindow.h"

#include "ui_displaywindow.h"

DisplayWindow::DisplayWindow(const QString &title, QWidget *parent)
    : m_title(title), QMainWindow(parent), ui(new Ui::DisplayWindow) {
    ui->setupUi(this);
}

void DisplayWindow::closeEvent(QCloseEvent *event) {
    emit closed();
    QMainWindow::closeEvent(event);
}

DisplayWindow::~DisplayWindow() {
    if (m_timerFps.isActive()) {
        m_timerFps.stop();
    }
    delete ui;
}

bool DisplayWindow::showFrame(const QByteArray &frame) {
    if (m_timerFps.isActive()) {
        m_frameCount++;
    } else {
        m_timerFps.start(1000);
        connect(&m_timerFps, &QTimer::timeout, [this]() {
            setWindowTitle(m_title + " - " + QString::number(m_frameCount) + " FPS");
            m_frameCount = 0;
        });
    }
    if (!frame.isEmpty()) {
        if (m_screenImage.loadFromData(frame)) {
            QPixmap pixmap = QPixmap::fromImage(m_screenImage);
            pixmap.setDevicePixelRatio(GlobalConfig::DPR);
            pixmap = pixmap.scaled(ui->label_screen->size() * pixmap.devicePixelRatio(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->label_screen->setPixmap(pixmap);
            return true;
        }
    }
    return false;
}