#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QCloseEvent>
#include <QImage>
#include <QMainWindow>
#include <QPixmap>
#include <QString>
#include <QTimer>

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

    QString title;
    QImage screenImage;

    QTimer timer1s;

    int frameCount = 0;
};

#endif  // DISPLAYWINDOW_H
