#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>

#include "ui_mainwindow.h"

namespace ui
{
    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private slots:
        void Inject();

    private:
        Ui_MainWindow *ui;
    };
}

#endif // __MAINWINDOW_H__