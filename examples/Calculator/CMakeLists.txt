set(ANYDEVKIT_LIBCXX_BUILD "" CACHE PATH "Configured AnyLibcxx build directory")
if(NOT EXISTS "${ANYDEVKIT_LIBCXX_BUILD}/AnyLibcxxConfig.cmake")
    message(FATAL_ERROR "Missing AnyLibcxx build configuration")
endif()
include("${ANYDEVKIT_LIBCXX_BUILD}/AnyLibcxxConfig.cmake")

set(ANYDEVKIT_CALCULATOR_WIDTH 1920 CACHE STRING "Calculator framebuffer width")
if(NOT ANYDEVKIT_CALCULATOR_WIDTH MATCHES "^[0-9]+$")
    message(FATAL_ERROR "ANYDEVKIT_CALCULATOR_WIDTH must contain decimal digits")
endif()

add_subdirectory(ShaderBuilder)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" calculatorShaderTarget)
set(ANYDEVKIT_CALCULATOR_SHADER_TARGET "${calculatorShaderTarget}" CACHE STRING "Calculator shader target: windows, linux or ps5")
set(shaderHeader "${CMAKE_CURRENT_BINARY_DIR}/generated/CalculatorShaders.hpp")
add_custom_command(OUTPUT "${shaderHeader}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "$<TARGET_FILE:CalculatorShaderBuilder>" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Background.vert" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Background.frag" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Interface.vert" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Interface.frag" "${shaderHeader}" --target "${ANYDEVKIT_CALCULATOR_SHADER_TARGET}"
    DEPENDS CalculatorShaderBuilder shaders/Background.vert shaders/Background.frag shaders/Interface.vert shaders/Interface.frag
    VERBATIM)
add_executable(Calculator src/Main.cpp src/Canvas.cpp src/Calculator.cpp src/Interface.cpp src/Input.cpp src/Display.cpp src/Settings.cpp src/Graphics/Renderer.cpp "${shaderHeader}")
target_compile_features(Calculator PRIVATE cxx_std_20)
target_compile_definitions(Calculator PRIVATE ANYDEVKIT_CALCULATOR_WIDTH=${ANYDEVKIT_CALCULATOR_WIDTH})
set_target_properties(Calculator PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(Calculator PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(Calculator PRIVATE AnyLibcxx::Runtime)
anydevkit_configure_windows_executable(Calculator ${ANYLIBCXX_WINDOWS_IMPORTS} __cxa_get_globals)
anydevkit_link_runtime_library(Calculator libSceVideoOut sceVideoOutOpen sceVideoOutSetBufferAttribute2 sceVideoOutRegisterBuffers2 sceVideoOutSubmitFlip sceVideoOutWaitVblank sceVideoOutGetFlipStatus sceVideoOutGetBufferLabelAddress)
anydevkit_link_runtime_library(Calculator libkernel sceKernelAllocateDirectMemory sceKernelGetDirectMemorySize sceKernelMapDirectMemory sceKernelReleaseDirectMemory sceKernelDebugOutText sceKernelGetProcessTime sceKernelMapFlexibleMemory sceKernelMunmap sceKernelUsleep_nid_postfix scePthreadCreate scePthreadJoin sceKernelOpen sceKernelClose sceKernelRead sceKernelWrite sceKernelFsync sceKernelRename)
anydevkit_link_runtime_library(Calculator libScePad scePadInit_nid_postfix scePadOpen_nid_postfix scePadReadState scePadSetVibration scePadSetVibrationMode)
anydevkit_link_runtime_library(Calculator libSceSysmodule sceSysmoduleLoadModuleInternal)
anydevkit_link_runtime_library(Calculator libSceSaveData.native sceSaveDataInitialize3 sceSaveDataTerminate sceSaveDataCreateTransactionResource sceSaveDataDeleteTransactionResource sceSaveDataMount3 sceSaveDataPrepare sceSaveDataCommit sceSaveDataUmount2 sceSaveDataSetParam)
anydevkit_link_runtime_library(Calculator libSceUserService sceUserServiceInitialize sceUserServiceGetInitialUser)
anydevkit_link_runtime_library(Calculator libSceAgc sceAgcSuspendPoint sceAgcInit sceAgcCreateShader sceAgcGetRegisterDefaults sceAgcLinkShaders sceAgcDcbContextStateOp sceAgcDcbAcquireMem sceAgcDcbSetCxRegistersIndirect sceAgcDcbSetUcRegistersIndirect sceAgcDcbSetShRegistersIndirect sceAgcDcbSetShRegisterDirect sceAgcDcbSetNumInstances sceAgcDcbDrawIndexAuto sceAgcCbReleaseMem)
anydevkit_link_runtime_library(Calculator libSceAgcDriver sceAgcDriverSubmitDcb)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/app0/sce_sys/param.json" CONTENT [=[
{"titleId":"FAKE26109","localizedParameters":{"defaultLanguage":"en-US","en-US":{"titleName":"AnyDevKit Calculator"}}}
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
