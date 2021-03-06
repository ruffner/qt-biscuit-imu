#ifndef RUFBISCUITIMUOBJECT_H
#define RUFBISCUITIMUOBJECT_H

#include <QLabel>
#include <QImage>
#include <QDebug>
#include <QDialog>
#include <QString>
#include <QWidget>
#include <QPainter>
#include <QSettings>
#include <QByteArray>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QVariantMap>
#include <QDialogButtonBox>
#include <QAbstractSocket>
#include <QNetworkDatagram>
#include <QUdpSocket>
#include <QQuaternion>

#include <QHostAddress>
#include <QNetworkInterface>

#define COMMAND_ACK             "GOTCHA"
#define COMMAND_CONFIGURE       "CONFIGURE"
#define COMMAND_STREAM_START    "STREAM_START"
#define COMMAND_STREAM_STOP     "STREAM_STOP"

#define BISCUIT_ADDRESS "biscuit.local"

typedef enum {
    BISCUIT_LISTENING,
    BISCUIT_CONNECTED,
    BISCUIT_CONNECTING,
    BISCUIT_STREAMING,
    BISCUIT_DISCONNECTED
} biscuit_state;

class RUFBiscuitIMUObject : public QObject
{
    Q_OBJECT
public:
    explicit RUFBiscuitIMUObject(QObject *parent);
    ~RUFBiscuitIMUObject();

    bool isValid() {
        return (b_state == BISCUIT_CONNECTED);
    }
    bool isNull() {
        return (b_state != BISCUIT_CONNECTED);
    }

private:
    void startStreaming();
    void configureStream();

    QUdpSocket *socket;
    QHostAddress myAddress;
    biscuit_state b_state;

signals:
    void connected();
    void streamStarted();
    void streamStopped();

public slots:
    void onToggleStream();

    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError);
    void onSocketStateChanged(QAbstractSocket::SocketState);
};

#endif // RUFBISCUITIMUOBJECT_H
