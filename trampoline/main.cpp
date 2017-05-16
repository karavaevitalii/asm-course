#include <utility>
#include <cassert>
#include <iostream>

template <typename T>
struct trampoline;

template <typename R, typename Arg>
struct trampoline<R (Arg)>
{
    using caller_t  = R     (*)(void*, Arg);
    using copier_t  = void* (*)(void*);
    using deleter_t = void  (*)(void*);

    trampoline()
        : func(nullptr)
        , caller(nullptr)
        , copier(nullptr)
        , deleter(nullptr)
    {}

    trampoline(trampoline const& that)
        : func(that.copier(that.func))
        , caller(that.caller)
        , copier(that.copier)
        , deleter(that.deleter)
    {}

    trampoline(trampoline&& that)
        : func(that.func)
        , caller(that.caller)
        , copier(that.copier)
        , deleter(that.deleter)
    {
        that.clear();
    }

    template <typename F>
    trampoline(F func)
        : func(new F(std::move(func)))
        , caller(do_call<F>)
        , copier(do_copy<F>)
        , deleter(do_delete<F>)
    {}

    trampoline(std::nullptr_t)
        : trampoline()
    {}

    trampoline& operator=(trampoline const& that)
    {
        trampoline tmp{that};
        swap_impl(*this, tmp);
        return *this;
    }

    trampoline& operator=(trampoline&& that)
    {
        swap_impl(*this, that);
        return *this;
    }

    template <typename F>
    trampoline& operator=(F&& func)
    {
        trampoline tmp{func};
        swap_impl(*this, tmp);
        return *this;
    }

    trampoline& operator=(std::nullptr_t)
    {
        deleter(func);
        clear();
        return *this;
    }

    void swap(trampoline& that)
    {
        swap_impl(*this, that);
    }

    explicit operator bool() const
    {
        return func != nullptr;
    }

    R operator()(Arg arg) const
    {
        return caller(func, arg);
    }

    ~trampoline()
    {
        if (func)
        {
            deleter(func);
            clear();
        }
    }

private:
    template <typename R0, typename Arg0>
    friend void swap_impl(trampoline<R0 (Arg0)>&, trampoline<R0 (Arg0)>&);

    template <typename R0, typename Arg0>
    friend bool operator==(trampoline<R0 (Arg0)> const&, std::nullptr_t);

    template <typename R0, typename Arg0>
    friend bool operator==(std::nullptr_t, trampoline<R0 (Arg0)> const&);

    template <typename R0, typename Arg0>
    friend bool operator!=(trampoline<R0 (Arg0)> const&, std::nullptr_t);

    template <typename R0, typename Arg0>
    friend bool operator!=(std::nullptr_t, trampoline<R0 (Arg0)> const&);

private:
    template <typename F>
    static R do_call(void* func, Arg arg)
    {
        return (*static_cast<F*>(func))(arg);
    }

    template <typename F>
    static void do_delete(void* func)
    {
        delete static_cast<F*>(func);
    }

    template <typename F>
    static void* do_copy(void* func)
    {
        return new F(*static_cast<F*>(func));
    }

    void clear()
    {
        func    = nullptr;
        caller  = nullptr;
        copier  = nullptr;
        deleter = nullptr;
    }

private:
    void* func;
    caller_t caller;
    copier_t copier;
    deleter_t deleter;
};

template <typename R0, typename Arg0>
void swap(trampoline<R0 (Arg0)>& a, trampoline<R0 (Arg0)>& b)
{
    swap_impl(a, b);
}

template <typename R0, typename Arg0>
void swap_impl(trampoline<R0 (Arg0)>& a, trampoline<R0 (Arg0)>& b)
{
    std::swap(a.func, b.func);
    std::swap(a.caller, b.caller);
    std::swap(a.copier, b.copier);
    std::swap(a.deleter, b.deleter);
}

template <typename R0, typename Arg0>
bool operator==(trampoline<R0 (Arg0)> const& a, std::nullptr_t)
{
    return a.func == nullptr;
}

template <typename R0, typename Arg0>
bool operator==(std::nullptr_t, trampoline<R0 (Arg0)> const& a)
{
    return a.func == nullptr;
}

template <typename R0, typename Arg0>
bool operator!=(trampoline<R0 (Arg0)> const& a, std::nullptr_t)
{
    return !(a == nullptr);
}

template <typename R0, typename Arg0>
bool operator!=(std::nullptr_t, trampoline<R0 (Arg0)> const& a)
{
    return !(nullptr == a);
}

int sub(int a)
{
    return a - 42;
}

int sum(int a)
{
    return a + 42;
}

int main()
{
    {
        trampoline<int (int)> tr([](int a) { return a + 42; });
        assert(tr);
        assert(tr(5) == 5 + 42);
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42;};
        trampoline<int (int)> tr2 = tr1;

        assert(tr1(2) == tr2(2));
        assert(tr1);
        assert(tr2);
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        assert(tr1);

        trampoline<int (int)> tr2 = std::move(tr1);
        assert(tr2);
        assert(!tr1);
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

        assert(tr1(5) == 5  + 42);
        assert(tr2(5) == 5  + 42);
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        tr2 = std::move(tr1);

        assert(tr2);
        assert(tr2(5) == 5  + 42);
    }

    {
        trampoline<int (int)> tr1 = [](int a) { return a + 42; };
        trampoline<int (int)> tr2 = [](int a) { return a - 42; };

        tr1.swap(tr2);

        assert(tr1);
        assert(tr2);

        assert(tr2(5) == 5  + 42);
        assert(tr1(5) == 5  - 42);
    }

    {
        trampoline<int (int)> tr = [](int a) { return a + 42; };
        assert(tr);
        tr = nullptr;
        assert(!tr);
    }

    {
        trampoline<int (int)> tr(sum);
        assert(tr);
        assert(tr(5) == 5 + 42);

        tr = sub;
        assert(tr);
        assert(tr(5) == 5 - 42);
    }

    {
        trampoline<int (int)> tr;
        assert(tr == nullptr);
        assert(!(tr != nullptr));

        tr = [](int a) { return a + 42; };
        assert(tr != nullptr);
        assert(!(tr == nullptr));
    }
}
