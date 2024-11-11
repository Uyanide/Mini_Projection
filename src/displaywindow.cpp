#include "displaywindow.h"

DisplayWindow::DisplayWindow(const QString &title, const QColor &backgroundRole, QWidget *parent)
    : QLabel(parent), m_title(title), m_ratio(1.0) {
    resize(800, 800);
    setWindowTitle(m_title);
    setAlignment(Qt::AlignCenter);
    setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, backgroundRole);
    setPalette(palette);
}

DisplayWindow::~DisplayWindow() {
    if (m_timerFps.isActive()) {
        m_timerFps.stop();
    }
}

void DisplayWindow::closeEvent(QCloseEvent *event) {
    emit closed();
    QLabel::closeEvent(event);
}

void DisplayWindow::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    putScreenImage();
}

bool DisplayWindow::showFrame(const QByteArray &frame) {
    if (!m_isFirstFrame) {
        m_frameCount++;
    } else {  // init
        // fps timer
        m_timerFps.start(1000);
        connect(&m_timerFps, &QTimer::timeout, this, [this]() {
            setWindowTitle(m_title + " - " + QString::number(m_frameCount) + " FPS");
            m_frameCount = 0;
        });

        // resize
        if (m_screenImage.loadFromData(frame)) {
            QSize targetSize = m_screenImage.size();
            QSize currentSize = size();
            double targetRatio = targetSize.width() / static_cast<double>(targetSize.height());
            double currentRatio = currentSize.width() / static_cast<double>(currentSize.height());
            if (targetRatio > currentRatio) {
                currentSize.setHeight(currentSize.width() / targetRatio);
            } else {
                currentSize.setWidth(currentSize.height() * targetRatio);
            }
            resize(currentSize);
            m_ratio = currentSize.width() / static_cast<double>(targetSize.width());
        }
        m_isFirstFrame = false;
    }
    if (!frame.isEmpty()) {
        m_screenImage.loadFromData(frame);
        return putScreenImage();
    } else {
        return false;
    }
}

bool DisplayWindow::putScreenImage() {
    if (!m_screenImage.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(m_screenImage);
        pixmap.setDevicePixelRatio(GlobalConfig::DPR);
        pixmap = pixmap.scaled(size() * pixmap.devicePixelRatio(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setPixmap(pixmap);
        return true;
    }
    return false;
}
