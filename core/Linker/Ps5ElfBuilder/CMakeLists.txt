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
target_link_libraries(AnyDevKitPs5ElfBuilder PRIVATE AnyDevKitNidCore AnyDevKitCrypto Ps5NidMap::Ps5NidMap)
target_compile_features(AnyDevKitPs5ElfBuilder PUBLIC cxx_std_20)
set_target_properties(AnyDevKitPs5ElfBuilder PROPERTIES CXX_EXTENSIONS OFF)

if(BUILD_TESTING)
    add_executable(Ps5ImportCatalogTests tests/ImportCatalog.cpp)
    target_link_libraries(Ps5ImportCatalogTests PRIVATE AnyDevKit::Ps5ElfBuilder)
    set_target_properties(Ps5ImportCatalogTests PROPERTIES CXX_EXTENSIONS OFF)
    if(MINGW)
        find_package(Threads REQUIRED)
        target_link_options(Ps5ImportCatalogTests PRIVATE -static -static-libgcc -static-libstdc++)
        target_link_libraries(Ps5ImportCatalogTests PRIVATE Threads::Threads)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_link_options(Ps5ImportCatalogTests PRIVATE -fuse-ld=lld)
        endif()
    endif()
    add_test(NAME ps5_import_catalog COMMAND Ps5ImportCatalogTests)
endif()
