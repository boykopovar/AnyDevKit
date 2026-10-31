include("${CMAKE_CURRENT_LIST_DIR}/ReleaseHeaders.cmake")

set(ANYDEVKIT_RUNTIME_DIRECTORY "" CACHE PATH "Directory containing patched AnyPS5 runtime libraries")

function(anydevkit_link_runtime_library target library)
    if(NOT EXISTS "${ANYDEVKIT_RUNTIME_DIRECTORY}/${library}.prx")
        message(FATAL_ERROR "Missing AnyPS5 runtime library: ${library}.prx")
    endif()
    find_program(ANYDEVKIT_DLLTOOL NAMES llvm-dlltool REQUIRED)
    string(JOIN "\n" symbols ${ARGN})
    set(definition "${CMAKE_CURRENT_BINARY_DIR}/${target}-${library}.def")
    set(importLibrary "${CMAKE_CURRENT_BINARY_DIR}/${target}-${library}.a")
    file(CONFIGURE OUTPUT "${definition}" CONTENT "LIBRARY ${library}.prx\nEXPORTS\n${symbols}\n" NEWLINE_STYLE UNIX)
    add_custom_command(OUTPUT "${importLibrary}"
        COMMAND "${ANYDEVKIT_DLLTOOL}" -m i386:x86-64 -d "${definition}" -l "${importLibrary}"
        DEPENDS "${definition}"
        VERBATIM)
    add_custom_target(${target}Import_${library} DEPENDS "${importLibrary}")
    add_dependencies(${target} ${target}Import_${library})
    target_link_libraries(${target} PRIVATE "${importLibrary}")
endfunction()

function(anydevkit_configure_windows_executable target)
    get_filename_component(sdkRoot "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    if(NOT MINGW OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Windows startup requires Clang targeting MinGW with DWARF exceptions")
    endif()
    find_program(ANYDEVKIT_READOBJ NAMES llvm-readobj REQUIRED)
    find_program(ANYDEVKIT_OBJCOPY NAMES objcopy HINTS "${CMAKE_SYSROOT}/bin" REQUIRED)
    set(CMAKE_CXX_STANDARD_LIBRARIES "" PARENT_SCOPE)
    target_compile_options(${target} PRIVATE -fdwarf-exceptions -fno-stack-protector -fno-stack-check -mno-stack-arg-probe)
    target_link_options(${target} PRIVATE -nostdlib -fuse-ld=lld -Wl,--entry,AnyDevKitEntry -Wl,--subsystem,windows)
    anydevkit_use_release_headers(${target})
    set(startupObject "${CMAKE_CURRENT_BINARY_DIR}/${target}-startup.obj")
    add_custom_command(OUTPUT "${startupObject}"
        COMMAND "$<TARGET_FILE:AnyDevKitWindowsStartupGenerator>" "${startupObject}"
        DEPENDS AnyDevKitWindowsStartupGenerator
        VERBATIM)
    set_source_files_properties("${startupObject}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
    target_sources(${target} PRIVATE "${startupObject}")
    target_link_libraries(${target} PRIVATE AnyDevKitWindowsStartup)
    add_dependencies(${target} AnyDevKitNidPatcher)
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "$<TARGET_FILE:AnyDevKitNidPatcher>")
    set(nativeExceptionSymbols __cxa_get_globals __cxa_allocate_exception __cxa_free_exception __cxa_throw __cxa_begin_catch __cxa_end_catch __cxa_rethrow __cxa_call_terminate __gxx_personality_v0 _Unwind_Resume _ZSt9terminatev)
    string(JOIN "\n" preservedExceptions ${nativeExceptionSymbols})
    set(preservedFile "${CMAKE_CURRENT_BINARY_DIR}/${target}-native-exceptions.txt")
    file(CONFIGURE OUTPUT "${preservedFile}" CONTENT "${preservedExceptions}\n" NEWLINE_STYLE UNIX)
    set(libcSymbols ${nativeExceptionSymbols} memcpy_nid_postfix memmove_nid_postfix memset_nid_postfix memcmp_nid_postfix memchr_nid_postfix strlen_nid_postfix ApplicationHeapInitialize_nid_no_patch malloc_nid_postfix free_nid_postfix exit_nid_postfix atexit_nid_postfix __cxa_atexit_nid_postfix "_ZTVN10__cxxabiv117__class_type_infoE DATA" ${ARGN})
    list(REMOVE_DUPLICATES libcSymbols)
    anydevkit_link_runtime_library(${target} libc ${libcSymbols})
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${ANYDEVKIT_OBJCOPY}" --rename-section .eh_fram=.ehfram --rename-section .rdata=.sdkdata,alloc,load,data,contents "$<TARGET_FILE:${target}>"
        COMMAND "$<TARGET_FILE:AnyDevKitNidPatcher>" --exec --preserve-export-list "${preservedFile}" "$<TARGET_FILE:${target}>"
        COMMAND "${CMAKE_COMMAND}" "-DREADOBJ=${ANYDEVKIT_READOBJ}" "-DEXECUTABLE=$<TARGET_FILE:${target}>" "-DRUNTIME_DIRECTORY=${ANYDEVKIT_RUNTIME_DIRECTORY}" "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}" "-DNID_PATCHER=$<TARGET_FILE:AnyDevKitNidPatcher>" -P "${sdkRoot}/cmake/VerifyRuntimeImports.cmake"
        VERBATIM)
endfunction()
