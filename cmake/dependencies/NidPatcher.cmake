set(nidRoot "${PROJECT_SOURCE_DIR}/3rdparty/AnyPS5/core/libs/nid")
add_executable(AnyDevKitNidPatcher
    "${nidRoot}/src/Sha1.cpp"
    "${nidRoot}/src/NidCompute.cpp"
    "${nidRoot}/src/NidResolver.cpp"
    "${nidRoot}/src/ExportExclusions.cpp"
    "${nidRoot}/src/ElfNidPatcher.cpp"
    "${nidRoot}/src/PeNidPatcher.cpp"
    "${nidRoot}/src/MachONidPatcher.cpp"
    "${nidRoot}/src/BinaryPatcherFactory.cpp"
    "${nidRoot}/src/main.cpp"
)
target_include_directories(AnyDevKitNidPatcher PRIVATE "${nidRoot}/include")
target_compile_features(AnyDevKitNidPatcher PRIVATE cxx_std_20)
if(MINGW)
    target_link_options(AnyDevKitNidPatcher PRIVATE -municode -static -static-libgcc -static-libstdc++)
    target_link_libraries(AnyDevKitNidPatcher PRIVATE Threads::Threads)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_link_options(AnyDevKitNidPatcher PRIVATE -fuse-ld=lld)
    endif()
endif()
