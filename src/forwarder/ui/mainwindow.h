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
    void FileSelectorTextChanged(const QString &text);
    void FileBrowserClicked();
    void ProcessSelectorPopup();
    void LoadClicked();
    void UnloadClicked();
    void ModuleListCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    Ui_MainWindow *ui_;
    QMessageBox error_message_;
    QMessageBox success_message_;
    LoaderInterface *loader_;

    std::optional<QIcon> dll_icon_;
    std::optional<QIcon> default_process_icon_;

    bool loading_ = false;
    bool unloading_ = false;
};

#endif // __MAINWINDOW_H__