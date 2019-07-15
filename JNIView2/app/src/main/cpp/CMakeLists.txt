cmake_minimum_required(VERSION 3.4.1)


set(CMAKE_BUILD_TYPE Debug)


include_directories(${PROJECT_SOURCE_DIR})

add_library(native-lib SHARED
        native-lib.cpp
        vulkan_wrapper.cpp)


target_link_libraries(
        native-lib
        android
        jnigraphics
        log
)