#include "rufbiscuitimuobject.h"

RUFBiscuitIMUObject::RUFBiscuitIMUObject(QObject *parent) : QObject(parent), socket(NULL)
{
    socket = new QUdpSocket(this);
    b_state = BISCUIT_DISCONNECTED;

    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

    // DETERMINE THIS MACHINES IP ON LOCAL NETWORK
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    for (int nIter=0; nIter<list.count(); nIter++ ){
        if (list[nIter].isLoopback() || list[nIter].protocol() != QAbstractSocket::IPv4Protocol ){
            list.removeAt(nIter);
            nIter--;
        }
    }
    if( list.size() == 1 ){
        myAddress = list[0];
    } else {
        // TODO: DIALOG TO CHOOSE WHICH INTERFACE ADDRESS TO USE
        myAddress = list[0];
    }
    qDebug() << "USING " << myAddress.toString() << " AS TARGET IP FOR BISCUIT";


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
    if( socket->state() == QAbstractSocket::UnconnectedState ){
        qDebug() << "SOCKET DISCONNECTED";
        b_state = BISCUIT_DISCONNECTED;
    }
    else if( socket->state() == QAbstractSocket::BoundState ){
        b_state = BISCUIT_LISTENING;
        socket->connectToHost(QString(BISCUIT_ADDRESS), 1980);
    }
    else if( socket->state() == QAbstractSocket::HostLookupState ){
        //qDebug() << "ATTEMPTING TO CONNECT TO BISCUIT AT " << BISCUIT_ADDRESS;
    }
    else if( socket->state() == QAbstractSocket::ConnectingState ){
        //qDebug() << "CONNECTING TO BISCUIT ON LOCAL NETWORK";
    }
    else if( socket->state() == QAbstractSocket::ConnectedState ){
        qDebug() << "BISCUIT FOUND ON LOCAL NETWORK";
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

        if( b_state == BISCUIT_CONNECTING ){
            if( QString(datagram.data()).compare(QString(COMMAND_ACK)) == 0 ){
                b_state = BISCUIT_CONNECTED;
                emit connected();
                return;
            }
        } else if( b_state == BISCUIT_STREAMING ){

            QJsonParseError jsonError;
            QJsonDocument doc = QJsonDocument::fromJson(datagram.data(), &jsonError);
            if (jsonError.error != QJsonParseError::NoError){
                qDebug() << jsonError.errorString();
            }
            QJsonObject jObject = doc.object();


            qDebug() << datagram.data();

            qDebug() << jObject["time"].toDouble();

            QJsonObject quat = jObject["quaternion"].toObject();


            QQuaternion q(
                    quat["w"].toDouble(),
                    quat["x"].toDouble(),
                    quat["y"].toDouble(),
                    quat["z"].toDouble());

            qDebug() << "EULER: " << q.toEulerAngles().x() << " " << q.toEulerAngles().y() << " " << q.toEulerAngles().z();

//            qDebug() << "W:" << quat["w"].toDouble();
//            qDebug() << "X:" << quat["x"].toDouble();
//            qDebug() << "Y:" << quat["y"].toDouble();
//            qDebug() << "Z:" << quat["z"].toDouble();



        }

    }
}

void RUFBiscuitIMUObject::configureStream()
{
    // SEND BISCUIT AN IP ADDRESS THAT IT WILL USE TO SEND IMU DATA TO
    QString cmd = COMMAND_CONFIGURE;
    cmd += myAddress.toString();

    // SEND CONFIGURE COMMAND AND WAIT FOR RESPONSE
    socket->write(cmd.toStdString().c_str());
    b_state = BISCUIT_CONNECTING;

    qDebug() << "CONTACTING BISCUIT FOR CONFIGURATION";
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
