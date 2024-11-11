#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QMainWindow>

#include "ui_displaywindow.h"

namespace Ui {
class DisplayWindow;
}

class DisplayWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit DisplayWindow(QWidget *parent = nullptr);
    ~DisplayWindow();

    QLabel *getLabelScreen() const { return ui->label_screen; }

   protected:
    void closeEvent(QCloseEvent *event) override;

   signals:
    void closed();

   private:
    Ui::DisplayWindow *ui;
};

#endif  // DISPLAYWINDOW_H
