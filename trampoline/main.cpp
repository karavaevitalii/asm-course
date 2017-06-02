#include <assert.h>

#include "trampoline.h"

int sub(int a)
{
    return a - 42;
}

int add(int a)
{
    return a + 42;
}

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
        trampoline<int (int)> tr1 = [](int a) { return a + 42;};
        trampoline<int (int)> tr2 = tr1;
        assert(tr1);
        assert(tr2);
        assert(tr1(2) == tr2(2));

        auto p1 = tr1.get();
        auto p2 = tr2.get();
        assert(p1(5) == p2(5));
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

        assert(tr2(5) == 5  + 42);
        assert(tr1(5) == 5  - 42);
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        tr2 = tr1;

        assert(tr1);
        assert(tr2);

        assert(tr1(5) == 5 + 42);
        assert(tr2(5) == 5 + 42);

        auto p1 = tr1.get();
        auto p2 = tr2.get();
        assert(p1(5) == p2(5));
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        tr2 = std::move(tr1);

        assert(tr2);
        assert(tr2(5) == 5  + 42);
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

}

int f(int a, int b, int c, double d, int e, int g, int h, int i)
{
    return a + b + c + static_cast<int>(d) + e + g + h + i;
}

int main()
{
    simple_test();

    trampoline<int (int, int, int, double, int, int, int, int)> tr(f);
    assert(f);

    assert(tr(1, 2, 3, 4., 5, 6, 7, 8) == f(1, 2, 3, 4., 5, 6, 7, 8));

    return 0;
}
