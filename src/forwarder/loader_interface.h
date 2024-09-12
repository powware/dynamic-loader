#ifndef __LOADER_INTERFACE_H__
#define __LOADER_INTERFACE_H__

#include <string>
#include <filesystem>
#include <optional>
#include <array>
#include <span>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <Windows.h>

#include <pfw.h>

#include "pipe.h"
#include "serialization.h"

class Loader
{
public:
    static std::unique_ptr<Loader> Create(std::wstring executable)
    {
        SECURITY_ATTRIBUTES security_attributes{.nLength = sizeof(security_attributes), .bInheritHandle = true};

        HANDLE read_pipe_read_handle;
        HANDLE read_pipe_write_handle;

        if (!CreatePipe(&read_pipe_read_handle, &read_pipe_write_handle, &security_attributes, 0))
        {
            return nullptr;
        }

        if (!SetHandleInformation(read_pipe_read_handle, HANDLE_FLAG_INHERIT, 0))
        {
            return nullptr;
        }

        HANDLE write_pipe_read_handle;
        HANDLE write_pipe_write_handle;

        if (!CreatePipe(&write_pipe_read_handle, &write_pipe_write_handle, &security_attributes, 0))
        {
            return nullptr;
        }

        if (!SetHandleInformation(write_pipe_write_handle, HANDLE_FLAG_INHERIT, 0))
        {
            return nullptr;
        }

        STARTUPINFO startup_info = {.cb = sizeof(startup_info), .dwFlags = STARTF_USESTDHANDLES, .hStdInput = write_pipe_read_handle, .hStdOutput = read_pipe_write_handle};
        auto process = pfw::CreateProcess(std::move(executable), &security_attributes, startup_info);
        if (!process)
        {
            return nullptr;
        }

        CloseHandle(read_pipe_write_handle);
        CloseHandle(write_pipe_read_handle);

        auto read_handle = pfw::Handle::Create(read_pipe_read_handle);
        auto write_handle = pfw::Handle::Create(write_pipe_write_handle);

        return std::unique_ptr<Loader>(new Loader(std::move(*process), std::make_unique<ReadPipe>(std::move(*read_handle)), std::make_unique<WritePipe>(std::move(*write_handle))));
    }

    bool Load(DWORD process_id, std::wstring dll)
    {
        auto buffer = Serializer().serialize(RemoteProcedure::Load).serialize(process_id).serialize(dll).buffer();
        return write_pipe_->Write(buffer);
    }

    bool Unload(DWORD process_id, HANDLE module)
    {
        auto buffer = Serializer().serialize(RemoteProcedure::Unload).serialize(process_id).serialize(module).buffer();
        return false;
    }

    void RegisterReadCallback(std::function<void(std::span<std::byte>)> Read_callback)
    {
        Read_callback_ = Read_callback;
    }

private:
    pfw::Process process_;

    // std::mutex mutex_;
    // std::condition_variable_any cv_;
    std::jthread thread_;

    std::unique_ptr<WritePipe> write_pipe_; // destory before read thread

    std::function<void(std::span<std::byte>)> Read_callback_;
    bool Write(std::span<std::byte> buffer)
    {
        return write_pipe_->Write(buffer);
    }

    Loader(pfw::Process process, std::unique_ptr<ReadPipe> &&read_pipe, std::unique_ptr<WritePipe> &&write_pipe) : process_(std::forward<pfw::Process>(process)), thread_(std::bind_front(&Loader::ReadThread, this, std::forward<std::unique_ptr<ReadPipe>>(read_pipe))), write_pipe_(std::forward<std::unique_ptr<WritePipe>>(write_pipe)) {}

    void ReadThread(std::unique_ptr<ReadPipe> read_pipe)
    {
        while (auto buffer = read_pipe->Read())
        {
            Read_callback_(*buffer);
        }
    }
};

struct Operation
{
    RemoteProcedure rpc;
    DWORD process_id;
    std::vector<std::byte> buffer;
};

class LoaderInterface
{
public:
    static std::unique_ptr<LoaderInterface> Create()
    {
        auto process = pfw::GetModuleFileName(nullptr);

        auto executable = std::filesystem::path(process).parent_path().wstring() + L"\\loader";

        auto loader = Loader::Create(executable + L".exe");
        if (!loader)
        {
            return nullptr;
        }

        // auto loader32 = Loader::Create(std::move(executable) + L"32.exe");
        // if (!loader32)
        // {
        //     return nullptr;
        // }
        std::unique_ptr<Loader> loader32(nullptr);

        return std::unique_ptr<LoaderInterface>(new LoaderInterface(std::move(loader32), std::move(loader)));
    }

    void Load(DWORD process_id, std::wstring dll)
    {
        auto loader = GetLoader(process_id);
        if (!loader)
        {
            return;
        }

        (*loader)->Load(process_id, dll);
    }

    void Unload(DWORD process_id, HMODULE module)
    {
        auto loader = GetLoader(process_id);
        if (!loader)
        {
            return;
        }

        (*loader)->Unload(process_id, module);
    }

    void RegisterLoadCallback(std::function<void(std::optional<HMODULE>)> callback)
    {
        load_callback_ = callback;
    }

    void RegisterUnloadCallback(std::function<void(bool)> callback)
    {
        unload_callback_ = callback;
    }

    void RegisterCloseProcessCallback(std::function<void(DWORD)> callback)
    {
        close_process_callback_ = callback;
    }

private:
    LoaderInterface(std::unique_ptr<Loader> &&loader32, std::unique_ptr<Loader> &&loader) : loader32_(std::forward<std::unique_ptr<Loader>>(loader32)), loader_(std::forward<std::unique_ptr<Loader>>(loader))
    {
        loader_->RegisterReadCallback(std::bind_front(&LoaderInterface::ReadCallback, this));
        // loader32_->RegisterReadCallback(std::bind_front(&LoaderInterface::ReadCallback, this));
    }

    void ReadCallback(std::span<std::byte> buffer)
    {
        Deserializer deserializer(buffer);
        RemoteProcedure rpc;
        deserializer.deserialize(rpc);
        switch (rpc)
        {
        case RemoteProcedure::Load:
        {
            std::optional<HMODULE> t;
            deserializer.deserialize(t);
            load_callback_(t);
        }
        break;
        case RemoteProcedure::Unload:
        {
            bool success;
            deserializer.deserialize(success);
            unload_callback_(success);
        }
        break;
        default:
        {
        }
        break;
        }
    }

    std::optional<Loader *>
    GetLoader(DWORD process_id)
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

        return loader_.get();
    }

    std::function<void(std::optional<HMODULE>)> load_callback_;
    std::function<void(bool)> unload_callback_;
    std::function<void(DWORD)> close_process_callback_;

    std::unique_ptr<Loader> loader_;
    std::unique_ptr<Loader> loader32_;
};

#endif // __LOADER_INTERFACE_H__