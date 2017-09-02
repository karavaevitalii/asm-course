#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <sys/mman.h>

#include <utility>
#include <cassert>

#include "arguments.h"

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
    func_ptr_t fptr;
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

    void clear() noexcept
    {
        func    = nullptr;
        fptr    = nullptr;
        code    = nullptr;
        caller  = nullptr;
        copier  = nullptr;
        deleter = nullptr;
    }

public:
    trampoline() noexcept
        : func{}, fptr{}, code{},
          caller{}, copier{}, deleter{}
    {}

    trampoline(trampoline const& that)
        : func{that.copier(that.func)},
          fptr{that.fptr},
          code{that.code},
          caller{that.caller},
          copier{that.copier},
          deleter{that.deleter}
    {}

    trampoline(trampoline&& that) noexcept
        : func{std::move(that.func)},
          fptr{std::move(that.fptr)},
          code{std::move(that.code)},
          caller{std::move(that.caller)},
          copier{std::move(that.copier)},
          deleter{std::move(that.deleter)}
    {
        that.clear();
    }

    trampoline(func_ptr_t fptr) noexcept
        : func{},
          fptr{std::move(fptr)},
          code{},
          caller{},
          copier{},
          deleter{}
    {}

    template <typename F>
    trampoline(F func)
        : func{new F(std::move(func))},
          fptr{},
          caller{do_call<F>},
          copier{do_copy<F>},
          deleter{do_delete<F>}
    {   
        code = ::mmap(nullptr, 4096, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (arguments<Args ...>::integral < 6)
        {
            auto* ncode = reinterpret_cast<unsigned char*>(code);

            *ncode++ = 0x48;
            *ncode++ = 0x89;
            *ncode++ = 0xfe;

            *ncode++ = 0x48;
            *ncode++ = 0xbf;
            *reinterpret_cast<void**>(ncode) = this->func;
            ncode += 8;

            *ncode++ = 0x48;
            *ncode++ = 0xb8;
            *reinterpret_cast<void**>(ncode) = reinterpret_cast<void*>(&do_call<F>);
            ncode += 8;

            *ncode++ = 0xFF;
            *ncode++ = 0xE0;
        }
    }

    trampoline(std::nullptr_t) noexcept
        : trampoline{}
    {}

    trampoline& operator=(trampoline that) noexcept
    {
        swap(that);
        return *this;
    }

    template <typename F>
    trampoline& operator=(F&& func)
    {
        trampoline tmp{std::forward<F>(func)};
        swap(tmp);
        return *this;
    }

    trampoline& operator=(std::nullptr_t)
    {
        if (func)
            deleter(func);

        clear();
        return *this;
    }

    void swap(trampoline& that) noexcept
    {
        swap_impl(*this, that);
    }

    explicit operator bool() const noexcept
    {
        return func != nullptr || fptr != nullptr;
    }

    R operator()(Args ... args) const
    {
        if (func)
            return caller(func, std::forward<Args>(args) ...);

        return fptr(std::forward<Args>(args) ...);
    }

    R (*get() const)(Args ... arg) noexcept
    {
        if (func)
            return reinterpret_cast<func_ptr_t const>(code);

        return fptr;
    }

    ~trampoline()
    {
        if (func)
        {
            deleter(func);

            int r = ::munmap(code, 4096);
            assert(r == 0);
        }

        clear();
    }

    template <typename R0, typename ... Args0>
    friend void swap_impl(trampoline<R0 (Args0 ...)>&, trampoline<R0 (Args0 ...)>&) noexcept;

    template <typename R0, typename ... Args0>
    friend bool operator==(trampoline<R0 (Args0 ...)> const&, std::nullptr_t) noexcept;

    template <typename R0, typename ... Args0>
    friend bool operator==(std::nullptr_t, trampoline<R0 (Args0 ...)> const&) noexcept;

    template <typename R0, typename ... Args0>
    friend bool operator!=(trampoline<R0 (Args0 ...)> const&, std::nullptr_t) noexcept;

    template <typename R0, typename ... Args0>
    friend bool operator!=(std::nullptr_t, trampoline<R0 (Args0 ...)> const&) noexcept;
};

template <typename R0, typename ... Arg0>
void swap(trampoline<R0 (Arg0 ...)>& a, trampoline<R0 (Arg0 ...)>& b) noexcept
{
    swap_impl(a, b);
}

template <typename R0, typename ... Args0>
void swap_impl(trampoline<R0 (Args0 ...)>& a, trampoline<R0 (Args0 ...)>& b) noexcept
{
    using std::swap;

    swap(a.func, b.func);
    swap(a.fptr, b.fptr);
    swap(a.code, b.code);
    swap(a.caller, b.caller);
    swap(a.copier, b.copier);
    swap(a.deleter, b.deleter);
}

template <typename R0, typename ... Args0>
bool operator==(trampoline<R0 (Args0 ...)> const& a, std::nullptr_t) noexcept
{
    return a.func == nullptr && a.fptr == nullptr;
}

template <typename R0, typename ... Args0>
bool operator==(std::nullptr_t, trampoline<R0 (Args0 ...)> const& a) noexcept
{
    return a == nullptr;
}

template <typename R0, typename ... Args0>
bool operator!=(trampoline<R0 (Args0 ...)> const& a, std::nullptr_t) noexcept
{
    return !(a == nullptr);
}

template <typename R0, typename ... Args0>
bool operator!=(std::nullptr_t, trampoline<R0 (Args0 ...)> const& a) noexcept
{
    return !(nullptr == a);
}

#endif //TRAMPOLINE_H
