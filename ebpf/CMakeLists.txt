cmake_minimum_required(VERSION 4.3.4)

find_program(CLANG clang REQUIRED)
message(STATUS "Using clang for eBPF: ${CLANG}")

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
            -D__TARGET_ARCH_x86_64
            -I${CMAKE_CURRENT_SOURCE_DIR}
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
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}
    COMMENT "Copying eBPF .o files to build dir"
    VERBATIM
)

set(EBPF_OBJECTS ${EBPF_OBJECTS} PARENT_SCOPE)