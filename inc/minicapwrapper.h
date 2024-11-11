#ifndef MINICAPWRAPPERH_H
#define MINICAPWRAPPERH_H

#include "adbcommand.h"
#include "displaywindow.h"
#include "globalconfig.h"
#include "minicapsocket.h"

class MinicapWrapper : public QObject {
    Q_OBJECT

   public:
    enum ServerState {
        IDLE,
        PREPARING,
        STARTING,
        STARTED,
        STOPPING,
    };

    MinicapWrapper(
        quint16 forwardPort, int frameRate, double scale,
        const QString &deviceName, const QColor &backgroundColor,
        AdbCommand *adbCommand, QObject *parent = nullptr);
    ~MinicapWrapper();
    bool start();
    void stop();

    inline ServerState getServerState() const { return m_serverState; }

   private:
    bool pushMinicapFiles(const QString &ABI, const QString &SDK);
    bool startMinicapServer();
    void initConnection();
    void initWindow();

   private slots:
    void onSocketConnected();
    void onSocketFrameReceived(QByteArray frame);
    void onSocketOnError(QString error);
    void onMinicapServerFinished(int, QProcess::ExitStatus);
    void onMinicapServerReadyReadStandardError();

   signals:
    void appendLog(const QString &log);
    void started();
    void stopped();

   private:
    quint16 m_forwardPort;
    int m_frameRate;
    double m_scale;
    QString m_deviceName;
    QColor m_backgroundColor;

    QString m_abi;
    QString m_sdk;

    QProcess *m_minicapServer = nullptr;
    ServerState m_serverState = ServerState::IDLE;

    MinicapSocket *m_pSocketThread = nullptr;

    DisplayWindow *m_displayWindow = nullptr;

    AdbCommand *m_adbCommand = nullptr;
};

#endif  // MINICAPWRAPPERH_H
