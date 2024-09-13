#ifndef __SERIALIZATION_H__
#define __SERIALIZATION_H__

#include <optional>
#include <vector>
#include <string>
#include <utility>

#include <pfw.h>

#include "pipe.h"

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

class Serializer
{
public:
    template <typename Type, typename = std::enable_if_t<!is_optional_v<Type>>>
    auto &&serialize(Type value)
    {
        const auto end = buffer_.size();
        buffer_.resize(buffer_.size() + sizeof(value));
        std::memcpy(buffer_.data() + end, &value, sizeof(value));

        return *this;
    }

    template <>
    auto &&serialize(std::wstring wstring)
    {
        serialize(std::uint32_t(wstring.size()));
        auto end = buffer_.size();
        buffer_.resize(buffer_.size() + wstring.size() * sizeof(wchar_t));
        std::memcpy(buffer_.data() + end, wstring.data(), wstring.size() * sizeof(wchar_t));

        return *this;
    }

    template <>
    auto &&serialize(HANDLE handle)
    {
        return serialize(reinterpret_cast<std::uint64_t>(handle));
    }

    template <>
    auto &&serialize(HMODULE handle)
    {
        return serialize(reinterpret_cast<std::uint64_t>(handle));
    }

    template <typename Type>
    auto &&serialize(std::optional<Type> value)
    {
        const auto has_value = value.has_value();
        serialize(has_value);
        if (has_value)
        {
            serialize(*value);
        }

        return *this;
    }

    [[nodiscard]] auto buffer() &
    {
        return buffer_;
    }

    [[nodiscard]] auto &&buffer() &&
    {
        return std::move(buffer_);
    }

private:
    std::vector<std::byte> buffer_;
};

std::vector<std::byte> Serialize(auto &&...args)
{
    Serializer serializer;
    ((serializer = std::move(serializer).serialize(std::forward<decltype(args)>(args))), ...);
    return std::move(serializer).buffer();
}

class Deserializer
{
public:
    Deserializer(std::span<std::byte> buffer) : buffer_(buffer) {}

    template <typename Type, typename = std::enable_if_t<!is_optional_v<Type>>>
    auto &&deserialize(Type &value)
    {
        std::memcpy(&value, buffer_.data(), sizeof(value));
        buffer_ = buffer_.subspan(sizeof(value));

        return *this;
    }

    template <>
    auto &&deserialize(std::wstring &wstring)
    {
        uint32_t size;
        deserialize(size);
        wstring.resize(size);
        std::memcpy(wstring.data(), buffer_.data(), size * sizeof(wchar_t));
        buffer_ = buffer_.subspan(size * sizeof(wchar_t));

        return *this;
    }

    template <>
    auto &&deserialize(HANDLE &handle)
    {
        std::uint64_t handle_value;
        deserialize(handle_value);
        handle = reinterpret_cast<HANDLE>(handle_value);

        return *this;
    }

    template <>
    auto &&deserialize(HMODULE &handle)
    {
        std::uint64_t handle_value;
        deserialize(handle_value);
        handle = reinterpret_cast<HMODULE>(handle_value);

        return *this;
    }

    template <typename Type>
    auto &&deserialize(std::optional<Type> &value)
    {
        bool has_value;
        deserialize(has_value);
        if (has_value)
        {
            Type content;
            deserialize(content);
            value = content;
        }
        else
        {
            value = std::nullopt;
        }

        return *this;
    }

private:
    std::span<std::byte> buffer_;
};

template <typename... Types, std::size_t... Is>
std::pair<Types...> DeserializeImpl(std::span<std::byte> buffer, std::index_sequence<Is...>)
{
    Deserializer deserializer(buffer);
    std::pair<Types...> result;
    ((deserializer = std::move(deserializer).deserialize(std::get<Is>(result))), ...);
    return result;
}

template <typename... Types>
auto Deserialize(std::span<std::byte> buffer)
{
    return DeserializeImpl<Types...>(buffer, std::index_sequence_for<Types...>{});
}

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

#endif // __SERIALIZATION_H__