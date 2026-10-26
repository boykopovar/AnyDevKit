set(ANYDEVKIT_RUNTIME_DIRECTORY "" CACHE PATH "Directory containing patched AnyPS5 runtime libraries")

if(NOT MINGW OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "Calculator currently requires Clang targeting MinGW with DWARF exceptions")
endif()
find_program(ANYDEVKIT_DLLTOOL NAMES llvm-dlltool REQUIRED)
find_program(ANYDEVKIT_OBJCOPY NAMES objcopy HINTS "${CMAKE_SYSROOT}/bin" REQUIRED)


if(NOT EXISTS "${ANYDEVKIT_RUNTIME_DIRECTORY}/libSceVideoOut.prx")
    message(FATAL_ERROR "ANYDEVKIT_RUNTIME_DIRECTORY must contain libSceVideoOut.prx")
endif()

set(CMAKE_CXX_STANDARD_LIBRARIES "")
add_executable(Calculator src/Main.cpp src/Canvas.cpp src/PeRelocations.cpp)
target_compile_features(Calculator PRIVATE cxx_std_20)
set_target_properties(Calculator PROPERTIES CXX_EXTENSIONS OFF)
add_dependencies(Calculator AnyDevKitNidPatcher)
set_property(TARGET Calculator APPEND PROPERTY LINK_DEPENDS "$<TARGET_FILE:AnyDevKitNidPatcher>")
target_include_directories(Calculator PRIVATE include)
target_include_directories(Calculator SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/3rdparty/AnyPS5/core/libs")
target_compile_options(Calculator PRIVATE -fdwarf-exceptions -fno-stack-protector -fno-stack-check -mno-stack-arg-probe)
target_link_options(Calculator PRIVATE -nostdlib -fuse-ld=lld -Wl,--entry,CalculatorEntry -Wl,--subsystem,windows)

set(nativeExceptionSymbols __cxa_allocate_exception __cxa_free_exception __cxa_throw __cxa_begin_catch __cxa_end_catch __cxa_rethrow __cxa_call_terminate __gxx_personality_v0 _Unwind_Resume _ZSt9terminatev)
string(JOIN "\n" preservedExceptions ${nativeExceptionSymbols})
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-exceptions.txt" CONTENT "${preservedExceptions}\n" NEWLINE_STYLE UNIX)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libc.def" "LIBRARY libc.prx\nEXPORTS\n${preservedExceptions}\nmalloc_nid_postfix\nfree_nid_postfix\nprintf_nid_postfix\nexit_nid_postfix\n_ZTVN10__cxxabiv117__class_type_infoE DATA\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libSceVideoOut.def" "LIBRARY libSceVideoOut.prx\nEXPORTS\nsceVideoOutOpen\nsceVideoOutSetBufferAttribute2\nsceVideoOutRegisterBuffers2\nsceVideoOutSubmitFlip\nsceVideoOutWaitVblank\n")
foreach(library IN ITEMS libc libSceVideoOut)
    set(importLibrary "${CMAKE_CURRENT_BINARY_DIR}/${library}.a")
    add_custom_command(OUTPUT "${importLibrary}"
        COMMAND "${ANYDEVKIT_DLLTOOL}" -m i386:x86-64 -d "${CMAKE_CURRENT_BINARY_DIR}/${library}.def" -l "${importLibrary}"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${library}.def"
        VERBATIM)
    add_custom_target(CalculatorImport_${library} DEPENDS "${importLibrary}")
    add_dependencies(Calculator CalculatorImport_${library})
    target_link_libraries(Calculator PRIVATE "${importLibrary}")
endforeach()
add_custom_command(TARGET Calculator POST_BUILD
    COMMAND "${ANYDEVKIT_OBJCOPY}" --rename-section .eh_frame=.ehfram --set-section-flags .rdata=alloc,load,data,contents "$<TARGET_FILE:Calculator>"
    COMMAND "$<TARGET_FILE:AnyDevKitNidPatcher>" --exec --preserve-export-list "${CMAKE_CURRENT_BINARY_DIR}/native-exceptions.txt" "$<TARGET_FILE:Calculator>"
    VERBATIM)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/app0/sce_sys/param.json" CONTENT [=[
{"titleId":"CALC00001","localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"AnyDevKit Calculator"}}}
]=])

get_filename_component(compilerDirectory "${CMAKE_CXX_COMPILER}" DIRECTORY)
if(WIN32)
    set(runtimeEnvironment "set(ENV{PATH} \"${ANYDEVKIT_RUNTIME_DIRECTORY};${compilerDirectory};\$ENV{PATH}\")")
elseif(APPLE)
    set(runtimeEnvironment "set(ENV{DYLD_LIBRARY_PATH} \"${ANYDEVKIT_RUNTIME_DIRECTORY}:\$ENV{DYLD_LIBRARY_PATH}\")")
else()
    set(runtimeEnvironment "set(ENV{LD_LIBRARY_PATH} \"${ANYDEVKIT_RUNTIME_DIRECTORY}:\$ENV{LD_LIBRARY_PATH}\")")
endif()

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/RunCalculator-$<CONFIG>.cmake" CONTENT "${runtimeEnvironment}
execute_process(COMMAND \"$<TARGET_FILE:Calculator>\" WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\" COMMAND_ERROR_IS_FATAL ANY)
")
add_custom_target(run_calculator
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/RunCalculator-$<CONFIG>.cmake"
    DEPENDS Calculator
    USES_TERMINAL
    VERBATIM)
