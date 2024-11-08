#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QString>

#include "gameoperation.h"
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
    QString getDeviceInfo(const QString &key);
    bool checkMinicapFiles();
    int pushMinicapFiles(const QString &ABI, const QString &SDK);
    int addExecutePermission();
    int startMinicapServer();
    void initConnection();

    void setEnableInputFields(bool enable);

   private slots:
    void onPushButtonAdbClicked();
    void onPushButtonMinicapStartClicked();
    void onPushButtonMinicapStopClicked();
    void onPushButtonStartClicked();
    void onPushButtonStopClicked();

    void onMinicapServerReadyReadStandardError();
    void onMinicapServerFinished(int, QProcess::ExitStatus);

    static QString COLOR_LOG(const QString &text, LogColor color);
    void appendLog(const QString &log);

    void onSocketFrameReceived(QByteArray frame);
    void onSocketOnError(QString error);
    void onSocketConnected();

    void onGameRequestImage();
    void onGameLog(QString log, GameOperation::LogType type);
    void onGameFailed();
    void onGameTap(int x, int y);

   signals:
    void sendImage(QImage image);

   private:
    Ui::MainWindow *ui;
    QString adbPath;
    QString deviceName;
    quint16 forwardPort;

    QProcess minicapServer = QProcess(this);
    ServerState serverState = ServerState::IDLE;

    MinicapSocket *pSocket = nullptr;
    QImage screenImage;

    GameOperation *pGameOperation = nullptr;

    static const QString MINICAP_PATH;
    static const QString MINICAP_DEVICE_PATH;
    static const QString MINICAP_SERVER_LOG;
};

#endif  // MAINWINDOW_H
