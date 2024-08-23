#include "mainwindow.h"

#include "pfw.h"

#include "processselector.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui_MainWindow)
{
    ui->setupUi(this);

    connect(ui->inject_button, &QPushButton::clicked, this, &MainWindow::Inject);
    connect(ui->process_selector, &QComboBox::activated, this, &MainWindow::ProcessSelectorActivated);
    connect(ui->process_selector, &ProcessSelector::popup, this, &MainWindow::ProcessSelectorPopup);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ProcessSelectorActivated()
{
}

void MainWindow::ProcessSelectorPopup()
{
    std::vector<QString> process_names;
    PROCESSENTRY32 current;
    current.dwSize = sizeof(PROCESSENTRY32);
    auto process_snapshot = pfw::MakeValidHandle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

    if (!Process32First(process_snapshot, &current))
    {
        throw std::runtime_error("derive this error");
    }
    do
    {
        process_names.push_back(QString::fromWCharArray(current.szExeFile));
    } while (Process32Next(process_snapshot, &current));

    std::sort(process_names.begin(), process_names.end());

    auto last = std::unique(process_names.begin(), process_names.end());

    process_names.erase(last, process_names.end());

    for (auto &process_name : process_names)
    {
        ui->process_selector->addItem(std::move(process_name));
    }
}

void MainWindow::Inject()
{
    auto data = ui->process_selector->currentData();
    // injector::Inject(L"ac_client.exe", L"C:\\Users\\powware\\repos\\assaultcube\\build\\Debug\\assaultcube.dll");
}
