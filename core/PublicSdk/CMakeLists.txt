add_library(AnyDevKitPublicSdk STATIC
    src/Application.cpp
    src/CommandLine/Arguments.cpp
    src/CommandLine/Options.cpp
    src/Commands/BuildCommand.cpp
    src/Commands/MetadataCommand.cpp
    src/Commands/NidCommand.cpp
    src/Commands/PackagingCommand.cpp
    src/Commands/ShaderCommand.cpp
    src/IO/Files.cpp
    src/Pipeline/ApplicationBuilder.cpp
)
add_library(AnyDevKit::PublicSdk ALIAS AnyDevKitPublicSdk)
target_include_directories(AnyDevKitPublicSdk PUBLIC include)
target_compile_features(AnyDevKitPublicSdk PUBLIC cxx_std_20)
set_target_properties(AnyDevKitPublicSdk PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(AnyDevKitPublicSdk PUBLIC
    AnyDevKit::EbootPackager
    AnyDevKit::PfsBuilder
    AnyDevKit::PkgPackager
    AnyDevKit::Ps5ElfBuilder
    PRIVATE
    AnyDevKitNidCore
    AnyDevKit::FfpfscPackager
    AnyDevKit::IconBuilder
    AnyDevKit::WindowsStartupBuilder
    AnyDevKit::ShaderCompiler
)
add_executable(adk src/Main.cpp)
target_link_libraries(adk PRIVATE AnyDevKit::PublicSdk)
set_target_properties(adk PROPERTIES CXX_EXTENSIONS OFF)
if(MINGW)
    find_package(Threads REQUIRED)
    target_link_options(adk PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_libraries(adk PRIVATE Threads::Threads)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_link_options(adk PRIVATE -fuse-ld=lld)
    endif()
endif()
install(TARGETS adk RUNTIME DESTINATION bin)
