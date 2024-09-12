#ifndef __PIPE_H__
#define __PIPE_H__

#include <optional>
#include <string>
#include <variant>

#include <pfw.h>

enum class RemoteProcedure : uint8_t
{
    Load,
    Unload,
    Close
};

class ReadPipe
{
public:
    static constexpr DWORD kBufferSize = 1024;

    ReadPipe(pfw::Handle &&handle) : handle_(std::forward<pfw::Handle>(handle)) {}

    std::optional<std::vector<std::byte>> Read()
    {
        std::vector<std::byte> buffer(kBufferSize);
        DWORD read;
        if (!ReadFile(handle_, buffer.data(), DWORD(buffer.size()), &read, nullptr) || read == 0)
        {
            return std::nullopt;
        }

        buffer.resize(read);

        return buffer;
    }

private:
    pfw::Handle handle_;
};

class WritePipe
{
public:
    static constexpr DWORD kBufferSize = 1024;

    WritePipe(pfw::Handle &&handle) : handle_(std::forward<pfw::Handle>(handle)) {}

    bool Write(std::span<std::byte> buffer)
    {
        DWORD written;
        if (!WriteFile(handle_, buffer.data(), DWORD(buffer.size()), &written, NULL) || written != DWORD(buffer.size()))
        {
            return false;
        }

        return true;
    }

private:
    pfw::Handle handle_;
};

#endif // __PIPE_H__