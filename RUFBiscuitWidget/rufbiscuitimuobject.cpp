#include "rufbiscuitimuobject.h"

RUFBiscuitIMUObject::RUFBiscuitIMUObject(QObject *parent) : QObject(parent), socket(NULL)
{
    socket = new QUdpSocket(this);
    b_state = BISCUIT_DISCONNECTED;

    qDebug() << b_state;
    qDebug() << "SOCKET STATE :: " << socket->state();

    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

    socket->bind(1980, QAbstractSocket::ReuseAddressHint);
}

RUFBiscuitIMUObject::~RUFBiscuitIMUObject()
{
    if( socket ){
        qDebug() << "DISCONNECTING!";
        socket->disconnectFromHost();
        qDebug() << "DELETING SOCKET";
        delete socket;
    }
}

void RUFBiscuitIMUObject::onSocketStateChanged(QAbstractSocket::SocketState)
{
    //qDebug() << "SOCKET STATE :: " << socket->state();

    if( socket->state() == QAbstractSocket::UnconnectedState ){
        qDebug() << "SOCKET BOUND, CONNECTING TO BISCUIT";
        b_state = BISCUIT_DISCONNECTED;
    } else if( socket->state() == QAbstractSocket::BoundState ){
        b_state = BISCUIT_LISTENING;
        socket->connectToHost(QString(BISCUIT_ADDRESS), 1980);
    } else if( socket->state() == QAbstractSocket::HostLookupState ){
        if(DEBUG) qDebug() << "ATTEMPTING TO CONNECT TO BISCUIT AT " << BISCUIT_ADDRESS;
    } else if( socket->state() == QAbstractSocket::ConnectingState ){
        if(DEBUG) qDebug() << "CONNECTING TO BISCUIT ON LOCAL NETWORK";
    } else if( socket->state() == QAbstractSocket::ConnectedState ){
        if(DEBUG) qDebug() << "CONNECTED TO BISCUIT";
        b_state = BISCUIT_CONNECTED;
        configureStream();
    }

}

void RUFBiscuitIMUObject::onSocketError(QAbstractSocket::SocketError)
{
    qDebug() << "SOCKET ERROR: " << socket->errorString();
}

void RUFBiscuitIMUObject::onReadyRead()
{
    if ( socket->hasPendingDatagrams() ) {

        QNetworkDatagram datagram = socket->receiveDatagram();

        // PROBABLY NOT JSON IF NO BRACES
        if( b_state == BISCUIT_CONNECTING ){
            if( QString(datagram.data()).compare(QString("192.168.1.22")) == 0 ){
                b_state = BISCUIT_CONNECTED;
                emit connected();
                return;
            }
        } else if( b_state == BISCUIT_STREAMING ){

            QJsonDocument doc = QJsonDocument::fromJson(datagram.data());
            QJsonObject jObject = doc.object();

            qDebug() << datagram.data();

            qDebug() << "W:" << jObject["w"].toDouble();
            qDebug() << "X:" << jObject["x"].toDouble();
            qDebug() << "Y:" << jObject["y"].toDouble();
            qDebug() << "Z:" << jObject["z"].toDouble();

        }

    }
}

void RUFBiscuitIMUObject::configureStream()
{
    // TODO: needs work
    socket->write("CONFIGURE192.168.1.22");
    b_state = BISCUIT_CONNECTING;
    qDebug() << "CONFIGURING BISCUIT";
}

void RUFBiscuitIMUObject::onToggleStream()
{
    if( b_state == BISCUIT_STREAMING ){
        socket->write(COMMAND_STREAM_STOP);
        b_state = BISCUIT_CONNECTED;
        emit streamStopped();
    } else if( b_state == BISCUIT_CONNECTED ){
        socket->write(COMMAND_STREAM_START);
        b_state = BISCUIT_STREAMING;
        emit streamStarted();
    }
}
