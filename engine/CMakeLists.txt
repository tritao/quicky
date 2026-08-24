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

add_executable(quiky-player-frame apps/quiky-player-frame.cpp)
target_link_libraries(quiky-player-frame PRIVATE quiky_core)

option(QUIKY_ENABLE_FRONTEND "Build the SDL3 interactive frontend" ON)
if(QUIKY_ENABLE_FRONTEND)
    find_package(SDL3 CONFIG QUIET)
    if(SDL3_FOUND AND TARGET SDL3::SDL3)
        add_executable(quiky-play apps/quiky-play.cpp)
        target_link_libraries(quiky-play PRIVATE quiky_core SDL3::SDL3)
    else()
        message(STATUS "SDL3 not found; interactive frontend disabled. Set QUIKY_ENABLE_FRONTEND=OFF to silence this message.")
    endif()
endif()

enable_testing()

add_executable(quiky-tests tests/test_formats.cpp)
target_link_libraries(quiky-tests PRIVATE quiky_core)
add_test(NAME quiky-tests COMMAND quiky-tests)
