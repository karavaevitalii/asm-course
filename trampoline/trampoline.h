#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <utility>
#include <sys/mman.h>

template <typename T>
class trampoline;

template <typename R, typename ... Args>
class trampoline<R (Args ...)>
{
    using caller_t      = R     (*)(void*, Args ...);
    using copier_t      = void* (*)(void*);
    using deleter_t     = void  (*)(void*);
    using func_ptr_t    = R     (*)(Args ...);

    void* func;
    void* code;
    caller_t caller;
    copier_t copier;
    deleter_t deleter;

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

public:
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
        : func(std::move(that.func))
        , code(std::move(that.code))
        , caller(std::move(that.caller))
        , copier(std::move(that.copier))
        , deleter(std::move(that.deleter))
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

        unsigned char* ncode = reinterpret_cast<unsigned char*>(code);

        *ncode++ = 0x48;
        *ncode++ = 0x89;
        *ncode++ = 0xfe;

        *ncode++ = 0x48;
        *ncode++ = 0xbf;
        *(void**)ncode = this->func;
        ncode += 8;

        *ncode++ = 0x48;
        *ncode++ = 0xb8;
        *(void**)ncode = reinterpret_cast<void*>(&do_call<F>);
        ncode += 8;

        *ncode++ = 0xFF;
        *ncode++ = 0xE0;
    }

    trampoline(std::nullptr_t)
        : trampoline()
    {}

    trampoline& operator=(trampoline that)
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
        return reinterpret_cast<func_ptr_t const>(code);
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

#endif //TRAMPOLINE_H
