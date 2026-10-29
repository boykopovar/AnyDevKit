add_library(AnyDevKitPublicSdk STATIC
    src/Project/Json.cpp
    src/Project/Metadata.cpp
    src/Application.cpp
    src/CommandLine/Help.cpp
    src/CommandLine/Arguments.cpp
    src/CommandLine/Options.cpp
    src/Commands/ProjectCommand.cpp
    src/Commands/BuildCommand.cpp
    src/Commands/MetadataCommand.cpp
    src/Commands/NidCommand.cpp
    src/Commands/PackagingCommand.cpp
    src/Commands/ShaderPipelineCommand.cpp
    src/Commands/ShaderCommand.cpp
    src/Shader/ShaderMetadata.cpp
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
    AnyDevKit::LibcModuleBuilder
    AnyDevKit::AgcPackager
    PRIVATE
    nlohmann_json::nlohmann_json
    AnyDevKitNidCore
    AnyDevKit::FfpfscPackager
    AnyDevKit::KeystoneBuilder
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
install(TARGETS adk RUNTIME DESTINATION bin COMPONENT sdk)
install(DIRECTORY templates/ DESTINATION share/AnyDevKit/templates COMPONENT sdk)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/sdk/" DESTINATION lib/cmake/AnyDevKit COMPONENT sdk)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/3rdparty/AnyPS5/core/libs/" DESTINATION include/anyps5 COMPONENT sdk FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "tests" EXCLUDE)
if(TARGET AnyDevKitWindowsStartup)
    install(TARGETS AnyDevKitWindowsStartup ARCHIVE DESTINATION lib COMPONENT sdk)
endif()

if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME sdk_project_commands COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/ProjectCommands.py" --adk "$<TARGET_FILE:adk>" --templates "${CMAKE_CURRENT_SOURCE_DIR}/templates")
    set_tests_properties(sdk_project_commands PROPERTIES TIMEOUT 60)
endif()

install(FILES "${PROJECT_SOURCE_DIR}/3rdparty/json/LICENSE.MIT" DESTINATION share/AnyDevKit/licenses/json COMPONENT sdk)
install(FILES "${PROJECT_SOURCE_DIR}/docs/USAGE.md" DESTINATION share/AnyDevKit/docs COMPONENT sdk)

install(FILES "${PROJECT_SOURCE_DIR}/cmake/VerifyRuntimeImports.cmake" DESTINATION lib/cmake/AnyDevKit COMPONENT sdk)

install(FILES "${PROJECT_SOURCE_DIR}/3rdparty/ps5-nid-map/LICENSE" DESTINATION share/AnyDevKit/licenses/ps5-nid-map COMPONENT sdk)
