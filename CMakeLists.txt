cmake_minimum_required(VERSION 3.20)
project(astro_blitz C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Raylib via FetchContent ──────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)

# Disable raylib examples and extras
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

# ── Game executable ──────────────────────────────────────────────────────────
add_executable(astro_blitz src/main.c)
target_link_libraries(astro_blitz PRIVATE raylib)

# On Windows, link required system libraries
if(WIN32)
    target_link_libraries(astro_blitz PRIVATE winmm)
endif()
