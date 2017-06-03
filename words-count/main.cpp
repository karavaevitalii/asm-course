#include <string>
#include <assert.h>
#include <random>
#include <emmintrin.h>
#include <chrono>
#include <iostream>

size_t naive_count(std::string const& str)
{
    size_t count = 0;
    bool is_previous_space = true;

    for (auto i = str.begin(); i != str.end(); ++i)
    {
        bool is_current_space = (*i == ' ');
        count += is_current_space & !is_previous_space;

        is_previous_space = is_current_space;
    }

    return count + !is_previous_space;
}

size_t calc_and_flush(__m128i& src)
{
    __m128i value = _mm_setzero_si128();
    value = _mm_sad_epu8(value, src);

    uint64_t low  = reinterpret_cast<uint64_t*>(&value)[0];
    uint64_t high = reinterpret_cast<uint64_t*>(&value)[1];

    src = _mm_setzero_si128();
    return low + high;
}

size_t count(std::string const& str)
{
    static size_t  const step       = sizeof(__m128i);
    static __m128i const space_mask = _mm_set1_epi8(' ');

    size_t position = 0;
    while (str[position] == ' ')
        ++position;

    size_t count = 0;
    bool is_previous_space = true;
    char const* data = str.data();
    for (; position < str.size() && (reinterpret_cast<size_t>(data) + position) % step != 0; ++position)
    {
        bool is_current_space = (str[position] == ' ');
        count += is_current_space & !is_previous_space;

        is_previous_space = is_current_space;
    }

    __m128i result = _mm_setzero_si128();
    for (; position <= str.size() - step; position += step)
    {
        __m128i spaces          = _mm_cmpeq_epi8(_mm_load_si128(reinterpret_cast<__m128i const*>(data + position)), space_mask);
        __m128i shifted_spaces  = _mm_cmpeq_epi8(_mm_loadu_si128(reinterpret_cast<__m128i const*>(data + position - 1)), space_mask);
        __m128i words           = _mm_andnot_si128(shifted_spaces, spaces);

        result = _mm_sub_epi8(result, words);
        if (_mm_movemask_epi8(result))
            count += calc_and_flush(result);
    }

    count += calc_and_flush(result);

    is_previous_space = position == 0 ? true : str[position - 1] == ' ';
    for (; position < str.size(); ++position)
    {
        bool is_current_space = (str[position] == ' ');
        count += is_current_space & !is_previous_space;

        is_previous_space = is_current_space;
    }

    return count + !is_previous_space;
}

std::string get_random_string(size_t limit)
{
    static char const alphabet[] = "ab cd ef gh ij kl mn op qr st uv wx yz";
    static size_t const max_index = sizeof alphabet;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<size_t> dist(0, max_index - 1);

    std::string ret;

    for (size_t i = 0; i != limit; ++i)
        ret.push_back(alphabet[dist(gen)]);

    return ret;
}

int main()
{
    {
        for (size_t i = 0; i != 100; ++i)
        {
            std::string s = get_random_string(100000);
            assert(naive_count(s) == count(s));
        }
    }

    {
        std::string s = get_random_string(100000000);

        std::cout << "counting " << static_cast<double>(s.size()) / 1000000000. << " Gb:" << std::endl;

        using clock_t = std::chrono::high_resolution_clock;
        auto start = clock_t::now();
        size_t naive = naive_count(s);
        auto end = clock_t::now();

        std::cout << "naive: " << static_cast<double>((end - start).count()) / 1000000000. << " second(s)" << std::endl;

        start = clock_t::now();
        size_t fast = count(s);
        end = clock_t::now();

        std::cout << "fast: " << static_cast<double>((end - start).count()) / 1000000000. << " second(s)" << std::endl;

        assert(naive == fast);
    }

    return 0;
}
