#include "allocator.h"

#include <sys/mman.h>

#include <cstdint>
#include <cassert>

namespace
{
    constexpr static std::size_t const NODE_SIZE = 256;
    constexpr static std::size_t const PAGE_SIZE = 4096;
} //namespace

using namespace utils;

allocator::allocator()
{
    head = ::mmap(nullptr, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    tail = head;

    auto* start = static_cast<char*>(head);
    for (auto* i = start + NODE_SIZE; i < start + PAGE_SIZE; i += NODE_SIZE)
        *reinterpret_cast<void**>(i - NODE_SIZE) = i;
}

allocator::~allocator()
{
    int r = ::munmap(head, PAGE_SIZE);
    assert(r == 0);
}

allocator& allocator::get_instance()
{
    static allocator instance = allocator{};
    return instance;
}

void* allocator::allocate()
{
    void* ret = tail;
    tail = *static_cast<void**>(tail);
    return ret;
}

void allocator::deallocate(void* ptr)
{
    if (!ptr)
        return;

    *static_cast<void**>(ptr) = tail;
    tail = static_cast<void**>(ptr);
}
