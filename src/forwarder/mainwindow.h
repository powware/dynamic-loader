#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>
#include <QMessageBox>
#include <string>

#include "ui_mainwindow.h"
#include "coms.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(LoaderInterface *loader, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void UpdateTooltip(const QString &n);
    void BrowseFiles();
    void PopulatePopup();
    void Inject();

private:
    Ui_MainWindow *ui_;
    QMessageBox error_message_;
    QMessageBox success_message_;
    LoaderInterface *loader_;
};

#endif // __MAINWINDOW_H__