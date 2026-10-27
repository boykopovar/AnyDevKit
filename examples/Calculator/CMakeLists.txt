set(ANYDEVKIT_CALCULATOR_WIDTH 1920 CACHE STRING "Calculator framebuffer width")
if(NOT ANYDEVKIT_CALCULATOR_WIDTH MATCHES "^[0-9]+$")
    message(FATAL_ERROR "ANYDEVKIT_CALCULATOR_WIDTH must contain decimal digits")
endif()

add_executable(Calculator src/Main.cpp src/Canvas.cpp src/Calculator.cpp src/Interface.cpp src/Input.cpp src/Display.cpp)
target_compile_features(Calculator PRIVATE cxx_std_20)
target_compile_definitions(Calculator PRIVATE ANYDEVKIT_CALCULATOR_WIDTH=${ANYDEVKIT_CALCULATOR_WIDTH})
set_target_properties(Calculator PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(Calculator PRIVATE include)
anydevkit_configure_windows_executable(Calculator snprintf_nid_postfix strtod_nid_postfix)
anydevkit_link_runtime_library(Calculator libSceVideoOut sceVideoOutOpen sceVideoOutSetBufferAttribute2 sceVideoOutRegisterBuffers2 sceVideoOutSubmitFlip sceVideoOutWaitVblank sceVideoOutGetFlipStatus sceVideoOutGetBufferLabelAddress)
anydevkit_link_runtime_library(Calculator libkernel sceKernelAllocateDirectMemory sceKernelGetDirectMemorySize sceKernelMapDirectMemory sceKernelReleaseDirectMemory sceKernelDebugOutText sceKernelGetProcessTime sceKernelMapFlexibleMemory sceKernelMunmap)
anydevkit_link_runtime_library(Calculator libScePad scePadInit_nid_postfix scePadOpen_nid_postfix scePadReadState scePadSetVibration)
anydevkit_link_runtime_library(Calculator libSceSysmodule sceSysmoduleLoadModuleInternal)
anydevkit_link_runtime_library(Calculator libSceUserService sceUserServiceInitialize sceUserServiceGetInitialUser)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/app0/sce_sys/param.json" CONTENT [=[
{"titleId":"FAKE40001","localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"AnyDevKit Calculator"}}}
]=])

get_filename_component(compilerDirectory "${CMAKE_CXX_COMPILER}" DIRECTORY)
if(WIN32)
    set(runtimeEnvironment "set(ENV{PATH} \"${ANYDEVKIT_RUNTIME_DIRECTORY};${CMAKE_SYSROOT}/bin;${compilerDirectory};\$ENV{PATH}\")")
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
