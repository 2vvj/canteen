#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QString::fromUtf8("寻味燕园"));
    app.setWindowIcon(QIcon("icon.png"));

    MainWindow w;
    w.show();
    return app.exec();
}

