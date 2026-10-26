set(ANYDEVKIT_NID_PATCHER "" CACHE FILEPATH "AnyPS5 nid_patcher executable")
set(ANYDEVKIT_VIDEOOUT_IMPORT_LIBRARY "" CACHE FILEPATH "Unpatched AnyPS5 VideoOut import library")

foreach(requiredPath IN ITEMS ANYDEVKIT_NID_PATCHER ANYDEVKIT_VIDEOOUT_IMPORT_LIBRARY)
    if(NOT EXISTS "${${requiredPath}}")
        message(FATAL_ERROR "${requiredPath} must name an existing file")
    endif()
endforeach()

add_executable(Calculator src/Main.cpp src/Canvas.cpp)
target_compile_features(Calculator PRIVATE cxx_std_20)
set_target_properties(Calculator PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(Calculator PRIVATE include "${PROJECT_SOURCE_DIR}/3rdparty/AnyPS5/core/libs")
target_link_libraries(Calculator PRIVATE "${ANYDEVKIT_VIDEOOUT_IMPORT_LIBRARY}")
add_custom_command(TARGET Calculator POST_BUILD
    COMMAND "${ANYDEVKIT_NID_PATCHER}" --exec --only-sce "$<TARGET_FILE:Calculator>"
    VERBATIM)
