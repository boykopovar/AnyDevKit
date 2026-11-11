add_library(AnyDevKitShaderCompiler STATIC
    src/ShaderCompiler.cpp
    src/Frontend/GlslCompiler.cpp
    src/Frontend/ShaderIncluder.cpp
    src/Validator/ShaderLayoutValidator.cpp
)
add_library(AnyDevKit::ShaderCompiler ALIAS AnyDevKitShaderCompiler)
target_compile_features(AnyDevKitShaderCompiler PUBLIC cxx_std_20)
set_target_properties(AnyDevKitShaderCompiler PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(AnyDevKitShaderCompiler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(AnyDevKitShaderCompiler PUBLIC AnyDevKit::SpirvCompiler PRIVATE glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_library(AnyDevKitSpirvCompiler STATIC src/Backend/PsbcCompiler.cpp)
add_library(AnyDevKit::SpirvCompiler ALIAS AnyDevKitSpirvCompiler)
target_compile_features(AnyDevKitSpirvCompiler PUBLIC cxx_std_20)
set_target_properties(AnyDevKitSpirvCompiler PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(AnyDevKitSpirvCompiler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(AnyDevKitSpirvCompiler PUBLIC AnyDevKit::AgcPackager PRIVATE AnyDevKitPsbc)

