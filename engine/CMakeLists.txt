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
    src/map.cpp
    src/palette.cpp
    src/renderer.cpp
    src/runtime.cpp
    src/level.cpp
    src/level_runtime.cpp
    src/sfx_events.cpp
    src/tileset.cpp
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

add_executable(quiky-calibrate apps/quiky-calibrate.cpp)
target_link_libraries(quiky-calibrate PRIVATE quiky_core)

add_executable(quiky-compare-trace apps/quiky-compare-trace.cpp)
target_link_libraries(quiky-compare-trace PRIVATE quiky_core)

add_executable(quiky-player-frame apps/quiky-player-frame.cpp)
target_link_libraries(quiky-player-frame PRIVATE quiky_core)

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

add_executable(quiky-tests tests/test_formats.cpp)
target_link_libraries(quiky-tests PRIVATE quiky_core)
add_test(NAME quiky-tests COMMAND quiky-tests)

add_executable(quiky-sfx-event-tests tests/test_sfx_events.cpp)
target_link_libraries(quiky-sfx-event-tests PRIVATE quiky_core)
add_test(NAME quiky-sfx-event-tests COMMAND quiky-sfx-event-tests)

add_test(NAME quiky-calibration-generate
    COMMAND quiky-calibrate ${CMAKE_CURRENT_SOURCE_DIR}/../game/NESTLE.DAT
            W1L1.MAP 100 100 ${CMAKE_CURRENT_BINARY_DIR}/quiky-calibration.csv)
add_test(NAME quiky-calibration-compare
    COMMAND quiky-compare-trace
            ${CMAKE_CURRENT_BINARY_DIR}/quiky-calibration.csv
            ${CMAKE_CURRENT_BINARY_DIR}/quiky-calibration.csv --strict)
set_tests_properties(quiky-calibration-compare
    PROPERTIES DEPENDS quiky-calibration-generate)
