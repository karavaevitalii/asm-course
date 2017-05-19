#include <utility>
#include <cassert>
#include <sys/mman.h>

template <typename T>
struct trampoline;

template <typename R, typename ... Args>
struct trampoline<R (Args ...)>
{
    using caller_t  = R     (*)(void*, Args ...);
    using copier_t  = void* (*)(void*);
    using deleter_t = void  (*)(void*);
    using getter_t  = R     (*)(Args ...);

    trampoline()
        : func(nullptr)
        , code(nullptr)
        , caller(nullptr)
        , copier(nullptr)
        , deleter(nullptr)
    {}

    trampoline(trampoline const& that)
        : func(that.copier(that.func))
        , code(that.code)
        , caller(that.caller)
        , copier(that.copier)
        , deleter(that.deleter)
    {}

    trampoline(trampoline&& that)
        : func(that.func)
        , code(that.code)
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
    {
        code = ::mmap(nullptr, 4096, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        char* ncode = reinterpret_cast<char*>(code);
        //4889FE        mov rsi, rdi
        *ncode++ = 0x48;
        *ncode++ = 0x89;
        *ncode++ = 0xfe;
        // 48BF         mov rdi, imm
        *ncode++ = 0x48;
        *ncode++ = 0xbf;
        *(void**)ncode = this->func;
        ncode += 8;
        // 48B8         mov rax, imm
        *ncode++ = 0x48;
        *ncode++ = 0xb8;
        *(void**)ncode = reinterpret_cast<void*>(&do_call<F>);
        ncode += 8;
        // FFE0         jmp rax
        *ncode++ = 0xFF;
        *ncode++ = 0xE0;
    }

    trampoline(std::nullptr_t)
        : trampoline()
    {}

    trampoline& operator=(trampoline const& that)
    {
        trampoline tmp{that};
        swap(tmp);
        return *this;
    }

    trampoline& operator=(trampoline&& that)
    {
        swap(that);
        return *this;
    }

    template <typename F>
    trampoline& operator=(F&& func)
    {
        trampoline tmp{func};
        swap(tmp);
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

    R operator()(Args ... args) const
    {
        return caller(func, std::forward<Args>(args) ...);
    }

    R (*get() const)(Args ... arg)
    {
        return reinterpret_cast<getter_t const>(code);
    }

    ~trampoline()
    {
        if (func)
        {
            deleter(func);

            int r = ::munmap(code, 4096);
            assert(r == 0);

            clear();
        }
    }

private:
    template <typename R0, typename ... Args0>
    friend void swap_impl(trampoline<R0 (Args0 ...)>&, trampoline<R0 (Args0 ...)>&);

    template <typename R0, typename ... Args0>
    friend bool operator==(trampoline<R0 (Args0 ...)> const&, std::nullptr_t);

    template <typename R0, typename ... Args0>
    friend bool operator==(std::nullptr_t, trampoline<R0 (Args0 ...)> const&);

    template <typename R0, typename ... Args0>
    friend bool operator!=(trampoline<R0 (Args0 ...)> const&, std::nullptr_t);

    template <typename R0, typename ... Args0>
    friend bool operator!=(std::nullptr_t, trampoline<R0 (Args0 ...)> const&);

private:
    template <typename F>
    static R do_call(void* func, Args ... args)
    {
        return (*static_cast<F*>(func))(std::forward<Args>(args) ...);
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
        code    = nullptr;
        caller  = nullptr;
        copier  = nullptr;
        deleter = nullptr;
    }

private:
    void* func;
    void* code;
    caller_t caller;
    copier_t copier;
    deleter_t deleter;
};

template <typename R0, typename Arg0>
void swap(trampoline<R0 (Arg0)>& a, trampoline<R0 (Arg0)>& b)
{
    swap_impl(a, b);
}

template <typename R0, typename ... Args0>
void swap_impl(trampoline<R0 (Args0 ...)>& a, trampoline<R0 (Args0 ...)>& b)
{
    std::swap(a.func, b.func);
    std::swap(a.code, b.code);
    std::swap(a.caller, b.caller);
    std::swap(a.copier, b.copier);
    std::swap(a.deleter, b.deleter);
}

template <typename R0, typename ... Args0>
bool operator==(trampoline<R0 (Args0 ...)> const& a, std::nullptr_t)
{
    return a.func == nullptr;
}

template <typename R0, typename ... Args0>
bool operator==(std::nullptr_t, trampoline<R0 (Args0 ...)> const& a)
{
    return a == nullptr;
}

template <typename R0, typename ... Args0>
bool operator!=(trampoline<R0 (Args0 ...)> const& a, std::nullptr_t)
{
    return !(a == nullptr);
}

template <typename R0, typename ... Args0>
bool operator!=(std::nullptr_t, trampoline<R0 (Args0 ...)> const& a)
{
    return !(nullptr == a);
}

int sub(int a)
{
    return a - 42;
}

int add(int a)
{
    return a + 42;
}

int main()
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
