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
    void initCheck();

    int pushMinicapFiles(const QString &ABI, const QString &SDK);
    bool startMinicapServer();
    void initConnection();
    void initWindow();

    void setEnableInputFields(bool enable);

   private slots:
    void onPushButtonAdbClicked();
    void onPushButtonDeviceClicked();
    void onComboBoxDeviceCurrentIndexChanged(int index);

    void onPushButtonStartClicked();
    void onPushButtonStopClicked();

    void onMinicapServerReadyReadStandardError();
    void onMinicapServerFinished(int, QProcess::ExitStatus);

    static QString COLOR_LOG(const QString &text, LogColor color);
    void appendLog(const QString &log);

    void onSocketFrameReceived(QByteArray frame);
    void onSocketOnError(QString error);
    void onSocketConnected();

   private:
    Ui::MainWindow *ui;
    quint16 m_forwardPort;

    QString m_abi;
    QString m_sdk;

    QProcess *m_minicapServer = nullptr;
    ServerState m_serverState = ServerState::IDLE;

    MinicapSocket *m_pSocketThread = nullptr;

    DisplayWindow *m_displayWindow = nullptr;

    AdbCommand *m_adbCommand;
};

#endif  // MAINWINDOW_H
