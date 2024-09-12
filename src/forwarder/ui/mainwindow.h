#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QMainWindow>
#include <QMessageBox>
#include <string>

#include "ui_mainwindow.h"
#include "..\loader_interface.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(LoaderInterface *loader, QWidget *parent = nullptr);
    ~MainWindow();

    bool event(QEvent *event) override;

private slots:
    void UpdateTooltip(const QString &text);
    void BrowseFiles();
    void PopulatePopup();
    void Load();
    void Unload();

private:
    Ui_MainWindow *ui_;
    QMessageBox error_message_;
    QMessageBox success_message_;
    LoaderInterface *loader_;

    bool loading_ = false;
    bool unloading_ = false;
};

#endif // __MAINWINDOW_H__