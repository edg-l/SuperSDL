cmake_minimum_required(VERSION 3.13)

project(SuperSDL)

set(SUPERSDL_VERSION_MAJOR "0")
set(SUPERSDL_VERSION_MINOR "1")
set(SUPERSDL_VERSION_REVISION "0")


# Needed so that cmake uses our find modules.

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


find_package(SDL2 REQUIRED)

find_package(SDL2_net REQUIRED)

find_package(SDL2_mixer REQUIRED)

find_package(SDL2_image REQUIRED)

find_package(SDL2_gfx REQUIRED)

find_package(SDL2_ttf REQUIRED)


set(SOURCE_FILES
    src/main.cpp
    )

include_directories(src)

add_library(SuperSDL SHARED ${SOURCE_FILES})
target_link_libraries(SuperSDL SDL2::Main SDL2::Net SDL2::Mixer SDL2::Image SDL2::TTF SDL2::GFX)

set_target_properties(SuperSDL PROPERTIES PUBLIC_HEADER src/supersdl.hpp)

include(GNUInstallDirs)

install(TARGETS SuperSDL PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/supersdl)


