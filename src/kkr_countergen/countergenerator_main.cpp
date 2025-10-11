#ifndef COUNTERGENERATORMAIN_H
#define COUNTERGENERATORMAIN_H
#include <QApplication>

#include "countergenwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    CounterGenWindow cgw;
    cgw.show();
    return a.exec();
}

#endif