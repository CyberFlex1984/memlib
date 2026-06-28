cmake_minimum_required(VERSION 4.3.4)
project(memlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES_CORE CONFIGURE_DEPENDS
    "src/core/*.cpp"
    "src/core/*.c"
)

add_library(memlib STATIC ${SOURCES_CORE})

target_include_directories(memlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)