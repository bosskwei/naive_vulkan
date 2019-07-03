cmake_minimum_required(VERSION 3.8)
project(untitled_1)

find_package(glfw3)
find_package(Vulkan)
find_package(Threads)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_COMPILER "clang-6.0")
set(CMAKE_CXX_COMPILER "clang++-6.0")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})
# include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(
        untitled_1
        main.cpp)

target_link_libraries(
        untitled_1
        -lglfw
        ${Vulkan_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
)