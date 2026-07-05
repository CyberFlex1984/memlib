cmake_minimum_required(VERSION 4.2.3)
project(memlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EBPF_BACKEND "Build eBPF backend" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

file(GLOB_RECURSE SOURCES_CORE CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.c"
)

add_library(memlib STATIC ${SOURCES_CORE})

if(BUILD_EBPF_BACKEND AND CMAKE_HOST_LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBBPF REQUIRED libbpf)

    if(LIBBPF_FOUND)
        target_include_directories(memlib PRIVATE ${LIBBPF_INCLUDE_DIRS})
        target_link_libraries(memlib PRIVATE ${LIBBPF_LIBRARIES})
        target_compile_options(memlib PRIVATE ${LIBBPF_CFLAGS_OTHER})

        add_subdirectory(ebpf)

        target_compile_definitions(memlib PRIVATE MEMLIB_EBPF)
        add_dependencies(memlib ebpf_copy)
    endif()
endif()



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
    
    if(BUILD_EBPF_BACKEND AND CMAKE_HOST_LINUX)
        add_executable(example_ebpf examples/example_ebpf.cpp)
        target_link_libraries(example_ebpf PRIVATE memlib)
    endif()
endif()