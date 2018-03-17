set(SRCS
    audio.cpp
    audio.h
    bitfield.h
    clock.cpp
    clock.h
    component.h
    cpu.h
    cpu_x86/backend.h
    cpu_x86/cached_interpreter_backend.cpp
    cpu_x86/cached_interpreter_backend.h
    cpu_x86/code_cache_backend.cpp
    cpu_x86/code_cache_backend.h
    cpu_x86/cpu.cpp
    cpu_x86/cpu.h
    cpu_x86/debugger_interface.cpp
    cpu_x86/debugger_interface.h
    cpu_x86/decode.cpp
    cpu_x86/decode.h
    cpu_x86/fast_interpreter_backend.cpp
    cpu_x86/fast_interpreter_backend.h
    cpu_x86/instprint.cpp
    cpu_x86/interpreter_backend.cpp
    cpu_x86/interpreter_backend.h
    cpu_x86/interpreter.cpp
    cpu_x86/interpreter.h
    cpu_x86/jitx64_backend.cpp
    cpu_x86/jitx64_backend.h
    cpu_x86/jitx64_codegen.cpp
    cpu_x86/jitx64_codegen.h
    cpu_x86/types.h
    debugger_interface.h
    display.cpp
    display.h
    dma_controller.h
    host_interface.cpp
    host_interface.h
    hw/adlib.cpp
    hw/adlib.h
    hw/cga.cpp
    hw/cga.h
    hw/cmos.cpp
    hw/cmos.h
    hw/fdc.cpp
    hw/fdc.h
    hw/hdc.cpp
    hw/hdc.h
    hw/i8237_dma.cpp
    hw/i8237_dma.h
    hw/i8253_pit.cpp
    hw/i8253_pit.h
    hw/i8259_pic.cpp
    hw/i8259_pic.h
    hw/keyboard_scancodes.cpp
    hw/keyboard_scancodes.h
    hw/pcspeaker.cpp
    hw/pcspeaker.h
    hw/ps2.cpp
    hw/ps2.h
    hw/serial.cpp
    hw/serial.h
    hw/serial_mouse.cpp
    hw/serial_mouse.h
    hw/soundblaster.cpp
    hw/soundblaster.h
    hw/vga.cpp
    hw/vga.h
    hw/xt_ppi.cpp
    hw/xt_ppi.h
    hw/ymf262.cpp
    hw/ymf262.h
    interrupt_controller.h
    mmio.cpp
    mmio.h
    save_state_version.h
    scancodes.h
    system.cpp
    system.h
    systems/pcami386.cpp
    systems/pcami386.h
    systems/pcat.cpp
    systems/pcat.h
    systems/pcbase.cpp
    systems/pcbase.h
    systems/pcbochs.cpp
    systems/pcbochs.h
    systems/pcxt.cpp
    systems/pcxt.h
    thirdparty/dosbox/dbopl.cpp
    thirdparty/dosbox/dbopl.h
    timing.cpp
    timing.h
    types.h
)

add_library(pce ${SRCS})

# Disable operators, since it breaks on xbyak.
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_target_properties(pce PROPERTIES COMPILE_FLAGS "-fno-operator-names")
endif()

target_include_directories(pce PRIVATE "${CMAKE_SOURCE_DIR}/dep/xbyak/xbyak")
target_include_directories(pce PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(pce PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(pce YBaseLib libsamplerate Threads::Threads)

