#ifndef HACKLIB_EXECUTABLE_ALLOCATOR_H
#define HACKLIB_EXECUTABLE_ALLOCATOR_H

#include <cstdint>
#include <vector>


namespace hl {


void *ExecutableAlloc(size_t n);
void ExecutableFree(void *p);


template <typename T>
class executable_allocator
{
public:
    typedef T value_type;

    executable_allocator()
    {
    }
    executable_allocator(const executable_allocator<T>& other)
    {
    }
    T *allocate(std::size_t n)
    {
        T *adr = (T*)ExecutableAlloc(n * sizeof(T));
        if (!adr)
            throw std::bad_alloc();
        return adr;
    }
    void deallocate(T *p, std::size_t n)
    {
        ExecutableFree(p);
    }
};

template <typename T>
bool operator==(const executable_allocator<T>& lhs, const executable_allocator<T>& rhs)
{
    return true;
}
template <typename T>
bool operator!=(const executable_allocator<T>& lhs, const executable_allocator<T>& rhs)
{
    return false;
}


typedef std::vector<unsigned char, executable_allocator<unsigned char>> code_vector;


}

#endif