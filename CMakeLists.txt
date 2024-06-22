cmake_minimum_required(VERSION 3.28)

project(qdalsabass)

file(GLOB_RECURSE SRCS src/*.cpp)
file(GLOB_RECURSE HDRS src/*.hpp)

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

target_include_directories(qdalsabass PRIVATE inc)
target_include_directories(qdalsabass PRIVATE submodules/imgui)
target_include_directories(qdalsabass PRIVATE submodules/imgui/backends)

file(GLOB_RECURSE LIBS lib/*.so)

target_link_libraries(qdalsabass PRIVATE asound)
target_link_libraries(qdalsabass PRIVATE SDL2)
target_link_libraries(qdalsabass PRIVATE imgui)
target_link_libraries(qdalsabass PRIVATE pfd)
target_link_libraries(qdalsabass PRIVATE ${LIBS})
