#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui_MainWindow)
{
    ui->setupUi(this);

    connect(ui->inject_button, &QPushButton::clicked, this, &MainWindow::Inject);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Inject()
{
    injector::Inject(L"ac_client.exe", L"C:\\Users\\powware\\repos\\assaultcube\\build\\Debug\\assaultcube.dll");
}
