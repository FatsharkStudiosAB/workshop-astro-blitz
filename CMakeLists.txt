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

# ── Game logic library (shared between exe and tests) ────────────────────────
add_library(astro_blitz_lib STATIC
    src/vec2.c
    src/tilemap.c
    src/game.c
    src/player.c
    src/bullet.c
    src/enemy.c
    src/audio.c
)
target_include_directories(astro_blitz_lib PUBLIC src)
target_link_libraries(astro_blitz_lib PUBLIC raylib)

# On non-MSVC platforms, link the math library for sqrtf etc.
if(NOT MSVC)
    target_link_libraries(astro_blitz_lib PUBLIC m)
endif()

# ── Game executable ──────────────────────────────────────────────────────────
add_executable(astro_blitz src/main.c)
target_link_libraries(astro_blitz PRIVATE astro_blitz_lib)

# On Windows, link required system libraries
if(WIN32)
    target_link_libraries(astro_blitz PRIVATE winmm)
endif()

# ── Testing (Unity + CTest) ─────────────────────────────────────────────────
include(CTest)

if(BUILD_TESTING)
    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(unity)

    # Sample test to verify the framework works
    add_executable(test_sample tests/test_sample.c)
    target_link_libraries(test_sample PRIVATE unity)
    add_test(NAME test_sample COMMAND test_sample)

    # Vec2 math tests
    add_executable(test_vec2 tests/test_vec2.c)
    target_link_libraries(test_vec2 PRIVATE unity astro_blitz_lib)
    add_test(NAME test_vec2 COMMAND test_vec2)

    # Player module tests
    add_executable(test_player tests/test_player.c)
    target_link_libraries(test_player PRIVATE unity astro_blitz_lib)
    add_test(NAME test_player COMMAND test_player)

    # Bullet module tests
    add_executable(test_bullet tests/test_bullet.c)
    target_link_libraries(test_bullet PRIVATE unity astro_blitz_lib)
    add_test(NAME test_bullet COMMAND test_bullet)

    # Game module tests
    add_executable(test_game tests/test_game.c)
    target_link_libraries(test_game PRIVATE unity astro_blitz_lib)
    add_test(NAME test_game COMMAND test_game)

    # Enemy module tests
    add_executable(test_enemy tests/test_enemy.c)
    target_link_libraries(test_enemy PRIVATE unity astro_blitz_lib)
    add_test(NAME test_enemy COMMAND test_enemy)

    # Tilemap module tests
    add_executable(test_tilemap tests/test_tilemap.c)
    target_link_libraries(test_tilemap PRIVATE unity astro_blitz_lib)
    add_test(NAME test_tilemap COMMAND test_tilemap)

    # Audio module tests
    add_executable(test_audio tests/test_audio.c)
    target_link_libraries(test_audio PRIVATE unity astro_blitz_lib)
    add_test(NAME test_audio COMMAND test_audio)
endif()
