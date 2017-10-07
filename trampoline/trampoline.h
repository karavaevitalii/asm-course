#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <utility>
#include <array>

#include "arguments.h"
#include "allocator.h"

namespace
{
    struct handler_t
    {
        char* ptr;

        constexpr static std::array<char const*, 5> const shifts = {
            {"\x48\x89\xfe",
            "\x48\x89\xf2",
            "\x48\x89\xd1",
            "\x49\x89\xc8",
            "\x4d\x89\xc1"}
        };

        handler_t(void* ptr)
            : ptr(static_cast<char*>(ptr))
        {}

        void write(std::string_view cmd)
        {
            for (auto i = cmd.begin(); i != cmd.end(); ++i)
                *(ptr++) = *i;
        }

        void write(std::string_view cmd, void* obj)
        {
            write(cmd);
            *reinterpret_cast<void**>(ptr) = obj;
            ptr += sizeof(obj);
        }

        void write(std::string_view cmd, int32_t const imm)
        {
            write(cmd);
            *reinterpret_cast<int32_t*>(ptr) = imm;
            ptr += sizeof(imm);
        }
    };
} //namespace

template <typename T>
class trampoline;

template <typename R, typename ... Args>
class trampoline<R (Args ...)>
{
    using caller_t      = R     (*)(void*, Args ...);
    using deleter_t     = void  (*)(void*);
    using func_ptr_t    = R     (*)(Args ...);

    void* func;
    func_ptr_t fptr;
    void* code;
    caller_t caller;
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

    void clear() noexcept
    {
        func    = nullptr;
        fptr    = nullptr;
        code    = nullptr;
        caller  = nullptr;
        deleter = nullptr;
    }

public:
    trampoline() noexcept
        : func{}, fptr{}, code{},
          caller{}, deleter{}
    {}

    trampoline(trampoline&& that) noexcept
        : func{std::move(that.func)},
          fptr{std::move(that.fptr)},
          code{std::move(that.code)},
          caller{std::move(that.caller)},
          deleter{std::move(that.deleter)}
    {
        that.clear();
    }

    trampoline(func_ptr_t fptr) noexcept
        : func{},
          fptr{std::move(fptr)},
          code{},
          caller{},
          deleter{}
    {}

    template <typename F>
    trampoline(F func)
        : func{new F(std::move(func))},
          fptr{},
          caller{do_call<F>},
          deleter{do_delete<F>}
    {   
        code = utils::allocator::get_instance().allocate();
        handler_t handler{code};

        size_t const arguments_size = utils::integral_arguments<Args ...>::value;

        //shift all arguments in registers and put given functional object at rdi
        if (arguments_size < 6)
        {
            for (size_t i = arguments_size; i != 0; --i)
                handler.write(handler_t::shifts[i - 1]);

            handler.write("\x48\xbf", this->func);                              //mov rdi this->func
            handler.write("\x48\xb8", reinterpret_cast<void*>(&do_call<F>));    //mov rax do_call
            handler.write("\xff\xe0");                                          //jmp rax
        }
        else
        {
            //save return address
            handler.write("\x4c\x8b\x1c\x24");                              //mov   r11 [rsp]

            //shift arguments in registers
            for (size_t i = 5; i != 0; i--)
                handler.write(handler_t::shifts[i - 1]);

            int32_t stack_size = (arguments_size - 6 + utils::floating_arguments<Args ...>::value) * 8;

            //prepare stack for function call
            handler.write("\x48\x89\xe0");                                  //mov   rax rsp
            handler.write("\x48\x05", stack_size);                          //add   rax stack_size
            handler.write("\x41\x51");                                      //push  r9
            handler.write("\x48\x81\xc4", 8);                               //add   rsp 8

            //placing arguments in stack
            auto* label = handler.ptr;                                      //label for cycling

            handler.write("\x48\x39\xe0");                                  //cmp   rax rsp
            handler.write("\x74");                                          //je    break

            auto* break_label = handler.ptr;

            ++handler.ptr;
            handler.write("\x48\x81\xc4", 8);                               //add   rsp 8
            handler.write("\x4c\x8c\x0c\x24");                              //mov   r9  [rsp]
            handler.write("\x4c\x89\x4c\x24\xf8");                          //mov   [rsp - 8] r9

            handler.write("\xeb");                                          //jmp   label

            *handler.ptr = static_cast<char>(label - handler.ptr);
            ++handler.ptr;
            *break_label = static_cast<char>(handler.ptr - break_label - 1);//break:

            //arguments are shifted, call the function
            handler.write("\x4c\x89\x1c\x24");                              //mov   [rsp] r11
            handler.write("\x48\x81\xec", stack_size + 8);                  //sub   rsp on_stack + 8
            handler.write("\x48\xbf", this->func);                          //mov   rdi obj
            handler.write("\x48\xb8", reinterpret_cast<void*>(&do_call<F>));//mov   rax caller
            handler.write("\xff\xd0");                                      //call  rax

            //restore stack
            handler.write("\x41\x59");                                      //pop   r9
            handler.write("\x4c\x8b\x9c\x24", stack_size);                  //mov   r11 [rsp + on_stack]
            handler.write("\x4c\x89\x1c\x24");                              //mov   [rsp] r11
            handler.write("\xc3");                                          //ret
        }
    }

    trampoline(std::nullptr_t) noexcept
        : trampoline{}
    {}

    trampoline& operator=(trampoline&& that)
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

    trampoline(trampoline const&)               = delete;
    trampoline& operator=(trampoline const&)    = delete;

    void swap(trampoline& that)
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

    R (*get() const)(Args ... arg)
    {
        if (func)
            return reinterpret_cast<func_ptr_t const>(code);

        return fptr;
    }

    ~trampoline()
    {
        if (func)
            deleter(func);

        utils::allocator::get_instance().deallocate(code);
        clear();
    }

    template <typename R0, typename ... Args0>
    friend void swap_impl(trampoline<R0 (Args0 ...)>&, trampoline<R0 (Args0 ...)>&);

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
void swap_impl(trampoline<R0 (Args0 ...)>& a, trampoline<R0 (Args0 ...)>& b)
{
    using std::swap;

    swap(a.func, b.func);
    swap(a.fptr, b.fptr);
    swap(a.code, b.code);
    swap(a.caller, b.caller);
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
