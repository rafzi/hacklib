#ifndef HACKLIB_EXECUTABLE_ALLOCATOR_H
#define HACKLIB_EXECUTABLE_ALLOCATOR_H

#include <cstdint>
#include <vector>
#include <cstddef>


namespace hl {


static const int PROTECTION_READ = 0x1;
static const int PROTECTION_WRITE = 0x2;
static const int PROTECTION_EXECUTE = 0x4;
static const int PROTECTION_GUARD = 0x8; // Only supported on Windows.
static const int PROTECTION_READ_WRITE = PROTECTION_READ|PROTECTION_WRITE;
static const int PROTECTION_READ_EXECUTE = PROTECTION_READ|PROTECTION_EXECUTE;
static const int PROTECTION_READ_WRITE_EXECUTE = PROTECTION_READ_WRITE|PROTECTION_EXECUTE;


uintptr_t GetPageSize();

void *PageAlloc(size_t n, int protection);
void PageFree(void *p, size_t n = 0);
void PageProtect(const void *p, size_t n, int protection);

template <typename T, typename A>
void PageProtectVec(const std::vector<T, A>& vec, int protection)
{
    PageProtect(vec.data(), vec.size()*sizeof(T), protection);
}


// This allocator cannot directly be using in standard containers, because
// of the second template argument.
template <typename T, int P>
class page_allocator
{
public:
    typedef T value_type;

    T *allocate(size_t n)
    {
        T *adr = (T*)PageAlloc(n * sizeof(T), P);
        if (!adr)
        {
            throw std::bad_alloc();
        }
        return adr;
    }
    void deallocate(T *p, size_t n)
    {
        PageFree(p, n * sizeof(T));
    }
};


template <typename T>
class data_page_allocator : public page_allocator<T, PROTECTION_READ_WRITE>
{
};

template <typename T>
using data_page_vector = std::vector<T, data_page_allocator<T>>;

template <typename T>
class code_page_allocator : public page_allocator<T, PROTECTION_READ_WRITE_EXECUTE>
{
};

typedef std::vector<unsigned char, code_page_allocator<unsigned char>> code_page_vector;


}

#endif