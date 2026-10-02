#include <QApplication>
#include <QStyleFactory>
#include "ui/mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("SOM");
    QCoreApplication::setApplicationName("SOM");

    MainWindow w;
    w.show();

    return app.exec();
}
