cmake_minimum_required(VERSION 3.22.1)
project(AnyDevKit LANGUAGES C CXX)

add_subdirectory(cmake/dependencies)

if(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(core/shader/AgcPackager)
add_subdirectory(core/shader/compiler)

option(ANYDEVKIT_BUILD_EXAMPLES "Build examples" OFF)
if(ANYDEVKIT_BUILD_EXAMPLES)
    add_subdirectory(examples/Calculator)
endif()
