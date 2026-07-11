cmake_minimum_required(VERSION 3.20)

find_program(CLANG clang REQUIRED)
message(STATUS "Using clang for eBPF: ${CLANG}")

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" EBPF_SYSTEM_PROCESSOR)
if(EBPF_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(EBPF_TARGET_ARCH x86)
elseif(EBPF_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(EBPF_TARGET_ARCH arm64)
elseif(EBPF_SYSTEM_PROCESSOR MATCHES "^arm")
    set(EBPF_TARGET_ARCH arm)
elseif(EBPF_SYSTEM_PROCESSOR MATCHES "^(ppc64|ppc64le|powerpc64)$")
    set(EBPF_TARGET_ARCH powerpc)
elseif(EBPF_SYSTEM_PROCESSOR MATCHES "^s390")
    set(EBPF_TARGET_ARCH s390)
elseif(EBPF_SYSTEM_PROCESSOR MATCHES "^riscv64$")
    set(EBPF_TARGET_ARCH riscv)
elseif(EBPF_SYSTEM_PROCESSOR MATCHES "^loongarch64$")
    set(EBPF_TARGET_ARCH loongarch)
else()
    message(FATAL_ERROR
        "Unsupported eBPF target processor '${CMAKE_SYSTEM_PROCESSOR}'. "
        "Set CMAKE_SYSTEM_PROCESSOR to a libbpf-supported architecture.")
endif()
message(STATUS "Using libbpf target architecture: ${EBPF_TARGET_ARCH}")

set(EBPF_INCLUDE_FLAGS "")
foreach(INCLUDE_DIR IN LISTS LIBBPF_INCLUDE_DIRS)
    list(APPEND EBPF_INCLUDE_FLAGS "-I${INCLUDE_DIR}")
endforeach()

if(CMAKE_SYSROOT)
    set(EBPF_SYSTEM_INCLUDE_ROOT "${CMAKE_SYSROOT}/usr/include")
else()
    set(EBPF_SYSTEM_INCLUDE_ROOT "/usr/include")
endif()
if(CMAKE_LIBRARY_ARCHITECTURE AND
   EXISTS "${EBPF_SYSTEM_INCLUDE_ROOT}/${CMAKE_LIBRARY_ARCHITECTURE}")
    list(APPEND EBPF_INCLUDE_FLAGS
        "-I${EBPF_SYSTEM_INCLUDE_ROOT}/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

set(EBPF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/memory_rw.bpf.c
)

set(EBPF_OBJECTS "")

foreach(EBPF_SRC ${EBPF_SOURCES})
    get_filename_component(EBPF_NAME ${EBPF_SRC} NAME_WE)
    set(EBPF_OBJ ${CMAKE_CURRENT_BINARY_DIR}/${EBPF_NAME}.bpf.o)
    
    add_custom_command(
        OUTPUT ${EBPF_OBJ}
        COMMAND ${CLANG}
            -g -O2 -target bpf
            -D__TARGET_ARCH_${EBPF_TARGET_ARCH}
            -I${CMAKE_CURRENT_SOURCE_DIR}
            ${EBPF_INCLUDE_FLAGS}
            -c ${EBPF_SRC}
            -o ${EBPF_OBJ}
        DEPENDS ${EBPF_SRC}
        COMMENT "Compiling eBPF: ${EBPF_NAME}.bpf.c"
        VERBATIM
    )
    
    list(APPEND EBPF_OBJECTS ${EBPF_OBJ})
endforeach()

add_custom_target(ebpf_compile DEPENDS ${EBPF_OBJECTS})

add_custom_target(ebpf_copy
    DEPENDS ebpf_compile
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${EBPF_OBJECTS}
        ${CMAKE_BINARY_DIR}
    COMMENT "Copying eBPF objects to the build directory"
    VERBATIM
)

set(EBPF_OBJECTS ${EBPF_OBJECTS} PARENT_SCOPE)
