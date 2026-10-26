add_library(AnyDevKitEbootPackager STATIC
    src/EbootPackager.cpp
    src/Reader/ElfReader.cpp
    src/Builder/SelfLayoutBuilder.cpp
    src/Writer/SelfWriter.cpp
)
add_library(AnyDevKit::EbootPackager ALIAS AnyDevKitEbootPackager)
target_compile_features(AnyDevKitEbootPackager PUBLIC cxx_std_20)
set_target_properties(AnyDevKitEbootPackager PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(AnyDevKitEbootPackager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(AnyDevKitEbootPackager PRIVATE AnyDevKit::Crypto)
