cmake_minimum_required(VERSION 3.16)
project(quiky_object_behavior_model LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(quiky_object_behavior_model object_behavior_model.cpp)
target_include_directories(quiky_object_behavior_model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(quiky_object_behavior_model PRIVATE
    -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(quiky_object_behavior_model_tests object_behavior_model_test.cpp)
target_link_libraries(quiky_object_behavior_model_tests PRIVATE
    quiky_object_behavior_model)
target_compile_options(quiky_object_behavior_model_tests PRIVATE
    -Wall -Wextra -Wpedantic)
add_test(NAME quiky_object_behavior_model_tests
         COMMAND quiky_object_behavior_model_tests)
