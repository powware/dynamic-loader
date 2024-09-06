#ifndef __COMS_H__
#define __COMS_H__

#include <string>
#include <filesystem>
#include <optional>
#include <array>
#include <iostream>

#include <Windows.h>

#include <pfw.h>

#include "pipe.h"

class Loader
{
public:
    static std::unique_ptr<Loader> Create(std::wstring executable)
    {
        auto executable_name = std::filesystem::path(executable).filename();

        auto pipe = Pipe::Create(executable_name);
        if (!pipe)
        {
            return nullptr;
        }

        STARTUPINFO startup_info = {.cb = sizeof(startup_info)};
        PROCESS_INFORMATION process_info;
        if (!CreateProcess(executable.c_str(), nullptr, nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info))
        {
            return nullptr;
        }

        auto process = pfw::HandleGuard::Create(process_info.hProcess);
        auto thread = pfw::HandleGuard::Create(process_info.hThread);
        if (!process || !thread)
        {
            return nullptr;
        };

        // if (!pipe->WaitForClient())
        // {
        //     return nullptr;
        // }

        pipe->Send(L"hello!");

        auto message = pipe->Receive();
        if (message)
        {
            std::wcout << *message << std::endl;
        }

        return std::unique_ptr<Loader>(new Loader(std::move(*process), std::move(*thread), std::move(pipe)));
    }

    std::optional<HANDLE> Load(DWORD process_id, std::wstring dll)
    {
        return std::nullopt;
    }

    bool Unload(DWORD process_id, HANDLE module)
    {
        return false;
    }

    ~Loader()
    {
        // WaitForSingleObject(process_, INFINITE);

        // DWORD exit_code;
        // if (!GetExitCodeProcess(loader_process->process, &exit_code))
        // {
        //     error_message_.setText("GetExitCodeProcess failed.");
        //     error_message_.show();
        // }

        // if (exit_code)
        // {
        //     error_message_.setText("Injection failed.");
        //     error_message_.show();
        //     return;
        // }

        TerminateProcess(process_, 0);
    }

    // Loader(Loader &&rhs) : process_(std::move(rhs.process_)), thread_(std::move(rhs.thread_)), pipe_() {}

private:
    pfw::HandleGuard process_;
    pfw::HandleGuard thread_;
    std::unique_ptr<Pipe::Server> pipe_;

    Loader(pfw::HandleGuard &&process, pfw::HandleGuard &&thread, std::unique_ptr<Pipe::Server> &&pipe) : process_(std::forward<pfw::HandleGuard>(process)), thread_(std::forward<pfw::HandleGuard>(thread)), pipe_(std::forward<std::unique_ptr<Pipe::Server>>(pipe)) {}
};

class LoaderInterface
{
public:
    static std::unique_ptr<LoaderInterface> Create(std::wstring loader_directory)
    {
        auto executable = loader_directory + L"/loader";

        auto loader32 = Loader::Create(executable + L"32.exe");
        if (!loader32)
        {
            // return nullptr; // TODO remove
        }

        auto loader64 = Loader::Create(executable + L"64.exe");
        if (!loader64)
        {
            return nullptr;
        }

        return std::unique_ptr<LoaderInterface>(new LoaderInterface(std::move(loader32), std::move(loader64)));
    }

    std::optional<HANDLE> Load(DWORD process_id, std::wstring dll)
    {
        auto loader = GetLoader(process_id);
        if (!loader)
        {
            return std::nullopt;
        }

        return (*loader)->Load(process_id, dll);
    }

    bool Unload(DWORD process_id, HANDLE module)
    {
        auto loader = GetLoader(process_id);
        if (!loader)
        {
            return false;
        }

        return (*loader)->Unload(process_id, module);
    }

private:
    LoaderInterface(std::unique_ptr<Loader> &&loader32, std::unique_ptr<Loader> &&loader64) : loader32_(std::forward<std::unique_ptr<Loader>>(loader32)), loader64_(std::forward<std::unique_ptr<Loader>>(loader64)) {}

    std::optional<Loader *> GetLoader(DWORD process_id)
    {
        auto is_32bit = pfw::IsProcess32bit(process_id);
        if (!is_32bit)
        {
            return std::nullopt;
        }

        if (*is_32bit)
        {
            return loader32_.get();
        }

        return loader64_.get();
    }

    std::unique_ptr<Loader> loader32_;
    std::unique_ptr<Loader> loader64_;
};

#endif // __COMS_H__