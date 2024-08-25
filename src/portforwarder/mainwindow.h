#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>
#include <QErrorMessage>
#include <string>

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString application_directory, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void ProcessSelectorPopup();
    void Inject();

private:
    Ui_MainWindow *ui_;
    QErrorMessage error_message_;
    std::wstring portinjector32_;
    std::wstring portinjector64_;
};

#endif // __MAINWINDOW_H__