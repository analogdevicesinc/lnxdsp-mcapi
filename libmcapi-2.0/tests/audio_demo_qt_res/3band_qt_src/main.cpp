#include <QtGui>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Main w;
    w.setWindowFlags(Qt::FramelessWindowHint);
    w.showMaximized();

    return a.exec();
}
