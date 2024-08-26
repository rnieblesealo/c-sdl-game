cmake_minimum_required(VERSION 3.5)
project(game)

# expose includes to lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# compile with debug information
set(CMAKE_BUILD_TYPE Debug)

add_executable(game
  src/main.c
)

# add my includes
INCLUDE_DIRECTORIES(game PRIVATE include/)

# add sdl includes and link libraries
INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2_image REQUIRED SDL2_image)

INCLUDE_DIRECTORIES(game
  ${SDL2_INCLUDE_DIRS}
  ${SDL2IMAGE_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(game
  ${SDL2_LIBRARIES}
  ${SDL2IMAGE_LIBRARIES}
)
