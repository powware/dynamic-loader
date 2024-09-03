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

class Injector
{
public:
    static std::unique_ptr<Injector> Create(std::wstring executable)
    {
        auto executable_name = std::filesystem::path(executable).filename();

        auto pipe = Pipe::Create(Pipe::Server, executable_name);
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

        if (!pipe->WaitForClient())
        {
            return nullptr;
        }

        auto message = pipe->Receive();
        if (message)
        {
            std::wcout << *message << std::endl;
        }

        return std::unique_ptr<Injector>(new Injector(std::move(*process), std::move(*thread), std::move(*pipe)));
    }

    ~Injector()
    {
    }

    // Injector(Injector &&rhs) : process_(std::move(rhs.process_)), thread_(std::move(rhs.thread_)), pipe_() {}

private:
    pfw::HandleGuard process_;
    pfw::HandleGuard thread_;
    Pipe pipe_;

    Injector(pfw::HandleGuard &&process, pfw::HandleGuard &&thread, Pipe &&pipe) : process_(std::forward<pfw::HandleGuard>(process)), thread_(std::forward<pfw::HandleGuard>(thread)), pipe_(std::forward<Pipe>(pipe)) {}
};

class InjectorInterface
{
public:
    static std::unique_ptr<InjectorInterface> Create(std::wstring dir)
    {
        auto executable = dir + L"/portinjector";

        // auto portinjector32 = Injector::Create(executable + L"32.exe");
        // if (!portinjector32)
        // {
        //     return nullptr;
        // }

        auto portinjector64 = Injector::Create(executable + L"64.exe");
        if (!portinjector64)
        {
            return nullptr;
        }

        // return std::unique_ptr<InjectorInterface>(new InjectorInterface(std::move(portinjector32), std::move(portinjector64)));
        return std::unique_ptr<InjectorInterface>(new InjectorInterface(std::move(portinjector64), std::move(portinjector64)));
    }

    std::optional<HANDLE> Load(DWORD process_id, std::wstring dll) { return std::nullopt; }
    bool Unload(DWORD process_id, std::wstring dll) { return false; }

private:
    InjectorInterface(std::unique_ptr<Injector> &&portinjector32, std::unique_ptr<Injector> &&portinjector64) : portinjector32_(std::forward<std::unique_ptr<Injector>>(portinjector32)), portinjector64_(std::forward<std::unique_ptr<Injector>>(portinjector64))
    {
    }

    std::unique_ptr<Injector> portinjector32_;
    std::unique_ptr<Injector> portinjector64_;
};

#endif // __COMS_H__