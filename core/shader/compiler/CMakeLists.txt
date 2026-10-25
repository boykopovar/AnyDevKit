add_library(AnyDevKitShaderCompiler STATIC
    src/ShaderCompiler.cpp
    src/Frontend/GlslCompiler.cpp
    src/Frontend/ShaderIncluder.cpp
    src/Backend/PsbcCompiler.cpp
)
add_library(AnyDevKit::ShaderCompiler ALIAS AnyDevKitShaderCompiler)
target_compile_features(AnyDevKitShaderCompiler PUBLIC cxx_std_20)
set_target_properties(AnyDevKitShaderCompiler PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(AnyDevKitShaderCompiler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(AnyDevKitShaderCompiler PUBLIC AnyDevKit::AgcPackager PRIVATE AnyDevKitPsbc glslang::glslang glslang::SPIRV glslang::glslang-default-resource-limits)
