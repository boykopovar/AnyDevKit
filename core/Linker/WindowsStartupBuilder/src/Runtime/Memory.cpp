#include "Runtime/Memory.hpp"
#include "prx/libc/include/Api.hpp"

extern "C" {
void* memcpy(void* destination, const void* source, std::size_t count) { return memcpy_nid_postfix(destination, source, count); }
void* memmove(void* destination, const void* source, std::size_t count) { return memmove_nid_postfix(destination, source, count); }
void* memset(void* destination, int value, std::size_t count) { return memset_nid_postfix(destination, value, count); }
int memcmp(const void* left, const void* right, std::size_t count) { return memcmp_nid_postfix(left, right, count); }
std::size_t strlen(const char* text) { return strlen_nid_postfix(text); }
}

auto memchr(const void* source, int value, std::size_t count) -> decltype(::memchr(source, value, count)) { return const_cast<void*>(memchr_nid_postfix(source, value, count)); }
