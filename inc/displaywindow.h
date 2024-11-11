#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QCloseEvent>
#include <QImage>
#include <QMainWindow>
#include <QPixmap>
#include <QString>
#include <QTimer>

#include "globalconfig.h"
#include "ui_displaywindow.h"

namespace Ui {
class DisplayWindow;
}

class DisplayWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit DisplayWindow(const QString &title, QWidget *parent = nullptr);
    ~DisplayWindow();

    bool showFrame(const QByteArray &frame);

   protected:
    void closeEvent(QCloseEvent *event) override;

   signals:
    void closed();

   private:
    Ui::DisplayWindow *ui;

    QString m_title;
    QImage m_screenImage;

    QTimer m_timerFps;

    int m_frameCount = 0;
};

#endif  // DISPLAYWINDOW_H
