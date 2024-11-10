#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QMainWindow>

namespace Ui {
class DisplayWindow;
}

class DisplayWindow : public QMainWindow {
    Q_OBJECT

    friend class MainWindow;

   public:
    explicit DisplayWindow(QWidget *parent = nullptr);
    ~DisplayWindow();

   protected:
    void closeEvent(QCloseEvent *event) override;

   signals:
    void closed();

   private:
    Ui::DisplayWindow *ui;
};

#endif  // DISPLAYWINDOW_H
