#include <QApplication>

#include "mainwindow.h"
#include "coms.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto injector = InjectorInterface::Create(application.applicationDirPath().toStdWString());
    if (!injector)
    {
        return EXIT_FAILURE;
    }

    MainWindow main_window(injector.get());
    main_window.show();

    return application.exec();
}