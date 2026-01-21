#!/bin/bash
#
# run_qemu.sh - Launch RTOS in QEMU
#
# Usage: ./scripts/run_qemu.sh [path/to/rtos.elf]
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Default ELF path
ELF_PATH="${1:-$PROJECT_DIR/build/rtos.elf}"

# Check if ELF exists
if [ ! -f "$ELF_PATH" ]; then
    echo "Error: ELF file not found: $ELF_PATH"
    echo ""
    echo "Build the project first:"
    echo "  mkdir -p build && cd build"
    echo "  cmake -DCMAKE_TOOLCHAIN_FILE=../arm-toolchain.cmake .."
    echo "  make"
    exit 1
fi

echo "Starting QEMU with: $ELF_PATH"
echo "Press Ctrl+A, X to exit QEMU"
echo "----------------------------------------"

# Run QEMU
# -M netduinoplus2: Netduino Plus 2 board (STM32F407)
# -nographic: No graphics, UART to console
# -serial mon:stdio: UART and monitor on stdio
# -kernel: ELF file to load
qemu-system-arm \
    -M netduinoplus2 \
    -nographic \
    -serial mon:stdio \
    -semihosting \
    -kernel "$ELF_PATH"
