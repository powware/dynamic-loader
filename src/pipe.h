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
    class Server;
    class Client;

    static const DWORD kMaxMessageSize;

    static std::unique_ptr<Server> Create(std::wstring name);

    static std::unique_ptr<Client> Connect();

    bool Send(std::wstring message)
    {
        const auto message_size = DWORD((message.size()) * sizeof(wchar_t));
        DWORD written;
        if (!WriteFile(handle_, message.c_str(), message_size, &written, NULL) || written != message_size)
        {
            return false;
        }

        return true;
    }

    std::optional<std::wstring> Receive()
    {
        std::wstring message(kMaxMessageSize / sizeof(wchar_t), L'\0');
        DWORD read;
        if (!ReadFile(handle_, message.data(), DWORD(message.size()), &read, nullptr) || read == 0)
        {
            return std::nullopt;
        }

        message.resize(read / sizeof(wchar_t), L'\0');

        return message;
    }

private:
    pfw::HandleGuard handle_;

    template <typename Type>
    Pipe(Type handle) : handle_(std::forward<Type>(handle)) {}
};

class Pipe::Server
{
public:
    bool Send(std::wstring message)
    {
        return pipe_.Send(message);
    }

    std::optional<std::wstring> Receive()
    {
        return pipe_.Receive();
    }

    bool WaitForClient()
    {
        return ConnectNamedPipe(pipe_.handle_, nullptr);
    }

    template <typename Type>
    Server(Type &&pipe) : pipe_(std::forward<Type>(pipe)) {}

private:
    Pipe pipe_;
};

class Pipe::Client
{
public:
    bool Send(std::wstring message)
    {
        return Send(message);
    }

    std::optional<std::wstring> Receive()
    {
        return pipe_.Receive();
    }

    template <typename Type>
    Client(Type &&pipe) : pipe_(std::forward<Type>(pipe)) {}

private:
    Pipe pipe_;
};
