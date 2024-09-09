#include "mainwindow.h"

#include <windows.h>
#include <shellapi.h>
#include <format>

#include <QFileDialog>

#include <pfw.h>

#include "processselector.h"

MainWindow::MainWindow(LoaderInterface *loader, QWidget *parent)
    : loader_(loader), QMainWindow(parent), ui_(new Ui_MainWindow), error_message_(this)
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

    std::wstring dll = ui_->file_selector->text().toStdWString();

    if (ui_->dll_copy->isChecked())
    {
        auto temp_directory = std::filesystem::temp_directory_path();

        auto uuid = []() -> std::optional<std::wstring>
        {
            UUID uuid;
            RPC_WSTR uuid_string;
            if (UuidCreate(&uuid))
            {
                return std::nullopt;
            }
            if (UuidToStringW(&uuid, &uuid_string))
            {
                return std::nullopt;
            }
            std::wstring result(reinterpret_cast<wchar_t *>(uuid_string));
            RpcStringFreeW(&uuid_string);
            return result;
        }();
        if (!uuid)
        {
            error_message_.setText("UUID creation failed.");
            error_message_.show();
            return;
        }

        temp_directory = temp_directory / (*uuid + L"_dynamic-linker");

        try
        {
            std::filesystem::create_directory(temp_directory);
            auto temp_dll = temp_directory / std::filesystem::path(dll).filename();
            std::filesystem::copy_file(dll, temp_dll);
            dll = temp_dll.wstring();
        }
        catch (std::exception &)
        {
            error_message_.setText("Creating dll copy failed.");
            error_message_.show();
            return;
        }
    }

    auto module = loader_->Load(*process_id, dll);
    if (!module)
    {
        error_message_.setText("Injection failed.");
        error_message_.show();
        return;
    }

    success_message_.setText("Injection successfull.");
    success_message_.show();
}
