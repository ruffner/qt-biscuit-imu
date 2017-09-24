#ifndef RUFBISCUITWIDGET_H
#define RUFBISCUITWIDGET_H

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
#include <QJsonObject>
#include <QVariantMap>
#include <QDialogButtonBox>
#include <QAbstractSocket>
#include <QNetworkDatagram>
#include <QUdpSocket>

#include "rufbiscuitimuobject.h"

class RUFBiscuitWidget : public QWidget
{
    Q_OBJECT

public:
    RUFBiscuitWidget(QWidget *parent = 0);
    ~RUFBiscuitWidget();

private:
    RUFBiscuitIMUObject *imuObject;
    QPushButton *streamButton;

public slots:
    void onConnected();
    void onStreamStop();
    void onStreamStart();
};

class RUFBiscuitDialog : public QDialog
{
    Q_OBJECT

public:
    RUFBiscuitDialog(QWidget *parent = 0) : QDialog(parent)
    {
        this->setLayout(new QVBoxLayout());
        this->layout()->setContentsMargins(6, 6, 6, 6);

        widget = new RUFBiscuitWidget();
        this->layout()->addWidget(widget);

        QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(box->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(box->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
        this->layout()->addWidget(box);
    }

    ~RUFBiscuitDialog()
    {
        ;
    }

protected:
    void accept()
    {
        QDialog::accept();
    }

    void reject()
    {
        QDialog::reject();
    }

private:
    RUFBiscuitWidget *widget;
};

#endif // RUFBISCUITWIDGET_H
