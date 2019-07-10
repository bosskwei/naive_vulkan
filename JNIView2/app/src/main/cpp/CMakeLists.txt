cmake_minimum_required(VERSION 3.4.1)

add_library(native-lib SHARED
        native-lib.cpp
        vulkan_wrapper.cpp)


target_link_libraries(
        native-lib
        android
        jnigraphics
        log
)