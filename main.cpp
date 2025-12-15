#include <QApplication>
#include <QMetaType>
#include <QVector>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<QVector<int>>("QVector<int>");

    MainWindow w;
    w.show();
    return app.exec();
}
