cmake_minimum_required(VERSION 3.20)
project(astro_blitz C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Raylib via FetchContent ──────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    raylib
    URL                    https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    URL_HASH               SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Disable raylib examples and extras (CACHE so raylib's option() doesn't override)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples")
set(BUILD_GAMES    OFF CACHE BOOL "Build raylib example games")

FetchContent_MakeAvailable(raylib)

# ── Game executable ──────────────────────────────────────────────────────────
add_executable(astro_blitz src/main.c)
target_link_libraries(astro_blitz PRIVATE raylib)

# On Windows, link required system libraries
if(WIN32)
    target_link_libraries(astro_blitz PRIVATE winmm)
endif()
