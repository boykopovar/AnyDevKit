cmake_minimum_required(VERSION 3.22.1)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(CppStandardLibrary LANGUAGES CXX)
endif()

set(ANYDEVKIT_LIBCXX_BUILD "" CACHE PATH "Configured AnyLibcxx build directory")
set(ANYDEVKIT_CPP_IMPORT_DIRECTORY "" CACHE PATH "ELF libc.prx import library directory")
set(runtimeRoot "${ANYDEVKIT_LIBCXX_BUILD}/3rdparty/llvm-project/runtimes")
foreach(required "${runtimeRoot}/include/c++/v1/__config_site" "${runtimeRoot}/lib/libc++.a" "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/libc.prx")
    if(NOT EXISTS "${required}")
        message(FATAL_ERROR "Missing C++ example dependency: ${required}")
    endif()
endforeach()
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    message(FATAL_ERROR "The PS5 C++ example requires Clang targeting x86_64")
endif()

add_executable(CppStandardLibrary
    src/Main.cpp
    src/Containers.cpp
    src/Memory.cpp
    src/Algorithms.cpp
    src/Utilities.cpp
    src/Strings.cpp
    src/Numeric.cpp
    src/Exceptions.cpp
    src/Coroutines.cpp
    src/Atomics.cpp
    src/Locale.cpp
    src/Streams.cpp
    src/Regex.cpp
    src/Formatting.cpp
    src/CCompatibility.cpp)
target_compile_features(CppStandardLibrary PRIVATE cxx_std_20)
target_compile_definitions(CppStandardLibrary PRIVATE __NO_CTYPE __errno_location=_Geterrno)
set_target_properties(CppStandardLibrary PROPERTIES CXX_EXTENSIONS OFF SUFFIX ".elf")
target_include_directories(CppStandardLibrary PRIVATE include)
target_include_directories(CppStandardLibrary SYSTEM PRIVATE "${runtimeRoot}/include/c++/v1")
target_compile_options(CppStandardLibrary PRIVATE -nostdinc++ -fPIC -fno-stack-protector -fvisibility=hidden -Wall -Wextra -Wpedantic -Werror)
target_link_options(CppStandardLibrary PRIVATE -nostdlib -fuse-ld=lld -shared "-Wl,--entry=main,--hash-style=sysv,--eh-frame-hdr,--no-undefined,-z,max-page-size=16384")
target_link_libraries(CppStandardLibrary PRIVATE "${runtimeRoot}/lib/libc++.a" "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/libc.prx")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/app0/sce_sys/param.json" CONTENT [=[
{"titleId":"FAKE26102","localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"AnyDevKit C++ Standard Library"}}}
]=])
