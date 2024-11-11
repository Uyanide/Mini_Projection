#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPair>
#include <QProcess>
#include <QString>

#include "adbcommand.h"
#include "globalconfig.h"
#include "minicapwrapper.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private:
    void applyQSS();
    void initUI();
    void initSlots();
    void initCheck();

    void setEnableInputFields(bool enable);

   private slots:
    void onPushButtonAdbClicked();
    void onPushButtonDeviceClicked();
    void onComboBoxDeviceCurrentIndexChanged(int index);

    void onPushButtonStartClicked();
    void onPushButtonStopClicked();

    void appendLog(const QString &log);

   private:
    Ui::MainWindow *ui;
    AdbCommand *m_adbCommand = nullptr;
    MinicapWrapper *m_minicapServer = nullptr;
};

#endif  // MAINWINDOW_H
