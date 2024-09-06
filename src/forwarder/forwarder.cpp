#include <QApplication>

#include "mainwindow.h"
#include "coms.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto loader = LoaderInterface::Create(application.applicationDirPath().toStdWString());
    if (!loader)
    {
        return EXIT_FAILURE;
    }

    MainWindow main_window(loader.get());
    main_window.show();

    return application.exec();
}