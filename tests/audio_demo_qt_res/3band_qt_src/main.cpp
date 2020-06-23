/*
 ** Copyright (c) 2019, Analog Devices, Inc.  All rights reserved.
*/
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
