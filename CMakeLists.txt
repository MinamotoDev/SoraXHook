cmake_minimum_required(VERSION 3.28)
project(SoraXHook LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(FetchContent)
set(LSPLANT_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(LSPLANT_STANDALONE OFF CACHE BOOL "" FORCE)
set(DOBBY_DEBUG OFF CACHE BOOL "" FORCE)
set(DOBBY_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(DOBBY_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(Plugin.SymbolResolver OFF CACHE BOOL "" FORCE)
FetchContent_Declare(lsplant
    GIT_REPOSITORY https://github.com/LSPosed/LSPlant.git
    GIT_TAG d8b5d1dbb664abc606644036822e4bb64547edf6
    GIT_SUBMODULES lsplant/src/main/jni/external/dex_builder
    GIT_SUBMODULES_RECURSE TRUE
    SOURCE_SUBDIR lsplant/src/main/jni)
FetchContent_Declare(dobby
    GIT_REPOSITORY https://github.com/jmpews/Dobby.git
    GIT_TAG 21f07a5ce184cda9346afb8009c07fe7e47390d0)
FetchContent_MakeAvailable(lsplant dobby)
FetchContent_Declare(xz_embedded
    GIT_REPOSITORY https://github.com/tukaani-project/xz-embedded.git
    GIT_TAG ae63ae3a36ed01724674e8f3d750dc47bf125410)
FetchContent_MakeAvailable(xz_embedded)
add_library(soraxhook_xz STATIC
    "${xz_embedded_SOURCE_DIR}/linux/lib/xz/xz_crc32.c"
    "${xz_embedded_SOURCE_DIR}/linux/lib/xz/xz_crc64.c"
    "${xz_embedded_SOURCE_DIR}/linux/lib/xz/xz_sha256.c"
    "${xz_embedded_SOURCE_DIR}/linux/lib/xz/xz_dec_stream.c"
    "${xz_embedded_SOURCE_DIR}/linux/lib/xz/xz_dec_lzma2.c")
target_include_directories(soraxhook_xz PUBLIC "${xz_embedded_SOURCE_DIR}/linux/include/linux"
    PRIVATE "${xz_embedded_SOURCE_DIR}/userspace")
target_compile_definitions(soraxhook_xz PUBLIC XZ_USE_CRC64 XZ_USE_SHA256)
if(NOT EXISTS "${SORAXHOOK_GENERATED_DIR}/hook_dex.hpp")
    message(FATAL_ERROR "Run scripts/build.py first to generate hook_dex.hpp")
endif()
add_library(soraxhook SHARED native/module.cpp native/art_symbols.cpp native/debug_data.cpp)
target_include_directories(soraxhook PRIVATE native third_party "${SORAXHOOK_GENERATED_DIR}" "${dobby_SOURCE_DIR}/include")
target_link_libraries(soraxhook PRIVATE lsplant_static dobby_static soraxhook_xz log dl)
target_compile_options(soraxhook PRIVATE -Wall -Wextra -Werror -fvisibility=hidden)
target_link_options(soraxhook PRIVATE -Wl,--exclude-libs,ALL -Wl,-z,max-page-size=16384)
