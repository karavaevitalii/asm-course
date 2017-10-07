#ifndef ALLOCATOR_H
#define ALLOCATOR_H

namespace utils
{
    class allocator
    {
        void* tail;
        void* head;

        allocator();
    public:
        ~allocator();

        allocator(allocator&&) = default;

        allocator(allocator const&)     = delete;
        allocator& operator=(allocator) = delete;

        static allocator& get_instance();

        void* allocate();
        void deallocate(void*);
    };
} // namespace utils

#endif // ALLOCATOR_H