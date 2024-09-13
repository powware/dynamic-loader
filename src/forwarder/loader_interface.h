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
#include <unordered_map>
#include <variant>

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

    bool Load(uint8_t seq, DWORD process_id, std::wstring dll)
    {
        auto buffer = Serializer().serialize(RemoteProcedure::Load).serialize(seq).serialize(process_id).serialize(dll).buffer();
        return write_pipe_->Write(buffer);
    }

    bool Unload(uint8_t seq, DWORD process_id, HANDLE module)
    {
        auto buffer = Serializer().serialize(RemoteProcedure::Unload).serialize(seq).serialize(process_id).serialize(module).buffer();
        return write_pipe_->Write(buffer);
    }

    void RegisterReadCallback(std::function<void(std::span<std::byte>)> read_callback)
    {
        read_callback_ = read_callback;
    }

private:
    pfw::Process process_;

    std::jthread thread_;

    std::unique_ptr<WritePipe> write_pipe_; // destory before read thread

    std::function<void(std::span<std::byte>)> read_callback_;
    bool Write(std::span<std::byte> buffer)
    {
        return write_pipe_->Write(buffer);
    }

    Loader(pfw::Process process, std::unique_ptr<ReadPipe> &&read_pipe, std::unique_ptr<WritePipe> &&write_pipe) : process_(std::forward<pfw::Process>(process)), thread_(std::bind_front(&Loader::ReadThread, this, std::forward<std::unique_ptr<ReadPipe>>(read_pipe))), write_pipe_(std::forward<std::unique_ptr<WritePipe>>(write_pipe)) {}

    void ReadThread(std::unique_ptr<ReadPipe> read_pipe)
    {
        while (auto buffer = read_pipe->Read())
        {
            read_callback_(*buffer);
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

        auto loader32 = Loader::Create(std::move(executable) + L"32.exe");

        return std::unique_ptr<LoaderInterface>(new LoaderInterface(std::move(loader32), std::move(loader)));
    }

    void Load(DWORD process_id, std::wstring dll, std::function<void(std::optional<HMODULE>)> callback)
    {
        auto seq = InsertCallback(std::move(callback));
        auto loader = GetLoader(process_id);
        if (!loader)
        {
            InvokeCallback<std::optional<HMODULE>>(seq, std::nullopt);
            return;
        }

        if (!loader->Load(seq, process_id, dll))
        {
            InvokeCallback<std::optional<HMODULE>>(seq, std::nullopt);
        }
    }

    void Unload(DWORD process_id, HMODULE module, std::function<void(bool)> callback)
    {
        auto seq = InsertCallback(std::move(callback));
        auto loader = GetLoader(process_id);
        if (!loader)
        {
            InvokeCallback(seq, false);
            return;
        }

        if (!loader->Unload(seq, process_id, module))
        {
            InvokeCallback(seq, false);
        }
    }

    void RegisterCloseProcessCallback(std::function<void(DWORD)> callback)
    {
        close_process_callback_ = callback;
    }

private:
    LoaderInterface(std::unique_ptr<Loader> &&loader32, std::unique_ptr<Loader> &&loader) : loader32_(std::forward<std::unique_ptr<Loader>>(loader32)), loader_(std::forward<std::unique_ptr<Loader>>(loader))
    {
        if (loader_)
        {
            loader_->RegisterReadCallback(std::bind_front(&LoaderInterface::ReadCallback, this));
        }

        if (loader32_)
        {
            loader32_->RegisterReadCallback(std::bind_front(&LoaderInterface::ReadCallback, this));
        }
    }

    void ReadCallback(std::span<std::byte> buffer)
    {
        Deserializer deserializer(buffer);
        RemoteProcedure rpc;
        uint8_t seq;
        deserializer.deserialize(rpc);
        deserializer.deserialize(seq);
        switch (rpc)
        {
        case RemoteProcedure::Load:
        {
            std::optional<HMODULE> module;
            deserializer.deserialize(module);
            InvokeCallback(seq, module);
        }
        break;
        case RemoteProcedure::Unload:
        {
            bool success;
            deserializer.deserialize(success);
            InvokeCallback(seq, success);
        }
        break;
        default:
        {
        }
        break;
        }
    }

    Loader *GetLoader(DWORD process_id)
    {
        auto is_32bit = pfw::IsProcess32bit(process_id);
        if (!is_32bit)
        {
            return nullptr;
        }

        if (*is_32bit)
        {
            return loader32_.get();
        }

        return loader_.get();
    }

    std::uint8_t InsertCallback(std::variant<std::function<void(std::optional<HMODULE>)>, std::function<void(bool)>> callback)
    {
        std::scoped_lock lock(callback_mutex_);
        callbacks_.emplace(std::make_pair(sequence_number_, callback));
        return sequence_number_++;
    }

    template <typename... Args>
    void InvokeCallback(uint8_t id, Args... args)
    {
        std::scoped_lock lock(callback_mutex_);
        auto iterator = callbacks_.find(id);
        std::get<std::function<void(Args...)>>(iterator->second)(args...);
        callbacks_.erase(iterator);
    }

    std::function<void(DWORD)> close_process_callback_;

    std::unique_ptr<Loader> loader_;
    std::unique_ptr<Loader> loader32_;

    std::mutex callback_mutex_;
    std::uint8_t sequence_number_ = 0;
    std::unordered_map<std::uint8_t, std::variant<std::function<void(std::optional<HMODULE>)>, std::function<void(bool)>>> callbacks_;
};

#endif // __LOADER_INTERFACE_H__