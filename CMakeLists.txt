#
#   qdalsabass: ALSA sequencer implementation for BASSMIDI.
#   Copyright (C) 2024  forteus19
#
#   See NOTICE.txt for the full notice.
#

cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)

project(qdalsabass)

file(GLOB_RECURSE SRCS src/*.cpp)
file(GLOB_RECURSE HDRS src/*.h)

add_executable(qdalsabass ${SRCS} ${HDRS})

file(GLOB IM_SRCS submodules/imgui/*.cpp)
file(GLOB IM_HDRS submodules/imgui/*.h)

add_library(
    imgui ${IM_SRCS} ${IM_HDRS}
    submodules/imgui/backends/imgui_impl_sdl2.cpp
    submodules/imgui/backends/imgui_impl_sdl2.h
    submodules/imgui/backends/imgui_impl_sdlrenderer2.cpp
    submodules/imgui/backends/imgui_impl_sdlrenderer2.h
)

find_package(SDL2 REQUIRED)

target_include_directories(imgui PRIVATE submodules/imgui)
target_include_directories(imgui PRIVATE ${SDL2_INCLUDE_DIRS})

file(GLOB PFD_SRCS submodules/pfd/*.cpp)
file(GLOB PFD_HDRS submodules/pfd/*.h)

add_library(pfd INTERFACE)

target_include_directories(pfd INTERFACE submodules/pfd)

add_library(nlohmann_json INTERFACE)

target_include_directories(nlohmann_json INTERFACE submodules/json/include)

target_include_directories(qdalsabass PRIVATE inc)
target_include_directories(qdalsabass PRIVATE submodules/imgui)
target_include_directories(qdalsabass PRIVATE submodules/imgui/backends)

file(GLOB_RECURSE LIBS lib/*.so)

target_link_libraries(qdalsabass PRIVATE asound)
target_link_libraries(qdalsabass PRIVATE SDL2)
target_link_libraries(qdalsabass PRIVATE imgui)
target_link_libraries(qdalsabass PRIVATE pfd)
target_link_libraries(qdalsabass PRIVATE nlohmann_json)
target_link_libraries(qdalsabass PRIVATE ${LIBS})
