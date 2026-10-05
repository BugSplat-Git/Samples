// BugSplatStlCompat.cpp
//
// Scalar fallbacks for the MSVC STL vectorized helpers that a BugSplat.lib built
// with a newer toolset references but an older toolset's C++ runtime does not
// export.
//
// BugSplat.lib is a static library; its STL references are resolved at the
// consumer's link against the consumer's toolset. MSVC binary compatibility is
// one-directional: objects built with an older toolset link into a newer one,
// but not the reverse. Linking a newer-built BugSplat.lib from an older toolset
// therefore fails with LNK2019 on symbols such as
// __std_find_last_of_trivial_pos_2.
//
// Each fallback is registered with /ALTERNATENAME, so the linker binds it only
// when the corresponding STL symbol is otherwise unresolved. On a toolset that
// exports the symbol, the native implementation is used and this file has no
// effect. It may therefore be added to any project without version guards.
//
// The implementations use explicit-predicate algorithms so that the compiler
// does not re-vectorize them into the symbols being defined.
//
// Applies to x64 only; ARM64 does not emit these helpers.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>

#if defined(_M_X64)

extern "C" {

// std::find on 16-bit elements: the first element equal to value in [first, last), else last.
const void* __stdcall BugSplatStlCompat_find_trivial_2(
    const void* const first, const void* const last, const uint16_t value) noexcept
{
    const uint16_t* const begin = static_cast<const uint16_t*>(first);
    const uint16_t* const end   = static_cast<const uint16_t*>(last);
    return std::find_if(begin, end, [value](uint16_t x) { return x == value; }); // == last if absent
}

// Reverse find on 16-bit elements: the last element equal to value in [first, last), else last.
const void* __stdcall BugSplatStlCompat_find_last_trivial_2(
    const void* const first, const void* const last, const uint16_t value) noexcept
{
    const uint16_t* const begin = static_cast<const uint16_t*>(first);
    const uint16_t* const end   = static_cast<const uint16_t*>(last);
    const auto rend = std::make_reverse_iterator(begin);
    const auto it   = std::find_if(std::make_reverse_iterator(end), rend,
                                   [value](uint16_t x) { return x == value; });
    return (it == rend) ? last : &*it;
}

// std::search: the first occurrence of the count2-element needle in [first1, last1), else last1.
const void* __stdcall BugSplatStlCompat_search_2(
    const void* const first1, const void* const last1,
    const void* const first2, const size_t count2) noexcept
{
    const uint16_t* const needle = static_cast<const uint16_t*>(first2);
    return std::search(static_cast<const uint16_t*>(first1), static_cast<const uint16_t*>(last1),
                       needle, needle + count2,
                       [](uint16_t l, uint16_t r) { return l == r; });
}

// Reverse find_first_of: the index of the last haystack element that is in the needle,
// else size_t(-1). An empty needle can match nothing.
__declspec(noalias) size_t __stdcall BugSplatStlCompat_find_last_of_trivial_pos_2(
    const void* const haystack, const size_t haystackLength,
    const void* const needle,   const size_t needleLength) noexcept
{
    if (needleLength == 0)
        return static_cast<size_t>(-1);
    const uint16_t* const hay = static_cast<const uint16_t*>(haystack);
    const uint16_t* const set = static_cast<const uint16_t*>(needle);
    const auto rend = std::make_reverse_iterator(hay);
    const auto it   = std::find_first_of(std::make_reverse_iterator(hay + haystackLength), rend,
                                         set, set + needleLength,
                                         [](uint16_t a, uint16_t b) { return a == b; });
    return (it == rend) ? static_cast<size_t>(-1) : static_cast<size_t>(&*it - hay);
}

// Element-wise compare of count 16-bit elements: the index of the first difference, else count.
__declspec(noalias) size_t __stdcall BugSplatStlCompat_mismatch_2(
    const void* const first1, const void* const first2, const size_t count) noexcept
{
    const uint16_t* const a = static_cast<const uint16_t*>(first1);
    const uint16_t* const b = static_cast<const uint16_t*>(first2);
    const auto r = std::mismatch(a, a + count, b, [](uint16_t x, uint16_t y) { return x == y; });
    return static_cast<size_t>(r.first - a);
}

} // extern "C"

// Bind each STL symbol to its fallback only when the linker cannot otherwise resolve it.
#pragma comment(linker, "/alternatename:__std_find_trivial_2=BugSplatStlCompat_find_trivial_2")
#pragma comment(linker, "/alternatename:__std_find_last_trivial_2=BugSplatStlCompat_find_last_trivial_2")
#pragma comment(linker, "/alternatename:__std_search_2=BugSplatStlCompat_search_2")
#pragma comment(linker, "/alternatename:__std_find_last_of_trivial_pos_2=BugSplatStlCompat_find_last_of_trivial_pos_2")
#pragma comment(linker, "/alternatename:__std_mismatch_2=BugSplatStlCompat_mismatch_2")

#endif // _M_X64
