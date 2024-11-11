#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QCloseEvent>
#include <QColor>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QString>
#include <QTimer>

#include "globalconfig.h"

class DisplayWindow : public QLabel {
    Q_OBJECT

   public:
    explicit DisplayWindow(const QString &title, const QColor &backgroundColor, QWidget *parent = nullptr);
    ~DisplayWindow();

    bool showFrame(const QByteArray &frame);

   protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

   private:
    bool putScreenImage();

   signals:
    void closed();

   private:
    QString m_title;
    QImage m_screenImage;
    double m_ratio;
    bool m_isFirstFrame = true;

    QTimer m_timerFps;

    int m_frameCount = 0;
};

#endif  // DISPLAYWINDOW_H
