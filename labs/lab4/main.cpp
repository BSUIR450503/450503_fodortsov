#include <QCoreApplication>
#include <QTextStream>
#include <iostream>
#include <unistd.h>
#include "Controller.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    int c;
    int cont=1;
    while(cont)
    {
        c = getchar();
        switch (c) {
        case '+':
            Controller::createOutputThread(0);
            break;
        case '-':
            Controller::endLastProcess();
            break;
        case 'q':
            Controller::endAll();
            cont=0;
            break;
        }
    }
    return 0;
}
