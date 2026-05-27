#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QString::fromUtf8("今天吃什么"));
    app.setApplicationVersion("2.0");

    MainWindow w;
    w.show();
    return app.exec();
}

