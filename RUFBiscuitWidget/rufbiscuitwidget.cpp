#include "rufbiscuitwidget.h"

RUFBiscuitWidget::RUFBiscuitWidget(QWidget *parent) : QWidget(parent), imuObject(NULL), streamButton(NULL)
{
    this->setLayout(new QVBoxLayout());
    streamButton = new QPushButton(QString("Start Stream"));
    streamButton->setDisabled(true);
    this->layout()->addWidget(streamButton);

    imuObject = new RUFBiscuitIMUObject(this);

    connect(streamButton, SIGNAL(clicked()), imuObject, SLOT(onToggleStream()));
    connect(imuObject, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(imuObject, SIGNAL(streamStarted()), this, SLOT(onStreamStart()));
    connect(imuObject, SIGNAL(streamStopped()), this, SLOT(onStreamStop()));
}

RUFBiscuitWidget::~RUFBiscuitWidget()
{
    if( streamButton ) delete streamButton;
    if( imuObject ) delete imuObject;
}

void RUFBiscuitWidget::onConnected()
{
    qDebug() << "BISCUIT FOUND ON NETWORK";
    streamButton->setEnabled(true);
}

void RUFBiscuitWidget::onStreamStart()
{
    streamButton->setText(QString("Stop Stream"));
}

void RUFBiscuitWidget::onStreamStop()
{
    streamButton->setText(QString("Start Stream"));
}
