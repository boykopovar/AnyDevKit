#ifndef ANYDEVKIT_WINDOWS_STARTUP_RUNTIME_MEMORY_HPP
#define ANYDEVKIT_WINDOWS_STARTUP_RUNTIME_MEMORY_HPP

#include <cstddef>

extern "C" {
void* memcpy(void* destination, const void* source, std::size_t count);
void* memmove(void* destination, const void* source, std::size_t count);
void* memset(void* destination, int value, std::size_t count);
int memcmp(const void* left, const void* right, std::size_t count);
void* memchr(const void* source, int value, std::size_t count);
std::size_t strlen(const char* text);
}

#endif
