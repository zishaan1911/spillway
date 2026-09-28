cmake_minimum_required(VERSION 3.20)
project(spillway LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SPILLWAY_BUILD_GAME "Build the game executable (needs raylib)" ON)
option(SPILLWAY_BUILD_TESTS "Build the physics and game-logic tests" ON)

if(MSVC)
  set(SPILLWAY_WARNINGS /W4)
else()
  set(SPILLWAY_WARNINGS -Wall -Wextra -Wpedantic)
endif()

# Physics and game rules. No raylib in here, so the tests build anywhere.
add_library(spillway_core STATIC
  src/game/level.cpp
  src/physics/collider_grid.cpp
  src/physics/contact.cpp
  src/physics/fluid.cpp
  src/physics/particle_grid.cpp
  src/physics/rigid.cpp
  src/physics/world.cpp)
target_include_directories(spillway_core PUBLIC src)
target_compile_options(spillway_core PRIVATE ${SPILLWAY_WARNINGS})

if(SPILLWAY_BUILD_TESTS)
  enable_testing()
  add_executable(spillway_tests
    tests/main.cpp
    tests/test_collider_grid.cpp
    tests/test_contact.cpp
    tests/test_geometry.cpp
    tests/test_level.cpp
    tests/test_particle_grid.cpp
    tests/test_rigid.cpp
    tests/test_world.cpp)
  target_include_directories(spillway_tests PRIVATE tests)
  target_link_libraries(spillway_tests PRIVATE spillway_core)
  target_compile_options(spillway_tests PRIVATE ${SPILLWAY_WARNINGS})
  add_test(NAME spillway_tests COMMAND spillway_tests)
endif()

if(SPILLWAY_BUILD_GAME)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(raylib)

  add_executable(spillway src/app/main.cpp)
  target_link_libraries(spillway PRIVATE raylib)
endif()
