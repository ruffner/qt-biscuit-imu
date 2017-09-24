#include "rufbiscuitwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    RUFBiscuitDialog w;
    return w.exec();

}
