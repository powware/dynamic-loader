#include "mainwindow.h"

#include <windows.h>
#include <shellapi.h>
#include <format>

#include <QFileDialog>

#include <pfw.h>

#include "processselector.h"

MainWindow::MainWindow(QString application_directory, QWidget *parent)
    : QMainWindow(parent), ui_(new Ui_MainWindow), error_message_(this)
{
    ui_->setupUi(this);
    error_message_.setWindowTitle("Error");
    error_message_.setModal(true);
    error_message_.setIcon(QMessageBox::Critical);
    auto check_box = error_message_.findChild<QCheckBox *>();
    if (check_box)
    {
        check_box->setVisible(false);
    }

    success_message_.setWindowTitle("Success");
    success_message_.setIcon(QMessageBox::Information);
    success_message_.setModal(true);

    connect(ui_->file_selector, &QLineEdit::textChanged, this, &MainWindow::UpdateTooltip);
    connect(ui_->file_browser, &QToolButton::clicked, this, &MainWindow::BrowseFiles);
    connect(ui_->process_selector, &ProcessSelector::popup, this, &MainWindow::PopulatePopup);
    connect(ui_->inject_button, &QPushButton::clicked, this, &MainWindow::Inject);

    PopulatePopup();

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
}

MainWindow::~MainWindow()
{
    delete ui_;
}

std::optional<QIcon> QIconFromProcessId(DWORD process_id)
{

    auto exe_path = pfw::ExecutablePathFromProcessId(process_id);
    if (!exe_path)
    {
        return std::nullopt;
    }

    SHFILEINFO shFileInfo;
    if (!SHGetFileInfo(exe_path->c_str(), 0, &shFileInfo, sizeof(shFileInfo), SHGFI_ICON | SHGFI_LARGEICON))
    {
        return std::nullopt;
    }

    ICONINFO iconInfo;
    if (!GetIconInfo(shFileInfo.hIcon, &iconInfo))
    {
        return std::nullopt;
    }

    BITMAP bm;
    if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bm))
    {
        return std::nullopt;
    }

    int width = bm.bmWidth;
    int height = bm.bmHeight;

    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    HDC hdc = CreateCompatibleDC(nullptr);
    SelectObject(hdc, iconInfo.hbmColor);

    BITMAPINFOHEADER bi;
    memset(&bi, 0, sizeof(bi));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Negative indicates a top-down DIB
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    GetDIBits(hdc, iconInfo.hbmColor, 0, height, image.bits(), reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    DeleteDC(hdc);

    return QIcon(QPixmap::fromImage(image));
}

void MainWindow::UpdateTooltip(const QString &n)
{
    ui_->file_selector->setToolTip(n);
}

void MainWindow::BrowseFiles()
{
    ui_->file_selector->setText(QFileDialog::getOpenFileName(this, "Select DLL", "", "(*.dll)"));
}

void MainWindow::PopulatePopup()
{
    struct Process
    {
        QString name;
        std::optional<QIcon> icon;
    };

    auto process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (process_snapshot == INVALID_HANDLE_VALUE)
    {
        error_message_.setText("CreateToolhelp32Snapshot failed.");
        return;
    }

    PROCESSENTRY32 current;
    current.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(process_snapshot, &current))
    {
        error_message_.setText("Process32First failed.");
        return;
    }

    std::vector<Process> processes;
    do
    {
        QString process_name(QString::fromWCharArray(current.szExeFile));
        if (process_name.toLower().endsWith(".exe"))
        {

            processes.push_back({std::move(process_name), QIconFromProcessId(current.th32ProcessID)});
        }
    } while (Process32Next(process_snapshot, &current));

    std::sort(processes.begin(), processes.end(), [](const auto &lhs, const auto &rhs)
              { return lhs.name.toLower() < rhs.name.toLower(); });

    auto last = std::unique(processes.begin(), processes.end(), [](const auto &lhs, const auto &rhs)
                            { return lhs.name == rhs.name; });

    processes.erase(last, processes.end());

    ui_->process_selector->clear();

    for (const auto &process : processes)
    {
        if (process.icon)
        {
            ui_->process_selector->addItem(*process.icon, process.name);
        }
        else
        {
            ui_->process_selector->addItem(process.name); // Add without icon if something went wrong
        }
    }
}

void MainWindow::Inject()
{
    auto process_id = pfw::GetProcessId(ui_->process_selector->currentText().toStdWString());
    if (!process_id)
    {
        error_message_.setText("GetProcessId failed.");
        error_message_.show();
        return;
    }

    auto process = pfw::OpenProcess(*process_id, PROCESS_QUERY_LIMITED_INFORMATION);
    if (!process)
    {
        error_message_.setText("OpenProcess failed.");
        error_message_.show();
        return;
    }

    BOOL is_32bit;
    if (!IsWow64Process(*process, &is_32bit))
    {
        error_message_.setText("IsWow64Process failed.");
        error_message_.show();
        return;
    }
    const auto &selected_portinjector = is_32bit ? portinjector32_ : portinjector64_;

    std::wstring dll_path = ui_->file_selector->text().toStdWString();

    auto command_line = std::format(L"--pid {} --dll {} --load", *process_id, dll_path);

    struct RemoteProcess
    {
        pfw::HandleGuard process;
        pfw::HandleGuard thread;
    };

    auto injector_process = [&selected_portinjector, &command_line]() -> std::optional<RemoteProcess>
    {
        STARTUPINFO startup_info = {.cb = sizeof(startup_info)};
        PROCESS_INFORMATION process_info;
        if (!CreateProcess(selected_portinjector.c_str(), command_line.data(), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info))
        {
            return std::nullopt;
        }
        auto process = pfw::HandleGuard::Create(process_info.hProcess);
        auto thread = pfw::HandleGuard::Create(process_info.hThread);
        if (!process || !thread)
        {
            return std::nullopt;
        }
        return RemoteProcess(std::move(*process), std::move(*thread));
    }();
    if (!injector_process)
    {
        error_message_.setText("CreateProcess failed.");
    }
    WaitForSingleObject(injector_process->process, INFINITE);

    DWORD exit_code;
    if (!GetExitCodeProcess(injector_process->process, &exit_code))
    {
        error_message_.setText("GetExitCodeProcess failed.");
        error_message_.show();
    }

    if (exit_code)
    {
        error_message_.setText("Injection failed.");
        error_message_.show();
        return;
    }

    success_message_.setText("Injection successfull.");
    success_message_.show();
}
