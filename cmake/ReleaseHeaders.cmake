include_guard(GLOBAL)

function(anydevkit_use_release_headers target)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    include(FetchContent)
    FetchContent_Declare(AnyDevKitPrxHeaders
        URL "https://github.com/boykopovar/AnyPS5/releases/download/v0.1.4/prx-headers-v0.1.4.zip"
        URL_HASH SHA256=e542b7b45810534bf52b077bdfd98608ea879fc732e922b337e72c41877249e3
    )
    FetchContent_MakeAvailable(AnyDevKitPrxHeaders)
    set(headers "${anydevkitprxheaders_SOURCE_DIR}/include/anyps5")
    if(NOT EXISTS "${headers}/prx/libc/libc.hpp")
        message(FATAL_ERROR "Missing AnyPS5 public release headers")
    endif()
    target_include_directories(${target} SYSTEM PRIVATE "${headers}")
endfunction()
