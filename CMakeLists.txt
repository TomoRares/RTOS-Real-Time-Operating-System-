cmake_minimum_required(VERSION 3.16)

# Set the toolchain file before project()
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/arm-toolchain.cmake)
endif()

project(rtos C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker.ld)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

# Source files
set(SOURCES
    startup.c
    src/rtos_port.c
    src/rtos_kernel.c
    src/rtos_task.c
    src/rtos_sync.c
    src/rtos_timer.c
    src/hal_uart.c
    src/hal_gpio.c
    src/main.c
)

# Create executable
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Linker flags
target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -nostartfiles
    -nostdlib
)

# Generate binary file
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Building ${PROJECT_NAME}.bin and printing size"
)

# Generate disassembly
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -d -S ${PROJECT_NAME}.elf > ${PROJECT_NAME}.dis
    COMMENT "Generating disassembly"
)

# Custom target to run in QEMU
add_custom_target(run
    COMMAND qemu-system-arm -M netduinoplus2 -nographic -kernel ${PROJECT_NAME}.elf
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Running in QEMU"
)

# Custom target for debugging with GDB
add_custom_target(debug
    COMMAND qemu-system-arm -M netduinoplus2 -nographic -kernel ${PROJECT_NAME}.elf -S -gdb tcp::3333
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Running in QEMU with GDB server on port 3333"
)
