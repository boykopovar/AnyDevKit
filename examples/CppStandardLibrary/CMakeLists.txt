cmake_minimum_required(VERSION 3.22.1)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(CppStandardLibrary LANGUAGES CXX)
endif()

set(ANYDEVKIT_LIBCXX_BUILD "" CACHE PATH "Configured AnyLibcxx build directory")
set(ANYDEVKIT_CPP_IMPORT_DIRECTORY "" CACHE PATH "ELF libc.prx import library directory")
if(NOT EXISTS "${ANYDEVKIT_LIBCXX_BUILD}/AnyLibcxxConfig.cmake")
    message(FATAL_ERROR "Missing AnyLibcxx build configuration")
endif()
include("${ANYDEVKIT_LIBCXX_BUILD}/AnyLibcxxConfig.cmake")

add_executable(CppStandardLibrary
    src/Main.cpp
    src/Screen.cpp
    ../Calculator/src/Canvas.cpp
    ../Calculator/src/Display.cpp
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
option(ANYDEVKIT_CPP_HEADLESS "Run standard library checks without a window" OFF)
target_compile_definitions(CppStandardLibrary PRIVATE ANYDEVKIT_CPP_HEADLESS=$<BOOL:${ANYDEVKIT_CPP_HEADLESS}>)
target_compile_features(CppStandardLibrary PRIVATE cxx_std_20)
set_target_properties(CppStandardLibrary PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(CppStandardLibrary PRIVATE include ../Calculator/include)
target_include_directories(CppStandardLibrary SYSTEM PRIVATE ../../3rdparty/AnyPS5/core/libs)
target_link_libraries(CppStandardLibrary PRIVATE AnyLibcxx::Runtime)
target_compile_options(CppStandardLibrary PRIVATE -fPIC -fno-stack-protector -fvisibility=hidden -Wall -Wextra -Wpedantic -Werror)
if(WIN32)
    if(NOT COMMAND anydevkit_configure_windows_executable)
        message(FATAL_ERROR "Configure the SDK root for a Windows executable")
    endif()
    anydevkit_configure_windows_executable(CppStandardLibrary ${ANYLIBCXX_WINDOWS_IMPORTS} __cxa_get_globals)
    anydevkit_link_runtime_library(CppStandardLibrary libkernel sceKernelAllocateDirectMemory sceKernelGetDirectMemorySize sceKernelMapDirectMemory sceKernelReleaseDirectMemory sceKernelGetProcessTime)
    anydevkit_link_runtime_library(CppStandardLibrary libSceVideoOut sceVideoOutOpen sceVideoOutSetBufferAttribute2 sceVideoOutRegisterBuffers2 sceVideoOutSubmitFlip sceVideoOutWaitVblank sceVideoOutGetFlipStatus sceVideoOutGetBufferLabelAddress)
else()
    set_target_properties(CppStandardLibrary PROPERTIES SUFFIX ".elf")
    if(NOT EXISTS "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/libc.prx")
        message(FATAL_ERROR "Missing ELF libc.prx import library")
    endif()
    target_link_options(CppStandardLibrary PRIVATE -nostdlib -fuse-ld=lld -shared "-Wl,--entry=main,--hash-style=sysv,--eh-frame-hdr,--no-undefined,-z,max-page-size=16384")
    foreach(library libc libkernel libSceVideoOut)
        if(NOT EXISTS "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/${library}.prx")
            message(FATAL_ERROR "Missing ELF ${library}.prx import library")
        endif()
        target_link_libraries(CppStandardLibrary PRIVATE "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/${library}.prx")
    endforeach()
endif()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/app0/sce_sys/param.json" CONTENT [=[
{"titleId":"FAKE26102","localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"AnyDevKit C++ Standard Library"}}}
]=])

if(WIN32)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/RunCppStandardLibrary.cmake" CONTENT "set(ENV{PATH} \"${ANYDEVKIT_RUNTIME_DIRECTORY};${CMAKE_SYSROOT}/bin;\$ENV{PATH}\")
execute_process(COMMAND \"$<TARGET_FILE:CppStandardLibrary>\" WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\" COMMAND_ERROR_IS_FATAL ANY)
")
    add_custom_target(run_cpp_standard_library
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/RunCppStandardLibrary.cmake"
        DEPENDS CppStandardLibrary
        USES_TERMINAL
        VERBATIM)
endif()
