set(anydevkitRoot "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
get_filename_component(anydevkitRoot "${anydevkitRoot}" ABSOLUTE)

foreach(requiredHeader IN ITEMS
    "${anydevkitRoot}/3rdparty/AnyPS5/core/libs/SceShaders.hpp"
    "${anydevkitRoot}/3rdparty/opengnm-psbc/libpsbc/psbc_compile.h")
    if(NOT EXISTS "${requiredHeader}")
        message(FATAL_ERROR "Required submodule header is missing: ${requiredHeader}")
    endif()
endforeach()

add_library(AnyDevKitAgcPackager STATIC
    src/AgcPackager.cpp
    src/Builder/AgcRegisterBuilder.cpp
    src/Builder/AgcResourceBuilder.cpp
    src/Builder/AgcSemanticBuilder.cpp
    src/Validator/AgcMetadataValidator.cpp
    src/Writer/AgcBinaryWriter.cpp
)
add_library(AnyDevKit::AgcPackager ALIAS AnyDevKitAgcPackager)
target_compile_features(AnyDevKitAgcPackager PUBLIC cxx_std_20)
set_target_properties(AnyDevKitAgcPackager PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(AnyDevKitAgcPackager PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${anydevkitRoot}/3rdparty/AnyPS5/core/libs"
    "${anydevkitRoot}/3rdparty/opengnm-psbc"
)
