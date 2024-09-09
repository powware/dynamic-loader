#ifndef __COMS_H__
#define __COMS_H__

#include <string>
#include <filesystem>
#include <optional>
#include <array>
#include <span>

#include <Windows.h>

#include <pfw.h>

#include "pipe.h"

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
        auto result = pfw::CreateProcess(executable, &security_attributes, startup_info);
        if (!result)
        {
            return nullptr;
        }

        CloseHandle(read_pipe_write_handle);
        CloseHandle(write_pipe_read_handle);

        auto read_handle = pfw::Handle::Create(read_pipe_read_handle);
        auto write_handle = pfw::Handle::Create(write_pipe_write_handle);

        std::unique_ptr<Pipe> pipe(new Pipe(std::move(*read_handle), std::move(*write_handle)));

        return std::unique_ptr<Loader>(new Loader(std::move(std::get<0>(*result)), std::move(std::get<1>(*result)), std::move(pipe)));
    }

    std::optional<HMODULE> Load(DWORD process_id, std::wstring dll)
    {
        ByteOutStream ostream;
        ostream << RemoteProcedure::Load;
        ostream << process_id;
        ostream << dll;

        if (!pipe_->Send(ostream.buffer_))
        {
            return std::nullopt;
        }

        auto ibuffer = pipe_->Receive();
        if (!ibuffer)
        {
            return std::nullopt;
        }

        ByteInStream istream(*ibuffer);

        std::optional<HMODULE> module;
        istream >> module;

        return module;
    }

    bool Unload(DWORD process_id, HANDLE module)
    {
        // return *CallRemoteProcedure<1, bool>(process_id, module);
        return false;
    }

    // template <uint8_t Id, typename ReturnType, typename... Args>
    // std::optional<ReturnType> CallRemoteProcedure(Args &&...args)
    // {
    //     std::vector<std::byte> buffer;
    //     std::basic_ospanstream<std::byte> stream(buffer);

    //     stream << Id;
    //     stream << uint32_t(wstring.size());

    //     for (auto wc : wstring)
    //     {
    //         stream << wc;
    //     }

    //     return std::nullopt;
    // }

    ~Loader()
    {
    }

private:
    pfw::Handle process_;
    pfw::Handle thread_;
    std::unique_ptr<Pipe> pipe_;

    Loader(pfw::Handle &&process, pfw::Handle &&thread, std::unique_ptr<Pipe> &&pipe) : process_(std::forward<pfw::Handle>(process)), thread_(std::forward<pfw::Handle>(thread)), pipe_(std::forward<std::unique_ptr<Pipe>>(pipe)) {}
};

class LoaderInterface
{
public:
    static std::unique_ptr<LoaderInterface> Create(std::wstring loader_directory)
    {
        auto executable = std::move(loader_directory) + L"/loader";

        auto loader32 = Loader::Create(executable + L"32.exe"); // TODO remove
        if (!loader32)
        {
            return nullptr;
        }

        auto loader64 = Loader::Create(executable + L"64.exe");
        if (!loader64)
        {
            return nullptr;
        }

        return std::unique_ptr<LoaderInterface>(new LoaderInterface(std::move(loader32), std::move(loader64)));
    }

    std::optional<HMODULE> Load(DWORD process_id, std::wstring dll)
    {
        auto loader = GetLoader(process_id);
        if (!loader)
        {
            return std::nullopt;
        }

        return (*loader)->Load(process_id, dll);
    }

    bool Unload(DWORD process_id, HMODULE module)
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