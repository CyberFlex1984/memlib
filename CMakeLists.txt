cmake_minimum_required(VERSION 4.3.4)
project(memlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EXAMPLES "Build example programs" ON)

file(GLOB_RECURSE SOURCES_CORE CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.c"
)

add_library(memlib STATIC ${SOURCES_CORE})

target_include_directories(memlib 
    PUBLIC
        include/
    PRIVATE
        src/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_EXAMPLES)
    add_executable(example_internal examples/example_internal.cpp)
    target_link_libraries(example_internal PRIVATE memlib)

    add_executable(example_external examples/example_external.cpp)
    target_link_libraries(example_external PRIVATE memlib)
endif()