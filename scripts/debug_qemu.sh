#!/bin/bash
#
# debug_qemu.sh - Launch RTOS in QEMU with GDB server
#
# Usage: ./scripts/debug_qemu.sh [path/to/rtos.elf]
#
# Then in another terminal:
#   arm-none-eabi-gdb build/rtos.elf
#   (gdb) target remote :3333
#   (gdb) load
#   (gdb) continue
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Default ELF path
ELF_PATH="${1:-$PROJECT_DIR/build/rtos.elf}"

# GDB port
GDB_PORT="${2:-3333}"

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

echo "Starting QEMU in debug mode with: $ELF_PATH"
echo "GDB server on port: $GDB_PORT"
echo ""
echo "Connect with GDB:"
echo "  arm-none-eabi-gdb $ELF_PATH"
echo "  (gdb) target remote :$GDB_PORT"
echo "  (gdb) load"
echo "  (gdb) continue"
echo ""
echo "Press Ctrl+A, X to exit QEMU"
echo "----------------------------------------"

# Run QEMU with GDB server
# -S: Don't start CPU (wait for GDB)
# -gdb tcp::3333: GDB server on port 3333
qemu-system-arm \
    -M netduinoplus2 \
    -nographic \
    -serial mon:stdio \
    -kernel "$ELF_PATH" \
    -S \
    -gdb tcp::${GDB_PORT}
