cmake_minimum_required(VERSION 3.22.1)
project(AnyDevKit LANGUAGES C CXX)

add_subdirectory(cmake/dependencies)

if(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(core/shader/AgcPackager)
add_subdirectory(core/shader/compiler)
add_subdirectory(core/Crypto)
add_subdirectory(core/Packager/EbootPackager)
add_subdirectory(core/AppMeta/ParamBuilder)
add_subdirectory(core/AppMeta/IconBuilder)
add_subdirectory(core/Packager/PfsBuilder)
add_subdirectory(core/Packager/FfpfscPackager)
add_subdirectory(core/Packager/PkgPackager)
add_subdirectory(core/Linker/Ps5ElfBuilder)

option(ANYDEVKIT_BUILD_EXAMPLES "Build examples" OFF)
if(ANYDEVKIT_BUILD_EXAMPLES)
    add_subdirectory(examples/Calculator)
endif()
