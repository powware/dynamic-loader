#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    MainWindow main_window(application.applicationDirPath());
    main_window.show();

    return application.exec();
}