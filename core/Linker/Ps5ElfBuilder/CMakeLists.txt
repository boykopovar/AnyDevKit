add_library(AnyDevKitPs5ElfBuilder STATIC
    src/Ps5ElfBuilder.cpp
    src/Reader/ElfImage.cpp
    src/Imports/ImportCatalog.cpp
    src/Builder/ProcessBuilder.cpp
    src/Builder/StartupBuilder.cpp
    src/Builder/UnwindBuilder.cpp
    src/Writer/ElfWriter.cpp
    src/Writer/BinaryWriter.cpp
)
add_library(AnyDevKit::Ps5ElfBuilder ALIAS AnyDevKitPs5ElfBuilder)
target_include_directories(AnyDevKitPs5ElfBuilder PUBLIC include)
target_link_libraries(AnyDevKitPs5ElfBuilder PRIVATE AnyDevKitNidCore AnyDevKitCrypto)
target_compile_features(AnyDevKitPs5ElfBuilder PUBLIC cxx_std_20)
set_target_properties(AnyDevKitPs5ElfBuilder PROPERTIES CXX_EXTENSIONS OFF)
