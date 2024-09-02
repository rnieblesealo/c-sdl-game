cmake_minimum_required(VERSION 3.30)
project(game)

# expose includes to lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# compile with debug information
set(CMAKE_BUILD_TYPE Debug)

add_executable(game
  src/main.cpp
)

# add my includes
INCLUDE_DIRECTORIES(game PRIVATE include/)

# add sdl includes and link libraries
INCLUDE(FindPkgConfig)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

INCLUDE_DIRECTORIES(game
  ${SDL2_INCLUDE_DIRS}
  ${SDL2IMAGE_INCLUDE_DIRS}
  ${SDL2TTF_INCLUDE_DIRS}
  ${SDL2MIXER_INCLUDE_DIRS}
)

# i don't know what this does
# but i do know i fucking hate cmake
TARGET_LINK_LIBRARIES(game
  SDL2::SDL2main
  SDL2::SDL2
  SDL2_image::SDL2_image
  SDL2_ttf::SDL2_ttf
  SDL2_mixer::SDL2_mixer
)
