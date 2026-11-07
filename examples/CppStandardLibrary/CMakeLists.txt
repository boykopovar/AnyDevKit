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

if(NOT TARGET Calculator3DShaderBuilder)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../Calculator3D/ShaderBuilder" "${CMAKE_CURRENT_BINARY_DIR}/shader-builder")
endif()
string(TOLOWER "${CMAKE_SYSTEM_NAME}" cppShaderTarget)
set(ANYDEVKIT_CPP_SHADER_TARGET "${cppShaderTarget}" CACHE STRING "Standard library example shader target")
set(shaderHeader "${CMAKE_CURRENT_BINARY_DIR}/generated/Calculator3DShaders.hpp")
set(shaderDirectory "${CMAKE_CURRENT_LIST_DIR}/../Calculator3D/shaders")
add_custom_command(OUTPUT "${shaderHeader}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "$<TARGET_FILE:Calculator3DShaderBuilder>" "${shaderDirectory}/Background.vert" "${shaderDirectory}/Background.frag" "${shaderDirectory}/Interface.vert" "${shaderDirectory}/Interface.frag" "${shaderHeader}" --target "${ANYDEVKIT_CPP_SHADER_TARGET}"
    DEPENDS Calculator3DShaderBuilder "${shaderDirectory}/Background.vert" "${shaderDirectory}/Background.frag" "${shaderDirectory}/Interface.vert" "${shaderDirectory}/Interface.frag"
    VERBATIM)

add_executable(CppStandardLibrary
    "${shaderHeader}"
    src/Graphics/Memory.cpp
    src/Graphics/Pipeline.cpp
    src/Graphics/Renderer.cpp
    src/Main.cpp
    src/Screen.cpp
    ../Calculator3D/src/Canvas.cpp
    ../Calculator3D/src/Display.cpp
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
    src/CCompatibility.cpp
    src/Mutexes.cpp
    src/Threads.cpp
    src/Clocks.cpp)
option(ANYDEVKIT_CPP_HEADLESS "Run standard library checks without a window" OFF)
target_compile_definitions(CppStandardLibrary PRIVATE ANYDEVKIT_CPP_HEADLESS=$<BOOL:${ANYDEVKIT_CPP_HEADLESS}>)
target_compile_features(CppStandardLibrary PRIVATE cxx_std_20)
set_target_properties(CppStandardLibrary PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(CppStandardLibrary PRIVATE include ../Calculator3D/include "${CMAKE_CURRENT_BINARY_DIR}/generated")
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ReleaseHeaders.cmake")
anydevkit_use_release_headers(CppStandardLibrary)
target_link_libraries(CppStandardLibrary PRIVATE AnyLibcxx::Runtime)
target_compile_options(CppStandardLibrary PRIVATE -fPIC -fno-stack-protector -fvisibility=hidden -Wall -Wextra -Wpedantic -Werror)
if(WIN32)
    if(NOT COMMAND anydevkit_configure_windows_executable)
        message(FATAL_ERROR "Configure the SDK root for a Windows executable")
    endif()
    anydevkit_configure_windows_executable(CppStandardLibrary ${ANYLIBCXX_WINDOWS_IMPORTS} __cxa_get_globals)
    anydevkit_link_runtime_library(CppStandardLibrary libkernel
        sceKernelMapFlexibleMemory sceKernelMunmap sceKernelAllocateDirectMemory sceKernelGetDirectMemorySize sceKernelMapDirectMemory sceKernelReleaseDirectMemory
        sceKernelGetProcessTime sceKernelUsleep_nid_postfix sceKernelClockGettime sceKernelNanosleep scePthreadGetaffinity
        scePthreadMutexInit scePthreadMutexDestroy scePthreadMutexLock scePthreadMutexTrylock scePthreadMutexUnlock
        scePthreadMutexattrInit scePthreadMutexattrDestroy scePthreadMutexattrSettype
        scePthreadCondInit scePthreadCondDestroy scePthreadCondSignal scePthreadCondBroadcast scePthreadCondWait scePthreadCondTimedwait
        scePthreadCreate scePthreadJoin scePthreadDetach scePthreadSelf scePthreadYield
        scePthreadKeyCreate scePthreadKeyDelete scePthreadGetspecific scePthreadSetspecific)
    anydevkit_link_runtime_library(CppStandardLibrary libSceVideoOut sceVideoOutOpen sceVideoOutSetBufferAttribute2 sceVideoOutRegisterBuffers2 sceVideoOutSubmitFlip sceVideoOutWaitVblank sceVideoOutGetFlipStatus sceVideoOutGetBufferLabelAddress sceVideoOutInitializeOutputOptions sceVideoOutIsOutputSupported sceVideoOutConfigureOutput sceVideoOutGetOutputStatus)
    anydevkit_link_runtime_library(CppStandardLibrary libSceSystemService sceSystemServiceHideSplashScreen)
    anydevkit_link_runtime_library(CppStandardLibrary libSceAgc sceAgcSuspendPoint sceAgcInit sceAgcCreateShader sceAgcGetRegisterDefaults sceAgcLinkShaders sceAgcDcbContextStateOp sceAgcDcbAcquireMem sceAgcDcbSetCxRegistersIndirect sceAgcDcbSetUcRegistersIndirect sceAgcDcbSetShRegistersIndirect sceAgcDcbSetShRegisterDirect sceAgcDcbSetNumInstances sceAgcDcbDrawIndexAuto sceAgcDcbSetFlip sceAgcCbReleaseMem)
    anydevkit_link_runtime_library(CppStandardLibrary libSceAgcDriver sceAgcDriverSubmitDcb)
else()
    set_target_properties(CppStandardLibrary PROPERTIES SUFFIX ".elf")
    if(NOT EXISTS "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/libc.prx")
        message(FATAL_ERROR "Missing ELF libc.prx import library")
    endif()
    target_link_options(CppStandardLibrary PRIVATE -nostdlib -fuse-ld=lld -shared "-Wl,--entry=main,--hash-style=sysv,--eh-frame-hdr,--no-undefined,-z,max-page-size=16384")
    foreach(library libc libkernel libSceVideoOut libSceSystemService libSceAgc libSceAgcDriver)
        if(NOT EXISTS "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/${library}.prx")
            message(FATAL_ERROR "Missing ELF ${library}.prx import library")
        endif()
        target_link_libraries(CppStandardLibrary PRIVATE "${ANYDEVKIT_CPP_IMPORT_DIRECTORY}/${library}.prx")
    endforeach()
endif()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/app0/sce_sys/param.json" CONTENT [=[
{"titleId":"FAKE26102","attribute3":64,"localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"AnyDevKit C++ Standard Library"}}}
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

