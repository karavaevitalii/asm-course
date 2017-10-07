#include "trampoline.h"

#include <cassert>
#include <numeric>

int sub(int a)
{
    return a - 42;
}

int add(int a)
{
    return a + 42;
}

struct func_obj
{
    int operator()(int, int, int, int, int) const
    {
        return 42;
    }
};

void simple_test()
{
    {
        trampoline<int (int)> tr([](int a) { return a + 42; });
        assert(tr);
        assert(tr(5) == 5 + 42);
        auto p = tr.get();
        assert(p(5) == tr(5));
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        assert(tr1);

        trampoline<int (int)> tr2 = std::move(tr1);
        assert(tr2);
        assert(!tr1);

        assert(tr2(5) == 5 + 42);
        auto p2 = tr2.get();
        assert(tr2(5) == p2(5));
    }

    {
        trampoline<int (int)> tr(nullptr);
        assert(!tr);
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        swap(tr1, tr2);

        assert(tr1);
        assert(tr2);

        assert(tr2(5) == 5 + 42);
        assert(tr1(5) == 5 - 42);
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        tr2 = std::move(tr1);

        assert(tr2);
        assert(tr2(5) == 5 + 42);
        auto p2 = tr2.get();
        assert(tr2(5) == p2(5));
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        tr1.swap(tr2);

        assert(tr1);
        assert(tr2);

        assert(tr2(5) == 5 + 42);
        assert(tr1(5) == 5 - 42);

        auto p1 = tr1.get();
        assert(p1(5) == tr1(5));
        auto p2 = tr2.get();
        assert(p2(5) == tr2(5));
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        swap(tr1, tr2);

        assert(tr1);
        assert(tr2);

        assert(tr2(5) == 5 + 42);
        assert(tr1(5) == 5 - 42);

        auto p1 = tr1.get();
        assert(p1(5) == tr1(5));
        auto p2 = tr2.get();
        assert(p2(5) == tr2(5));
    }

    {
        trampoline<int (int)> tr = [](int a) { return a + 42; };
        assert(tr);
        tr = nullptr;
        assert(!tr);
    }

    {
        trampoline<int (int)> tr(add);
        assert(tr);
        assert(tr(5) == 5 + 42);

        tr = sub;
        assert(tr);
        assert(tr(5) == 5 - 42);

        auto p = tr.get();
        assert(p(5) == tr(5));
    }

    {
        trampoline<int (int)> tr;

        assert(tr == nullptr);
        assert(nullptr == tr);
        assert(!(tr != nullptr));
        assert(!(nullptr != tr));

        tr = [](int a) { return a + 42; };

        assert(tr != nullptr);
        assert(nullptr != tr);
        assert(!(tr == nullptr));
        assert(!(nullptr == tr));
    }

    {
        int b = 42;
        trampoline<int (int)> tr([&](int a) { return a + b; });

        assert(tr(5) == 5 + 42);
        auto p = tr.get();
        assert(p(5) == tr(5));
        assert(p(5) == b + 5);

        b = 124;
        assert(tr(6) == 124 + 6);
        assert(p(6) == tr(6));
        assert(p(6) == b + 6);
    }

    {
        func_obj tmp;
        trampoline<int (int, int, int, int, int)> tr(tmp);
        int (*p)(int, int, int, int, int) = tr.get();
        assert(p(1, 1, 1, 1, 1) == 42);
    }

    {
        trampoline<int (int, int, int, int, int)> tr = [](int a, int b, int c, int d, int e)
        {
            return a + b + c + d + e;
        };

        auto p = tr.get();
        assert(p(1, 1, 1, 1, 1) == 5);
    }

    {
        trampoline<double (double, double, double, double, double)> tr = [](double a, double b, double c, double d, double e)
        {
            return a + b + c + d + e;
        };

        auto p = tr.get();
        assert(std::abs(5.2 - p(1.0, 1.2, 1, 1, 1)) < std::numeric_limits<double>::epsilon());
    }

    {
        trampoline<float (float, float, float, float, float)> tr = [](float a, float b, float c, float d, float e)
        {
            return a + b + c + d + e;
        };

        auto p = tr.get();
        assert(std::abs(5.2f - p(1.f, 1.2f, 1, 1, 1)) < std::numeric_limits<float>::epsilon());
    }

    {
        trampoline<float (int, double, int, float, float)> tr = [](int a, double b, int c, float d, float e)
        {
            return a + b + c + static_cast<double>(d) + static_cast<double>(e);
        };

        auto p = tr.get();
        assert(std::abs(5.2f - p(1, 1.2, 1, 1.f, 1.f)) < std::numeric_limits<float>::epsilon());
    }

    {
        trampoline<int (int&)> tr = [](int const& a) { return a; };

        auto p = tr.get();
        int a = 1;
        assert(p(a) == 1 );
    }

    {
        trampoline<double (double, double, double, double, double, double, double, double)> tr =
                [](double a, double b, double c, double d, double e, double f, double g, double h)
        {
            return a + b + c + d + e + f + g + h;
        };

        auto p = tr.get();
        assert(8 - p(1, 1, 1, 1, 1, 1, 1, 1) < std::numeric_limits<double>::epsilon());
    }

    {
        trampoline<float (float, float, float, float, float, float, float, float)> tr =
                [](float a, float b, float c, float d, float e, float f, float g, float h)
        {
            return a + b + c + d + e + f + g + h;
        };

        auto p = tr.get();
        assert(std::abs(8 - p(1, 1, 1, 1, 1, 1, 1, 1)) < std::numeric_limits<float>::epsilon());
    }

    {
        trampoline<double (double&, double&, double&, double, double, double, double, double)> tr =
                [](double a, double b, double c, double d, double e, double f, double g, double h)
        {
            return a + b + c + d + e + f + g + h;
        };

        auto p = tr.get();

        double a = 1, b = 1, c = 1;

        assert(8 - p(a, b, c, 1, 1, 1, 1, 1) < std::numeric_limits<double>::epsilon());
    }
}

void hard_test()
{
    {
        trampoline<long long (int, int, int, int, int, int, int, int)> tr =
                [](int a, int b, int c, int d, int e, int f, int g, int h)
        {
            return a + b + c + d + e + f + g + h;
        };

        auto p = tr.get();
        assert(p(1, 1, 1, 1, 1, 1, 1, 1) == 8);
    }

    {
        trampoline<long long (int&, int&, int&, int&, int, int, int, int)> tr =
                [](int a, int b, int c, int d, int e, int f, int g, int h)
        {
            return a + b + c + d + e + f + g + h;
        };

        auto p = tr.get();

        int a = 1, b = 1, c = 1, d = 1;
        assert(p(a, b, c, d, 1, 1, 1, 1) == 8);
    }

    {
        trampoline<float (double, int, float, int, int, double, double, float)> t1 =
                [](double a, int b, float c, int d, int e, double f, double g, float h)
        {
            return a + b + static_cast<double>(c) + d + e + f + g + static_cast<double>(h);
        };
        auto p1 = t1.get();

        trampoline<float (double, int, float, int, const int&, double, double, float)> t2 =
                [](double a, int b, float c, int d, int e, double f, double g, float h)
        {
            return a + b + static_cast<double>(c) + d + e + f + g + static_cast<double>(h);
        };
        auto p2 = t2.get();

        const int a = 1;
        double b = 3.7;
        float c = 4.1f;

        assert(static_cast<float>(p1(1, 1, 1.f, 1, 1, 1, 1, 1.f) + 103.8f) - p2(1, 2, 100, -1, a, b, 1, c)
               < std::numeric_limits<float>::epsilon());
    }

    {
        trampoline<int (double, int, float, int, const int&, double, double, float)> tr =
                [](double, int a, float, int, int, double, double, float)
        {
            return a;
        };

        auto p = tr.get();
        assert(p(1, 2, 3, 4, 5, 6, 7, 8) == 2);
    }

    {
        trampoline<int (double, int, float, int, const int&, double, double, float)> tr =
                [](double, int, float, int, int, double, double, float a)
        {
            return a;
        };

        auto p = tr.get();

        assert(p(1, 2, 3, 4, 5, 6, 7, 8.8f) == 8);
    }  
}

int main()
{
    simple_test();
    hard_test();

    return EXIT_SUCCESS;
}
