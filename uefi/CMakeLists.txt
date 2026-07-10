# uefi/CMakeLists.txt

if(NOT EDK2_SOURCE_DIR)
    message(FATAL_ERROR "EDK2_SOURCE_DIR not set!")
endif()

find_package(Python3 REQUIRED)

add_custom_target(uefi_driver
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/build_uefi.py
        --edk2-path ${EDK2_SOURCE_DIR}
        --project-path ${CMAKE_SOURCE_DIR}
        --output-dir ${CMAKE_BINARY_DIR}/uefi
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building UEFI driver with EDK2..."
    VERBATIM
)

if(BUILD_EXAMPLES)
    add_custom_target(uefi_example
        DEPENDS uefi_driver
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/uefi/MemlibDxe.efi
            ${CMAKE_BINARY_DIR}/
        COMMENT "Copying UEFI driver..."
        VERBATIM
    )
endif()