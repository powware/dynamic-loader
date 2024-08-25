#include "mainwindow.h"

#include <format>

#include "pfw.h"

#include "processselector.h"

MainWindow::MainWindow(QString application_directory, QWidget *parent)
    : QMainWindow(parent), ui_(new Ui_MainWindow), error_message_(this)
{
    ui_->setupUi(this);
    error_message_.setWindowTitle("Error");
    error_message_.setModal(true);
    auto check_box = error_message_.findChild<QCheckBox *>();
    if (check_box)
    {
        check_box->setVisible(false);
    }

    connect(ui_->inject_button, &QPushButton::clicked, this, &MainWindow::Inject);
    connect(ui_->process_selector, &ProcessSelector::popup, this, &MainWindow::ProcessSelectorPopup);

    auto incomplete = (application_directory + "/portinjector").toStdWString();

    for (auto &c : incomplete)
    {
        if (c == L'/')
        {
            c = L'\\';
        }
    }

    portinjector32_ = incomplete + L"32.exe";
    portinjector64_ = incomplete + L"64.exe";

    ProcessSelectorPopup();
}

MainWindow::~MainWindow()
{
    delete ui_;
}

void MainWindow::ProcessSelectorPopup()
{
    auto process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (process_snapshot == INVALID_HANDLE_VALUE)
    {
        error_message_.showMessage("CreateToolhelp32Snapshot failed.");
        return;
    }

    PROCESSENTRY32 current;
    current.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(process_snapshot, &current))
    {
        error_message_.showMessage("Process32First failed.");
        return;
    }

    std::vector<std::tuple<QString, DWORD>> processes;
    do
    {
        QString process_name(QString::fromWCharArray(current.szExeFile));
        if (process_name.toLower().endsWith(".exe"))
        {
            processes.push_back(std::make_tuple(std::move(process_name), current.th32ProcessID));
        }
    } while (Process32Next(process_snapshot, &current));

    std::sort(processes.begin(), processes.end(), [](const auto &lhs, const auto &rhs)
              { return std::get<0>(lhs).toLower() < std::get<0>(rhs).toLower(); });

    auto last = std::unique(processes.begin(), processes.end());

    processes.erase(last, processes.end());

    ui_->process_selector->clear();

    for (auto &process_name : processes)
    {
        ui_->process_selector->addItem(std::get<0>(process_name), uint(std::get<1>(process_name)));
    }
}

void MainWindow::Inject()
{
    DWORD process_id = ui_->process_selector->currentData().toUInt();
    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, process_id);
    if (process_handle == NULL)
    {
        error_message_.showMessage("OpenProcess failed.");
        return;
    }
    pfw::HandleGuard process_handle_guard(process_handle);
    BOOL is_32bit;
    if (!IsWow64Process(process_handle, &is_32bit))
    {
        error_message_.showMessage("IsWow64Process failed.");
        return;
    }
    const auto &selected_portinjector = [this, is_32bit]() -> auto &
    { return is_32bit ? portinjector32_ : portinjector64_; }();

    auto command_line = std::format(L"-p {}", process_id);

    STARTUPINFO startup_info = {.cb = sizeof(startup_info)};
    PROCESS_INFORMATION process_info;
    if (!CreateProcess(selected_portinjector.c_str(), command_line.data(), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info))
    {
        error_message_.showMessage("CreateProcess failed.");
    }
    pfw::HandleGuard injector_process_handle_guard(process_info.hProcess);
    pfw::HandleGuard injector_thread_handle(process_info.hThread);
    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code;
    if (!GetExitCodeProcess(process_info.hProcess, &exit_code))
    {
        error_message_.showMessage("GetExitCodeProcess failed.");
    }

    if (exit_code)
    {
        error_message_.showMessage("Injection failed.");
    }
    // injector::Inject(L"ac_client.exe", L"C:\\Users\\powware\\repos\\assaultcube\\build\\Debug\\assaultcube.dll");
}
