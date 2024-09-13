#include "mainwindow.h"

#include <format>

#include <shellapi.h>

#include <QFileDialog>

#include <pfw.h>

#include "icon_helpers.h"
#include "load_event.h"
#include "processselector.h"
#include "unload_event.h"

MainWindow::MainWindow(LoaderInterface *loader, QWidget *parent)
    : loader_(loader), QMainWindow(parent), ui_(new Ui_MainWindow), error_message_(this)
{
    ui_->setupUi(this);
    ui_->load->setEnabled(false);
    ui_->load->setStyleSheet("QPushButton { color: grey; background-color: lightgrey; }");
    ui_->unload->setEnabled(false);
    ui_->unload->setStyleSheet("QPushButton { color: grey; background-color: lightgrey; }");

    error_message_.setWindowTitle("Error");
    error_message_.setModal(false);
    error_message_.setIcon(QMessageBox::Critical);
    auto check_box = error_message_.findChild<QCheckBox *>();
    if (check_box)
    {
        check_box->setVisible(false);
    }

    success_message_.setWindowTitle("Success");
    success_message_.setIcon(QMessageBox::Information);
    success_message_.setModal(false);

    connect(ui_->file_selector, &QLineEdit::textChanged, this, &MainWindow::FileSelectorTextChanged);
    connect(ui_->file_browser, &QToolButton::clicked, this, &MainWindow::FileBrowserClicked);
    connect(ui_->process_selector, &ProcessSelector::popup, this, &MainWindow::ProcessSelectorPopup);
    connect(ui_->load, &QPushButton::clicked, this, &MainWindow::LoadClicked);
    connect(ui_->unload, &QPushButton::clicked, this, &MainWindow::UnloadClicked);
    connect(ui_->module_list, &QTreeWidget::currentItemChanged, this, &MainWindow::ModuleListCurrentItemChanged);

    ProcessSelectorPopup();

    dll_icon_ = LoadIconFromImageres(67);             // Computer\HKEY_CLASSES_ROOT\dllfile\DefaultIcon
    default_process_icon_ = LoadIconFromImageres(15); // manually looked in a file, but might change from sys to sys
}

MainWindow::~MainWindow()
{
    delete ui_;
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == LoadEventType)
    {
        loading_ = false;
        auto load_event = static_cast<LoadEvent *>(event);
        if (!load_event->module)
        {
            error_message_.setText("Load failed.");
            error_message_.show();
            return true;
        }

        const auto module = "0x" + QString::number(reinterpret_cast<std::uint64_t>(*load_event->module), 16);

        auto process = [=, this]
        {
            auto results = ui_->module_list->findItems(QString::number(load_event->process_id), Qt::MatchExactly, 1);
            return !results.isEmpty() ? results[0] : nullptr;
        }();
        if (!process)
        {
            auto is_32bit = pfw::IsProcess32bit(load_event->process_id);
            if (!is_32bit)
            {
                error_message_.setText("IDKKKKKKKKKKKKKKKK");
                error_message_.show();
                return true;
            }

            process = new QTreeWidgetItem({load_event->process,
                                           QString::number(load_event->process_id),
                                           *is_32bit ? "x32" : "x64",
                                           "Process"});
            if (load_event->process_icon.isNull() && default_process_icon_)
            {
                process->setIcon(0, *default_process_icon_);
            }
            else
            {
                process->setIcon(0, load_event->process_icon);
            }
            ui_->module_list->addTopLevelItem(process);
            process->setExpanded(true);
        }
        else
        {
            const auto count = process->childCount();
            for (int i = 0; i < count; ++i)
            {
                if (process->child(i)->text(1) == module)
                {
                    return true;
                }
            }
        }

        auto child = new QTreeWidgetItem({QString::fromStdWString(load_event->dll),
                                          module,
                                          "", "Module"});
        process->addChild(child);
        if (dll_icon_)
        {
            child->setIcon(0, *dll_icon_);
        }

        return true;
    }
    else if (event->type() == UnloadEventType)
    {
        unloading_ = false;
        auto unload_event = static_cast<UnloadEvent *>(event);
        if (!unload_event->success)
        {
            error_message_.setText("Unload failed.");
            error_message_.show();

            return true;
        }

        return true;
    }

    return QMainWindow::event(event);
}

void MainWindow::FileSelectorTextChanged(const QString &text)
{
    ui_->file_selector->setToolTip(text);

    if (text.isEmpty())
    {
        ui_->load->setEnabled(false);
        ui_->load->setStyleSheet("QPushButton { color: grey; background-color: lightgrey; }");
    }
    else
    {
        ui_->load->setEnabled(true);
        ui_->load->setStyleSheet("");
    }
}

void MainWindow::FileBrowserClicked()
{
    ui_->file_selector->setText(QFileDialog::getOpenFileName(this, "Select DLL", "", "(*.dll)"));
}

void MainWindow::ProcessSelectorPopup()
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

void MainWindow::LoadClicked()
{
    if (loading_)
    {
        return;
    }

    auto index = ui_->process_selector->currentIndex();

    auto process = ui_->process_selector->itemText(index);
    auto process_icon = ui_->process_selector->itemIcon(index);

    auto process_id = pfw::GetProcessId(process.toStdWString());
    if (!process_id)
    {
        error_message_.setText("GetProcessId failed.");
        error_message_.show();
        return;
    }

    std::wstring dll = ui_->file_selector->text().toStdWString();
    if (dll.empty())
    {
        error_message_.setText("No DLL entered.");
        error_message_.show();
        return;
    }

    std::wstring dll_name;
    try
    {
        dll_name = std::filesystem::path(dll).filename().wstring();
    }
    catch (std::exception &)
    {
        error_message_.setText("Reading file name failed.");
        error_message_.show();
        return;
    }

    if (ui_->dll_copy->isChecked())
    {
        auto temp_directory = std::filesystem::temp_directory_path();

        auto uuid = pfw::GenerateUUID();
        if (!uuid)
        {
            error_message_.setText("UUID creation failed.");
            error_message_.show();
            return;
        }

        temp_directory = temp_directory / (*uuid + L"_dynamic-loader");

        try
        {
            std::filesystem::create_directory(temp_directory);
            auto temp_dll = temp_directory / dll_name;
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

    loading_ = true;
    loader_->Load(*process_id, dll, [this, process_icon = std::move(process_icon), process = std::move(process), process_id = *process_id, dll_name = std::move(dll_name)](std::optional<HMODULE> module) mutable
                  { QCoreApplication::postEvent(this, new LoadEvent(std::move(process_icon), std::move(process), process_id, std::move(dll_name), module)); });
}

void MainWindow::UnloadClicked()
{
    if (unloading_)
    {
        return;
    }

    auto item = ui_->module_list->currentItem();
    auto module = item->text(1);
    auto module_handle = reinterpret_cast<HMODULE>(module.toULongLong());
    auto process = item->parent()->text(1);
    auto process_id = DWORD(process.toUInt());

    unloading_ = true;
    loader_->Unload(process_id, module_handle, [=, this](bool success)
                    { QCoreApplication::postEvent(this, new UnloadEvent(process, module, success)); });
}

void MainWindow::ModuleListCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem * /*previous*/)
{
    if (current->text(3) == "Module")
    {
        ui_->unload->setEnabled(true);
        ui_->unload->setStyleSheet("");
    }
    else
    {
        ui_->unload->setEnabled(false);
        ui_->unload->setStyleSheet("QPushButton { color: grey; background-color: lightgrey; }");
    }
}
