#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPair>
#include <QProcess>
#include <QString>

#include "adbcommand.h"
#include "displaywindow.h"
#include "globalconfig.h"
#include "minicapsocket.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

    enum ServerState {
        IDLE,
        PREPARING,
        STARTING,
        STARTED,
        STOPPING,
    };

    enum LogColor {
        GRAY = 0,
        YELLOW,
        RED,
        GREEN,
        BLUE,
    };

   public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

   private:
    void applyQSS();
    void initUI();
    void initSlots();
    // QString getDeviceInfo(const QString &key);
    // bool checkMinicapFiles();
    int pushMinicapFiles(const QString &ABI, const QString &SDK);
    // int addExecutePermission();
    // QPair<int, int> adbGetScreenSize();
    int startMinicapServer();
    int initConnection();
    void initWindow();

    void setEnableInputFields(bool enable);

   private slots:
    void onPushButtonAdbClicked();
    void onPushButtonDeviceClicked();
    void onComboBoxDeviceCurrentIndexChanged(int index);

    void onPushButtonMinicapStartClicked();
    void onPushButtonMinicapStopClicked();

    void onMinicapServerReadyReadStandardError();
    void onMinicapServerFinished(int, QProcess::ExitStatus);

    static QString COLOR_LOG(const QString &text, LogColor color);
    void appendLog(const QString &log);

    void onSocketFrameReceived(QByteArray frame);
    void onSocketOnError(QString error);
    void onSocketConnected();

   private:
    Ui::MainWindow *ui;
    quint16 forwardPort;

    QString abi;
    QString sdk;

    QProcess *minicapServer = nullptr;
    ServerState serverState = ServerState::IDLE;

    MinicapSocket *pSocket = nullptr;
    QImage screenImage;

    DisplayWindow *displayWindow = nullptr;

    AdbCommand *adbCommand;
};

#endif  // MAINWINDOW_H
