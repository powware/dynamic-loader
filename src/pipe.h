#include <optional>
#include <string>
#include <variant>

#include <pfw.h>

enum class RemoteProcedure : uint8_t
{
    Load,
    Unload
};

template <typename Type>
struct is_optional : std::false_type
{
};

template <typename Type>
struct is_optional<std::optional<Type>> : std::true_type
{
};

template <typename Type>
constexpr bool is_optional_v = is_optional<Type>::value;

struct ByteOutStream
{
    template <typename Type, typename = std::enable_if_t<!is_optional_v<Type>>>
    auto &operator<<(Type value)
    {
        auto end = buffer_.size();
        buffer_.resize(buffer_.size() + sizeof(value));
        std::memcpy(buffer_.data() + end, &value, sizeof(value));

        return *this;
    }

    template <>
    auto &operator<<(std::wstring wstring)
    {
        operator<<(uint32_t(wstring.size()));
        auto end = buffer_.size();
        buffer_.resize(buffer_.size() + wstring.size() * sizeof(wchar_t));
        std::memcpy(buffer_.data() + end, wstring.data(), wstring.size() * sizeof(wchar_t));

        return *this;
    }

    template <>
    auto &operator<<(HANDLE handle)
    {
        return operator<<(reinterpret_cast<std::uint64_t>(handle));
    }

    template <>
    auto &operator<<(HMODULE handle)
    {
        return operator<<(reinterpret_cast<std::uint64_t>(handle));
    }

    template <typename Type>
    auto &operator<<(std::optional<Type> value)
    {
        auto has_value = value.has_value();
        operator<<(has_value);
        if (has_value)
        {
            operator<<(*value);
        }

        return *this;
    }

    std::vector<std::byte> buffer_;
};

struct ByteInStream
{

    template <typename Type, typename = std::enable_if_t<!is_optional_v<Type>>>
    auto &operator>>(Type &value)
    {
        std::memcpy(&value, buffer_.data() + pos_, sizeof(value));
        pos_ += sizeof(value);

        return *this;
    }

    template <>
    auto &operator>>(std::wstring &wstring)
    {
        uint32_t wstring_size;
        operator>>(wstring_size);
        wstring.resize(wstring_size);
        std::memcpy(wstring.data(), buffer_.data() + pos_, wstring_size * sizeof(wchar_t));
        pos_ += wstring_size * sizeof(wchar_t);

        return *this;
    }

    template <>
    auto &operator>>(HANDLE &handle)
    {
        std::uint64_t handle_value;
        operator>>(handle_value);
        handle = reinterpret_cast<HANDLE>(handle_value);

        return *this;
    }

    template <>
    auto &operator>>(HMODULE &handle)
    {
        std::uint64_t handle_value;
        operator>>(handle_value);
        handle = reinterpret_cast<HMODULE>(handle_value);

        return *this;
    }

    template <typename Type>
    auto &operator>>(std::optional<Type> &value)
    {
        bool has_value;
        operator>>(has_value);
        if (has_value)
        {
            Type content;
            operator>>(content);
            value = content;
        }
        else
        {
            value = std::nullopt;
        }

        return *this;
    }

    ByteInStream(std::vector<std::byte> buffer) : buffer_(buffer) {}

    std::size_t pos_ = 0;
    std::vector<std::byte> buffer_;
};

// template <typename... Types>
// inline void Serialize(std::vector<std::byte> &buffer, std::variant<Types...> variant)
// {
//     auto size = buffer.size();
//     uint8_t index = uint8_t(variant.index());
//     buffer.resize(size + sizeof(index));
//     std::memcpy(buffer.data() + size, &index, sizeof(index));
//     std::visit([&](auto &&arg)
//                { Serialize(buffer, arg); }, variant);
// };

// template <typename T>
// struct Deserializer
// {
//     auto operator()(std::span<std::byte> buffer)
//     {
//         return T{};
//     }
// };

// template <typename... Ts>
// struct Deserializer<std::variant<Ts...>>
// {
//     auto operator()(std::span<std::byte> buffer)
//     {
//         uint8_t index;
//         std::memcpy(&index, buffer.data(), sizeof(index));

//         using Deserialize_t = std::variant<Ts...> (*)(std::span<std::byte> buffer);
//         Deserialize_t deserializers[] =
//             {
//                 [](std::span<std::byte> buffer) -> std::variant<Ts...>
//                 {
//                     return Deserializer<Ts>{}(buffer);
//                 }...,
//             };

//         return deserializers[index]({buffer.data() + sizeof(index), buffer.size() - sizeof(index)});
//     }
// };

class Pipe
{
public:
    static constexpr DWORD kBufferSize = 1024;

    Pipe(pfw::Handle &&read_handle, pfw::Handle &&write_handle) : read_handle_(std::forward<pfw::Handle>(read_handle)), write_handle_(std::forward<pfw::Handle>(write_handle)) {}

    static std::unique_ptr<Pipe> ConnectToParent()
    {
        auto read_handle = pfw::Handle::Create(GetStdHandle(STD_INPUT_HANDLE));
        if (!read_handle)
        {
            return nullptr;
        }

        auto write_handle = pfw::Handle::Create(GetStdHandle(STD_OUTPUT_HANDLE));
        if (!write_handle)
        {
            return nullptr;
        }

        std::unique_ptr<Pipe> pipe(new Pipe(std::move(*read_handle), std::move(*write_handle)));

        return pipe;
    }

    bool Send(std::span<std::byte> buffer)
    {
        DWORD written;
        if (!WriteFile(write_handle_, buffer.data(), DWORD(buffer.size()), &written, NULL) || written != DWORD(buffer.size()))
        {
            return false;
        }

        return true;
    }

    std::optional<std::vector<std::byte>> Receive()
    {
        std::vector<std::byte> buffer(kBufferSize);
        DWORD read;
        if (!ReadFile(read_handle_, buffer.data(), DWORD(buffer.size()), &read, nullptr) || read == 0)
        {
            return std::nullopt;
        }

        buffer.resize(read);

        return buffer;
    }

private:
    pfw::Handle read_handle_;
    pfw::Handle write_handle_;
};
