#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <cstdint>
#include <type_traits>

template <std::size_t N, bool B, typename ... Args>
struct arguments_counter;

template <typename Arg, typename ... Args>
struct arguments
{
    using args = arguments_counter<sizeof... (Args), std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t integral = args::integral;
    constexpr static const std::size_t floating = args::floating;
};

template <typename ... Args>
struct arguments_counter<0, true, Args ...>
{
    constexpr static const std::size_t integral = 1;
    constexpr static const std::size_t floating = 0;
};

template <typename ... Args>
struct arguments_counter<0, false, Args ...>
{
    constexpr static const std::size_t integral = 0;
    constexpr static const std::size_t floating = 1;
};

template <std::size_t N, typename Arg, typename ... Args>
struct arguments_counter<N, true, Arg, Args ...>
{
    using args = arguments_counter<N - 1, std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t integral = args::integral + 1;
    constexpr static const std::size_t floating = args::floating;
};

template <std::size_t N, typename Arg, typename ... Args>
struct arguments_counter<N, false, Arg, Args ...>
{
    using args = arguments_counter<N - 1, std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t integral = args::integral;
    constexpr static const std::size_t floating = args::floating + 1;
};

#endif // ARGUMENTS_H