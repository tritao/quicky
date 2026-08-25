cmake_minimum_required(VERSION 3.13)

project(quiky-engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(quiky_core
    src/area.cpp
    src/archive.cpp
    src/bob.cpp
    src/binary_reader.cpp
    src/collision_kernel.cpp
    src/fixed.cpp
    src/map.cpp
    src/player_record.cpp
    src/player_update.cpp
    src/palette.cpp
    src/pcx.cpp
    src/player_animation.cpp
    src/renderer.cpp
    src/input.cpp
    src/descriptors.cpp
    src/scheduler.cpp
    src/simulation.cpp
    src/level.cpp
    src/level_runtime.cpp
    src/sfx_events.cpp
    src/tileset.cpp
    src/trace.cpp
    src/world_view.cpp
)

target_include_directories(quiky_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(NOT MSVC)
    target_compile_options(quiky_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(quiky-inspect apps/quiky-inspect.cpp)
target_link_libraries(quiky-inspect PRIVATE quiky_core)

add_executable(quiky-render apps/quiky-render.cpp)
target_link_libraries(quiky-render PRIVATE quiky_core)

add_executable(quiky-bob-sheet apps/quiky-bob-sheet.cpp)
target_link_libraries(quiky-bob-sheet PRIVATE quiky_core)

add_executable(quiky-simulate apps/quiky-simulate.cpp)
target_link_libraries(quiky-simulate PRIVATE quiky_core)

add_executable(quiky-trace-compare apps/quiky-trace-compare.cpp)
target_link_libraries(quiky-trace-compare PRIVATE quiky_core)

add_executable(quiky-player-frame apps/quiky-player-frame.cpp)
target_link_libraries(quiky-player-frame PRIVATE quiky_core)

add_executable(quiky-player-trace apps/quiky-player-trace.cpp)
target_link_libraries(quiky-player-trace PRIVATE quiky_core)

add_executable(quiky-frame apps/quiky-frame.cpp)
target_link_libraries(quiky-frame PRIVATE quiky_core)

enable_testing()

option(QUIKY_ENABLE_MUSIC "Build the TFMX music player" ON)

if(QUIKY_ENABLE_MUSIC)
    add_library(quiky_music
        src/audio.cpp
        src/music.cpp
    )
    target_link_libraries(quiky_music PUBLIC quiky_core)
    if(NOT MSVC)
        target_compile_options(quiky_music PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(quiky-music apps/quiky-music.cpp)
    target_link_libraries(quiky-music PRIVATE quiky_music)

    add_executable(quiky-sfx apps/quiky-sfx.cpp)
    target_link_libraries(quiky-sfx PRIVATE quiky_music)

    add_executable(quiky-sfx-tests tests/test_sfx.cpp)
    target_link_libraries(quiky-sfx-tests PRIVATE quiky_music)

    find_package(SDL3 CONFIG QUIET)
    if(SDL3_FOUND AND TARGET SDL3::SDL3)
        target_link_libraries(quiky-music PRIVATE SDL3::SDL3)
        target_compile_definitions(quiky-music PRIVATE QUIKY_WITH_SDL=1)
        target_link_libraries(quiky-sfx PRIVATE SDL3::SDL3)
        target_compile_definitions(quiky-sfx PRIVATE QUIKY_WITH_SDL=1)
    endif()
endif()

option(QUIKY_ENABLE_FRONTEND "Build the SDL3 interactive frontend" ON)
if(QUIKY_ENABLE_FRONTEND)
    find_package(SDL3 CONFIG QUIET)
    if(SDL3_FOUND AND TARGET SDL3::SDL3)
        add_executable(quiky-play apps/quiky-play.cpp)
        target_link_libraries(quiky-play PRIVATE quiky_core SDL3::SDL3)
        if(QUIKY_ENABLE_MUSIC)
            target_link_libraries(quiky-play PRIVATE quiky_music)
            target_compile_definitions(quiky-play PRIVATE QUIKY_WITH_MUSIC=1)
        endif()
    else()
        message(STATUS "SDL3 not found; interactive frontend disabled. Set QUIKY_ENABLE_FRONTEND=OFF to silence this message.")
    endif()
endif()

if(QUIKY_ENABLE_MUSIC)
    add_test(NAME quiky-music-list
        COMMAND quiky-music ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT list)
    add_test(NAME quiky-music-info-title
        COMMAND quiky-music ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT info TITEL)
    add_test(NAME quiky-music-info-all
        COMMAND quiky-music ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT info ONGAME2)
    add_test(NAME quiky-music-render-title
        COMMAND quiky-music ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT
                render TITEL ${CMAKE_CURRENT_BINARY_DIR}/quiky-music-title.wav)
    add_test(NAME quiky-sfx-info-gameplay
        COMMAND quiky-sfx ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT info ONGAME2 13)
    add_test(NAME quiky-sfx-tests
        COMMAND quiky-sfx-tests ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT)
    add_test(NAME quiky-sfx-render-gameplay
        COMMAND quiky-sfx ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT
                render ONGAME2 0 ${CMAKE_CURRENT_BINARY_DIR}/quiky-sfx-0.wav 1)
endif()

add_executable(quiky-tests tests/test_formats_faithful.cpp)
target_link_libraries(quiky-tests PRIVATE quiky_core)
add_test(NAME quiky-tests COMMAND quiky-tests)

add_executable(quiky-foundation-tests tests/test_foundation.cpp)
target_link_libraries(quiky-foundation-tests PRIVATE quiky_core)
add_test(NAME quiky-foundation-tests COMMAND quiky-foundation-tests)

add_executable(quiky-horizontal-tests tests/test_horizontal.cpp)
target_link_libraries(quiky-horizontal-tests PRIVATE quiky_core)
add_test(NAME quiky-horizontal-tests
    COMMAND quiky-horizontal-tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player-horizontal-v1.tsv)

add_executable(quiky-vertical-tests tests/test_vertical.cpp)
target_link_libraries(quiky-vertical-tests PRIVATE quiky_core)
add_test(NAME quiky-vertical-tests
    COMMAND quiky-vertical-tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player-vertical-v1.tsv)

add_executable(quiky-player-callback-tests tests/test_player_callback.cpp)
target_link_libraries(quiky-player-callback-tests PRIVATE quiky_core)
add_test(NAME quiky-player-callback-tests COMMAND quiky-player-callback-tests)

add_executable(quiky-sfx-event-tests tests/test_sfx_events.cpp)
target_link_libraries(quiky-sfx-event-tests PRIVATE quiky_core)
add_test(NAME quiky-sfx-event-tests COMMAND quiky-sfx-event-tests)

add_executable(quiky-transition-reload-tests tests/test_transition_reload.cpp)
target_link_libraries(quiky-transition-reload-tests PRIVATE quiky_core)
add_test(NAME quiky-transition-reload-tests
    COMMAND quiky-transition-reload-tests
            ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT)
