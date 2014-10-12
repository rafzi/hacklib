#ifndef FOREIGNCLASS_H
#define FOREIGNCLASS_H

#include <cstdint>


// a foreign class whose layout is defined at runtime
class ForeignClass
{
public:
    ForeignClass(uintptr_t ptr) : m_ptr(ptr)
    {
    }

    template <typename T>
    T get(uintptr_t offset) const
    {
        return *(T*)(m_ptr + offset);
    }

    template <typename T>
    void set(uintptr_t offset, T const& value)
    {
        *(T*)(m_ptr + offset) = value;
    }

    template <typename T, typename... Ts>
    T call(uintptr_t offset, Ts... args)
    {
        return (
            (T(__thiscall*)(uintptr_t, Ts...))
            (*(uintptr_t*)((*(uintptr_t*)m_ptr) + offset)))(m_ptr, args...);
    }

    explicit operator bool() const
    {
        return m_ptr != 0;
    }

private:
    uintptr_t m_ptr;

};


#endif