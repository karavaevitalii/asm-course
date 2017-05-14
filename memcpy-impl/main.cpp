#include <cstdint>
#include <vector>
#include <algorithm>
#include <cassert>
#include <emmintrin.h>
#include <chrono>
#include <iostream>

void* memcpy(void* dest, void const* src, size_t count)
{
    char* ndest = static_cast<char*>(dest);
    char const* nsrc = static_cast<char const*>(src);

    size_t const step = sizeof(__m128i);
    size_t position = 0;
    for (;;)
    {
        if ((reinterpret_cast<size_t>(dest) + position) % step == 0
                || position == count)
            break;

        ndest[position] = nsrc[position];
        ++position;
    }

    size_t const aligned_size = (count - position) % step;
    size_t times = count - aligned_size;

    __m128i tmp;
    __asm__ volatile(
    "1:"
         "movdqu    (%[dest]),   %[tmp]  \n"
         "movntdq   %[tmp],     (%[src])\n"

         "add       %[step],    %[dest]  \n"
         "add       %[step],    %[src]  \n"
         "sub       %[step],    %[times]\n"

         "jnz       1b                  \n"

         : [dest] "=r" (dest)
                , [tmp] "=x" (tmp)
         : "0" (dest)
                , [src] "r" (src)
                , [times] "r" (times)
                , [step] "r" (step)
         : "memory", "cc"
    );

    size_t const from_aligned = count - aligned_size;
    for (size_t i = from_aligned; i != count; ++i)
        ndest[i] = nsrc[i];

    return dest;
}

int main()
{    
    using type = char;
    using clock_t = std::chrono::high_resolution_clock;

    size_t const N = 100000000;
    size_t const M = 100;

    std::vector<type> a(N), b(N);
    std::fill(a.begin(), a.end(), 'a');
    std::fill(b.begin(), b.end(), 'b');

    size_t bytes = N * sizeof(type);

    auto start = clock_t::now();
    for (size_t i = 0; i != M; ++i)
        memcpy(b.data(), a.data(), bytes);
    auto end = clock_t::now();

    for (size_t i = 0; i != N; ++i)
        assert(a[i] == b[i]);

    std::cout << static_cast<double>(bytes * M) / 1000000000. << "Gb: "
              << static_cast<double>((end - start).count()) / 1000000000. << " second(s)" << std::endl;

    return EXIT_SUCCESS;
}
