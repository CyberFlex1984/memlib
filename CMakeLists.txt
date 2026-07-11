cmake_minimum_required(VERSION 3.20)
project(memlib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EBPF_BACKEND "Build the Linux-only eBPF backend" OFF)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_UEFI_DRIVER "Build the experimental UEFI driver" OFF)


file(GLOB_RECURSE SOURCES_CORE CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.c"
)

add_library(memlib STATIC ${SOURCES_CORE})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(memlib PRIVATE _GNU_SOURCE)
endif()

if(BUILD_UEFI_DRIVER)
    if(EDK2_SOURCE_DIR AND EXISTS ${EDK2_SOURCE_DIR}/ReadMe.rst)
        message(STATUS "Using EDK2 source: ${EDK2_SOURCE_DIR}")
    elseif(EXISTS ${CMAKE_SOURCE_DIR}/third-party/edk2/ReadMe.rst)
        message(STATUS "Using EDK2 submodule: ${CMAKE_SOURCE_DIR}/third-party/edk2")
        set(EDK2_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third-party/edk2 CACHE PATH "EDK2 source" FORCE)
    else()
        message(STATUS "EDK2 submodule not found, downloading via FetchContent...")
        include(FetchContent)

        FetchContent_Declare(
            edk2
            GIT_REPOSITORY https://github.com/tianocore/edk2.git
            GIT_TAG edk2-stable202605
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(edk2)
        set(EDK2_SOURCE_DIR ${edk2_SOURCE_DIR} CACHE PATH "EDK2 source" FORCE)
    endif()

    message(STATUS "EDK2_SOURCE_DIR: ${EDK2_SOURCE_DIR}")

    add_subdirectory(uefi)

    add_custom_target(build_uefi DEPENDS uefi_driver)
    add_dependencies(memlib build_uefi)
endif()


if(BUILD_EBPF_BACKEND AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
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

if(BUILD_EBPF_BACKEND AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(WARNING "BUILD_EBPF_BACKEND is only available when targeting Linux; skipping it")
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
    
    if(BUILD_EBPF_BACKEND AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(example_ebpf examples/example_ebpf.cpp)
        target_link_libraries(example_ebpf PRIVATE memlib)
    endif()
endif()
