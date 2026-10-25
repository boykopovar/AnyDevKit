find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import mako, yaml, lxml, rnc2rng" RESULT_VARIABLE makoResult)
if(NOT makoResult EQUAL 0)
    message(FATAL_ERROR "PSBC Python dependencies are missing. Install cmake/dependencies/requirements.txt into the selected Python environment.")
endif()

set(psbcRoot "${PROJECT_SOURCE_DIR}/3rdparty/opengnm-psbc")
set(psbcGenerated "${CMAKE_CURRENT_BINARY_DIR}/psbc-generated")
set(psbcGeneratedFiles
    src/amd/common/amdgfxregs.h
    src/amd/common/sid_tables.h
    src/amd/compiler/aco_builder.h
    src/amd/compiler/aco_opcodes.cpp
    src/amd/compiler/aco_opcodes.h
    src/compiler/nir/nir_builder_opcodes.h
    src/compiler/nir/nir_constant_expressions.c
    src/compiler/nir/nir_intrinsics.c
    src/compiler/nir/nir_intrinsics.h
    src/compiler/nir/nir_intrinsics_indices.h
    src/compiler/nir/nir_opcodes.c
    src/compiler/nir/nir_opcodes.h
    src/compiler/nir/nir_opt_algebraic.c
    src/compiler/spirv/spirv_info.c
    src/compiler/spirv/spirv_info.h
    src/compiler/spirv/vtn_gather_types.c
    src/compiler/spirv/vtn_generator_ids.h
    src/util/format/u_format_gen.h
    src/util/format/u_format_pack.h
    src/util/format/u_format_table.c
    src/amd/common/amd_cp_packets_gfx11.h
    src/amd/common/amd_cp_packets_gfx12.h
    src/compiler/builtin_types.h
    src/compiler/builtin_types.c
    src/util/format_srgb.c
    src/util/shader_stats.h
    src/vulkan/util/vk_struct_type_cast.h
    src/amd/common/gfx10_format_table.c
)
list(TRANSFORM psbcGeneratedFiles PREPEND "${psbcGenerated}/")
file(GLOB_RECURSE psbcGeneratorInputs CONFIGURE_DEPENDS "${psbcRoot}/src/*.py" "${psbcRoot}/src/*.json" "${psbcRoot}/src/*.yaml" "${psbcRoot}/src/*.xml" "${psbcRoot}/src/*.rnc" "${psbcRoot}/src/amd/packets/*.h")
add_custom_command(OUTPUT ${psbcGeneratedFiles}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/generate_psbc.py" "${psbcRoot}" "${psbcGenerated}" "${PROJECT_SOURCE_DIR}/3rdparty/SPIRV-Headers" "${PROJECT_SOURCE_DIR}/3rdparty/Vulkan-Headers"
    DEPENDS ${psbcGeneratorInputs} "${psbcRoot}/Makefile" "${CMAKE_CURRENT_LIST_DIR}/generate_psbc.py" "${PROJECT_SOURCE_DIR}/3rdparty/SPIRV-Headers/include/spirv/spir-v.xml" "${PROJECT_SOURCE_DIR}/3rdparty/Vulkan-Headers/registry/vk.xml"
    VERBATIM
)
add_custom_target(AnyDevKitPsbcGenerated DEPENDS ${psbcGeneratedFiles})

file(GLOB psbcSources CONFIGURE_DEPENDS
    "${psbcRoot}/src/compiler/nir/*.c"
    "${psbcRoot}/src/compiler/*.c"
    "${psbcRoot}/src/compiler/spirv/*.c"
    "${psbcRoot}/src/amd/compiler/*.cpp"
    "${psbcRoot}/src/amd/compiler/instruction_selection/*.cpp"
    "${psbcRoot}/src/amd/common/*.c"
    "${psbcRoot}/src/amd/common/nir/*.c"
    "${psbcRoot}/src/amd/vulkan/nir/*.c"
    "${psbcRoot}/src/util/*.c"
    "${psbcRoot}/src/util/*.cpp"
    "${psbcRoot}/src/util/format/*.c"
)
foreach(excluded IN ITEMS
    src/compiler/nir/nir_stub.c
    src/compiler/spirv/spirv2nir.c src/compiler/spirv/vtn_bindgen2.c
    src/amd/common/ac_linux_drm.c src/amd/common/ac_rtld.c src/amd/common/ac_rgp_elf_object_pack.c
    src/amd/common/ac_gpu_info.c src/amd/common/ac_video.c src/amd/common/ac_surface.c
    src/amd/common/ac_surface_meta_address_test.c src/amd/common/ac_surface_modifier_test.c
    src/amd/common/ac_vcn_dec.c src/amd/common/amdgpu_devices.c src/amd/common/ac_ib_parser.c
    src/amd/vulkan/nir/radv_nir_lower_ray_queries.c src/amd/vulkan/nir/radv_nir_rt_common.c
    src/amd/vulkan/nir/radv_nir_rt_stage_common.c src/amd/vulkan/nir/radv_nir_rt_stage_cps.c
    src/amd/vulkan/nir/radv_nir_rt_stage_functions.c src/amd/vulkan/nir/radv_nir_rt_stage_monolithic.c
    src/amd/vulkan/nir/radv_nir_rt_traversal_shader.c
    src/util/cache_ops_aarch64.c src/util/cache_ops_null.c src/util/cache_ops_x86.c
    src/util/cache_ops_x86_clflushopt.c src/util/u_debug_stack.c src/util/u_sync_provider.c
    src/util/os_socket.c src/util/xmlconfig.c
)
    list(REMOVE_ITEM psbcSources "${psbcRoot}/${excluded}")
endforeach()
add_library(AnyDevKitPsbc STATIC ${psbcSources} ${psbcGeneratedFiles}
    "${psbcRoot}/libpsbc/psbc_compile.c"
    "${psbcRoot}/cmd/psbc/crc32_sb.c"
    "${psbcRoot}/psbc_stubs.c"
    "${psbcRoot}/src/amd/vulkan/radv_postprocess_nir_standalone.c"
    "${psbcRoot}/src/amd/vulkan/radv_shader.c"
    "${psbcRoot}/src/amd/vulkan/radv_shader_args.c"
    "${psbcRoot}/src/amd/vulkan/radv_shader_info.c"
    "${psbcRoot}/src/util/blake3/blake3.c"
    "${psbcRoot}/src/util/blake3/blake3_dispatch.c"
    "${psbcRoot}/src/util/blake3/blake3_portable.c"
    "${psbcRoot}/src/c11/impl/time.c"
    "${psbcRoot}/src/vulkan/runtime/vk_nir_lower_descriptor_heaps.c"
    "${psbcRoot}/src/vulkan/runtime/vk_nir_convert_ycbcr.c"
)
add_dependencies(AnyDevKitPsbc AnyDevKitPsbcGenerated)
set_target_properties(AnyDevKitPsbc PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS ON CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
target_include_directories(AnyDevKitPsbc PUBLIC "${psbcRoot}")
foreach(includeDirectory IN ITEMS src src/amd src/amd/common src/amd/compiler src/compiler src/compiler/nir src/compiler/spirv src/util src/util/format src/vulkan/util)
    target_include_directories(AnyDevKitPsbc PRIVATE "${psbcGenerated}/${includeDirectory}")
endforeach()
target_include_directories(AnyDevKitPsbc PRIVATE
    "${PROJECT_SOURCE_DIR}/3rdparty/opengnm/include"
    "${PROJECT_SOURCE_DIR}/3rdparty/Vulkan-Headers/include"
    "${PROJECT_SOURCE_DIR}/3rdparty/SPIRV-Headers/include/spirv/unified1"
)
foreach(includeDirectory IN ITEMS include libpsbc src src/amd src/amd/packets src/amd/common src/amd/common/nir src/amd/compiler src/amd/vulkan src/amd/vulkan/nir src/vulkan/runtime src/vulkan/runtime/bvh src/vulkan/util src/compiler src/compiler/nir src/compiler/spirv src/gallium/include src/mesa src/mesa/main src/util src/util/format cmd/psbc include/mesa)
    target_include_directories(AnyDevKitPsbc PRIVATE "${psbcRoot}/${includeDirectory}")
endforeach()
if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    message(FATAL_ERROR "PSBC requires a GCC-compatible C compiler with packed enum support.")
endif()
target_compile_definitions(AnyDevKitPsbc PRIVATE UTIL_ARCH_LITTLE_ENDIAN=1 UTIL_ARCH_BIG_ENDIAN=0 HAVE_STRUCT_TIMESPEC=1 HAVE_FUNC_ATTRIBUTE_PACKED=1 BLAKE3_NO_SSE2 BLAKE3_NO_SSE41 BLAKE3_NO_AVX2 BLAKE3_NO_AVX512)
find_package(Threads REQUIRED)
target_link_libraries(AnyDevKitPsbc PRIVATE Threads::Threads)
if(WIN32)
    target_sources(AnyDevKitPsbc PRIVATE "${psbcRoot}/src/c11/impl/threads_win32.c")
    target_compile_definitions(AnyDevKitPsbc PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_compile_options(AnyDevKitPsbc PRIVATE -mno-ms-bitfields)
    target_link_libraries(AnyDevKitPsbc PRIVATE synchronization)
else()
    target_sources(AnyDevKitPsbc PRIVATE "${psbcRoot}/src/c11/impl/threads_posix.c")
    target_compile_definitions(AnyDevKitPsbc PRIVATE _GNU_SOURCE HAVE_PTHREAD=1 HAVE_SYSCONF=1)
    target_link_libraries(AnyDevKitPsbc PRIVATE m ${CMAKE_DL_LIBS})
endif()
