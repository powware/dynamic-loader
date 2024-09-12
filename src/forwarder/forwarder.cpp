#include <QApplication>

#include "ui\mainwindow.h"
#include "loader_interface.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto loader = LoaderInterface::Create();
    if (!loader)
    {
        return EXIT_FAILURE;
    }

    MainWindow main_window(loader.get());
    main_window.show();

    return application.exec();
}