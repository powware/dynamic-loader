#include <optional>
#include <string>

#include <pfw.h>

enum class Operation
{
    Load,
    Unload
};

class Pipe
{
public:
    enum Type
    {
        Server,
        Client
    };

    static const DWORD kMaxMessageSize;

    static std::optional<Pipe> Create(Type type, std::wstring name)
    {
        auto pipe_name = L"\\\\.\\pipe\\" + name;

        auto pipe = pfw::HandleGuard::Create([=]()
                                             {
            if (type == Type::Server)
            {
                return CreateNamedPipe(pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, kMaxMessageSize, kMaxMessageSize, 0, nullptr);
            }
            else
            {
                return CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            } }());
        if (!pipe)
        {
            return std::nullopt;
        }

        return std::make_optional<Pipe>(Pipe(type, std::move(*pipe)));
    }

    bool WaitForClient()
    {
        if (type_ != Server)
        {
            return false;
        }

        return ConnectNamedPipe(pipe_, nullptr);
    }

    bool Send(std::wstring message)
    {
        const auto message_size = DWORD((message.size()) * sizeof(wchar_t));
        DWORD written;
        if (!WriteFile(pipe_, message.c_str(), message_size, &written, NULL) || written != message_size)
        {
            return false;
        }

        return true;
    }

    std::optional<std::wstring> Receive()
    {
        std::wstring message(kMaxMessageSize / sizeof(wchar_t), L'\0');
        DWORD read;
        if (!ReadFile(pipe_, message.data(), DWORD(message.size()), &read, nullptr) || read == 0)
        {
            return std::nullopt;
        }

        message.resize(read / sizeof(wchar_t), L'\0');

        return message;
    }

private:
    pfw::HandleGuard pipe_;
    Type type_;

    Pipe(Type type, pfw::HandleGuard &&pipe) : type_(type), pipe_(std::forward<pfw::HandleGuard>(pipe)) {}
};