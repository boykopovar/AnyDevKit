cmake_minimum_required(VERSION 3.22)
project(AnyDevKit LANGUAGES CXX)

if(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(core/shader/AgcPackager)
