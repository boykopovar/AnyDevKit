set(ANYDEVKIT_NID_PATCHER "" CACHE FILEPATH "AnyPS5 nid_patcher executable")
set(ANYDEVKIT_VIDEOOUT_IMPORT_LIBRARY "" CACHE FILEPATH "Unpatched AnyPS5 VideoOut import library")
set(ANYDEVKIT_RUNTIME_DIRECTORY "" CACHE PATH "Directory containing patched AnyPS5 runtime libraries")

foreach(requiredPath IN ITEMS ANYDEVKIT_NID_PATCHER ANYDEVKIT_VIDEOOUT_IMPORT_LIBRARY)
    if(NOT EXISTS "${${requiredPath}}")
        message(FATAL_ERROR "${requiredPath} must name an existing file")
    endif()
endforeach()

if(NOT EXISTS "${ANYDEVKIT_RUNTIME_DIRECTORY}/libSceVideoOut.prx")
    message(FATAL_ERROR "ANYDEVKIT_RUNTIME_DIRECTORY must contain libSceVideoOut.prx")
endif()

add_executable(Calculator src/Main.cpp src/Canvas.cpp)
target_compile_features(Calculator PRIVATE cxx_std_20)
set_target_properties(Calculator PROPERTIES CXX_EXTENSIONS OFF)
set_property(TARGET Calculator APPEND PROPERTY LINK_DEPENDS "${ANYDEVKIT_NID_PATCHER}")
target_include_directories(Calculator PRIVATE include "${PROJECT_SOURCE_DIR}/3rdparty/AnyPS5/core/libs")
target_link_libraries(Calculator PRIVATE "${ANYDEVKIT_VIDEOOUT_IMPORT_LIBRARY}")
add_custom_command(TARGET Calculator POST_BUILD
    COMMAND "${ANYDEVKIT_NID_PATCHER}" --exec --only-sce "$<TARGET_FILE:Calculator>"
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
