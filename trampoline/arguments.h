#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <cstdint>
#include <type_traits>

template <std::size_t N, bool B, typename ... Args>
struct integral_arguments_counter;

template <typename Arg, typename ... Args>
struct integral_arguments
{
    using args = integral_arguments_counter<sizeof... (Args), std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t value = args::value;
};

template <typename ... Args>
struct integral_arguments_counter<0, true, Args ...>
{
    constexpr static const std::size_t value = 1;
};

template <typename ... Args>
struct integral_arguments_counter<0, false, Args ...>
{
    constexpr static const std::size_t value = 0;
};

template <std::size_t N, typename Arg, typename ... Args>
struct integral_arguments_counter<N, true, Arg, Args ...>
{
    using args = integral_arguments_counter<N - 1, std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t value = args::value + 1;
};

template <std::size_t N, typename Arg, typename ... Args>
struct integral_arguments_counter<N, false, Arg, Args ...>
{
    using args = integral_arguments_counter<N - 1, std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t value = args::value;
};

template <std::size_t N, bool B, typename ... Args>
struct floating_arguments_counter;

template <typename Arg, typename ... Args>
struct floating_arguments
{
    using args = floating_arguments_counter<sizeof... (Args), !std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t value = args::value;
};

template <typename ... Args>
struct floating_arguments_counter<0, true, Args ...>
{
    constexpr static const std::size_t value = 1;
};

template <typename ... Args>
struct floating_arguments_counter<0, false, Args ...>
{
    constexpr static const std::size_t value = 0;
};

template <std::size_t N, typename Arg, typename ... Args>
struct floating_arguments_counter<N, true, Arg, Args ...>
{
    using args = floating_arguments_counter<N - 1, !std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t value = args::value + 1;
};

template <std::size_t N, typename Arg, typename ... Args>
struct floating_arguments_counter<N, false, Arg, Args ...>
{
    using args = floating_arguments_counter<N - 1, !std::is_integral<Arg>::value, Args ...>;

    constexpr static const std::size_t value = args::value;
};

#endif // ARGUMENTS_H