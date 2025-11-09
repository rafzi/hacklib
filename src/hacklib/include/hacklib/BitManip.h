#ifndef HACKLIB_BITMANIP_H
#define HACKLIB_BITMANIP_H

#include <cstdint>


namespace hl
{

// Aligns a value v to the next value aligned by align bits.
template <typename T>
T Align(T v, T align)
{
    return (v + align - 1) & ~(align - 1);
}

// Convenience overload for pointer types.
template <typename T, typename U>
    requires std::is_pointer_v<T>
T Align(T v, U align)
{
    return (T)(((uintptr_t)v + (uintptr_t)align - 1) & ~((uintptr_t)align - 1));
}

// Aligns a value v to the previous value aligned by align bits.
template <typename T>
T AlignDown(T v, T align)
{
    return v & ~(align - 1);
}

// Convenience overload for pointer types.
template <typename T, typename U>
    requires std::is_pointer_v<T>
T AlignDown(T v, U align)
{
    return (T)((uintptr_t)v & ~((uintptr_t)align - 1));
}

// Returns true if the given ranges [start, end] overlap.
template <typename T>
bool HasOverlapInclusive(T r1start, T r1end, T r2start, T r2end)
{
    return r1start <= r2end && r2start <= r1end;
}

// Returns true if the given ranges [start, end) overlap.
template <typename T>
bool HasOverlap(T r1start, T r1end, T r2start, T r2end)
{
    return r1start < r2end && r2start < r1end;
}

// Returns a mask value with the given number of bits set to one.
template <typename T = uint32_t>
T MakeMask(int width)
{
    return width ? (((T)(-1)) >> ((sizeof(T) * 8) - width)) : 0;
}

// Returns a mask value with bits startBit to startBit+width set to one.
template <typename T = uint32_t>
T MakeMask(int startBit, int width)
{
    return MakeMask<T>(width) << startBit;
}

} // namespace eda

#endif
