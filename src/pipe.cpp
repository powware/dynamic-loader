#include "pipe.h"

const DWORD Pipe::kMaxMessageSize = 512;

std::unique_ptr<Pipe::Server> Pipe::Create(std::wstring name)
{
    auto pipe_name = L"\\\\.\\pipe\\" + name;

    auto pipe = pfw::HandleGuard::Create(CreateNamedPipe(pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, kMaxMessageSize, kMaxMessageSize, 0, nullptr));
    if (!pipe)
    {
        return nullptr;
    }

    return std::make_unique<Server>(std::move(*pipe));
}

std::unique_ptr<Pipe::Client> Pipe::Connect()
{
    auto pipe = pfw::HandleGuard::Create(GetStdHandle(STD_INPUT_HANDLE));
    if (!pipe)
    {
        return nullptr;
    }

    return std::make_unique<Client>(std::move(*pipe));
}