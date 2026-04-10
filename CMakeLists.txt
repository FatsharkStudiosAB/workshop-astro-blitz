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

# ── Game logic objects (compiled once, linked into multiple targets) ──────────
add_library(astro_blitz_objs OBJECT
    src/vec2.c
    src/tilemap.c
    src/game.c
    src/player.c
    src/bullet.c
    src/enemy.c
    src/audio.c
    src/settings.c
    src/particle.c
    src/screenshake.c
    src/weapon.c
    src/upgrade.c
)
target_include_directories(astro_blitz_objs PUBLIC src)
# Objects need Raylib headers (types, raymath inlines) at compile time, but
# we deliberately do NOT link raylib here -- that happens in the consuming
# targets so integration tests can substitute stubs.
target_include_directories(astro_blitz_objs SYSTEM PUBLIC
    $<TARGET_PROPERTY:raylib,INTERFACE_INCLUDE_DIRECTORIES>
)
# Also pull in the raylib source include directory for raymath.h
target_include_directories(astro_blitz_objs SYSTEM PUBLIC
    ${raylib_SOURCE_DIR}/src
)

# ── Game logic library (objects + real Raylib, for exe and unit tests) ────────
add_library(astro_blitz_lib STATIC $<TARGET_OBJECTS:astro_blitz_objs>)
target_include_directories(astro_blitz_lib PUBLIC src)
target_link_libraries(astro_blitz_lib PUBLIC raylib)

# On non-MSVC platforms, link the math library for sqrtf etc.
if(NOT MSVC)
    target_link_libraries(astro_blitz_lib PUBLIC m)
endif()

# ── Game executable ──────────────────────────────────────────────────────────
add_executable(astro_blitz src/main.c src/postfx.c)
target_link_libraries(astro_blitz PRIVATE astro_blitz_lib)

# Note: winmm is NOT explicitly linked because Raylib's PlaySound symbol
# conflicts with winmm's PlaySound. Raylib handles its own Windows deps.

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

    # Settings module tests
    add_executable(test_settings tests/test_settings.c)
    target_link_libraries(test_settings PRIVATE unity astro_blitz_lib)
    add_test(NAME test_settings COMMAND test_settings)

    # Particle module tests
    add_executable(test_particle tests/test_particle.c)
    target_link_libraries(test_particle PRIVATE unity astro_blitz_lib)
    add_test(NAME test_particle COMMAND test_particle)

    # Screen shake module tests
    add_executable(test_screenshake tests/test_screenshake.c)
    target_link_libraries(test_screenshake PRIVATE unity astro_blitz_lib)
    add_test(NAME test_screenshake COMMAND test_screenshake)

    # Weapon module tests
    add_executable(test_weapon tests/test_weapon.c)
    target_link_libraries(test_weapon PRIVATE unity astro_blitz_lib)
    add_test(NAME test_weapon COMMAND test_weapon)

    # Upgrade module tests
    add_executable(test_upgrade tests/test_upgrade.c)
    target_link_libraries(test_upgrade PRIVATE unity astro_blitz_lib)
    add_test(NAME test_upgrade COMMAND test_upgrade)

    # ── Integration tests (game objects + Raylib stubs, no real Raylib) ───
    add_library(raylib_stubs STATIC tests/raylib_stubs.c)
    target_include_directories(raylib_stubs PUBLIC
        ${raylib_SOURCE_DIR}/src   # raylib.h, raymath.h type definitions
    )

    add_executable(test_integration tests/test_integration.c)
    target_include_directories(test_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src    # game headers (game.h, player.h, etc.)
        ${CMAKE_CURRENT_SOURCE_DIR}/tests  # raylib_stubs.h
        ${raylib_SOURCE_DIR}/src           # raylib.h, raymath.h (types only)
    )
    target_link_libraries(test_integration PRIVATE
        unity
        $<TARGET_OBJECTS:astro_blitz_objs>
        raylib_stubs
    )
    # On non-MSVC platforms, link the math library for sqrtf etc.
    if(NOT MSVC)
        target_link_libraries(test_integration PRIVATE m)
    endif()
    add_test(NAME test_integration COMMAND test_integration)
endif()
