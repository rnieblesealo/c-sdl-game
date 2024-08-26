cmake_minimum_required(VERSION 3.5)
project(game)

# expose includes to lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# compile with debug information
set(CMAKE_BUILD_TYPE Debug)

# use system sdl, not vendored one
find_package(SDL2 REQUIRED)
include_directories(SDL2Test ${SDL2_INCLUDE_DIRS})

add_executable(game
  src/main.c
)

# add the includes
target_include_directories(game PRIVATE include/)

# link sdl 
target_link_libraries(game ${SDL2_LIBRARIES})
