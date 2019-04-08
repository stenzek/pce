set(SRCS
    bus.cpp
    bus.h
    component.cpp
    component.h
    cpu.cpp
    cpu.h
    cpu_8086/cpu.cpp
    cpu_8086/cpu.h
    cpu_8086/debugger_interface.cpp
    cpu_8086/debugger_interface.h
    cpu_8086/decoder.cpp
    cpu_8086/decoder.h
    cpu_8086/instruction.h
    cpu_8086/instructions.cpp
    cpu_8086/types.h
    cpu_x86/backend.h
    cpu_x86/cached_interpreter_backend.cpp
    cpu_x86/cached_interpreter_backend.h
    cpu_x86/code_cache_backend.cpp
    cpu_x86/code_cache_backend.h
    cpu_x86/cpu_x86.cpp
    cpu_x86/cpu_x86.h
    cpu_x86/cycles.cpp
    cpu_x86/cycles.h
    cpu_x86/debugger_interface.cpp
    cpu_x86/debugger_interface.h
    cpu_x86/decoder.cpp
    cpu_x86/decoder.h
    cpu_x86/instruction.h
    cpu_x86/interpreter.cpp
    cpu_x86/interpreter.h
    cpu_x86/interpreter_backend.cpp
    cpu_x86/interpreter_backend.h
    cpu_x86/types.h
    debugger_interface.h
    dma_controller.cpp
    dma_controller.h
    host_interface.cpp
    host_interface.h
    hw/adlib.cpp
    hw/adlib.h
    hw/ata_cdrom.cpp
    hw/ata_cdrom.h
    hw/ata_device.cpp
    hw/ata_device.h
    hw/ata_hdd.cpp
    hw/ata_hdd.h
    hw/cdrom.cpp
    hw/cdrom.h
    hw/cga.cpp
    hw/cga.h
    hw/cmos_info.h
    hw/ds12887.cpp
    hw/ds12887.h
    hw/et4000.cpp
    hw/et4000.h
    hw/fdc.cpp
    hw/fdc.h
    hw/floppy.cpp
    hw/floppy.h
    hw/hdc.cpp
    hw/hdc.h
    hw/i8042_ps2.cpp
    hw/i8042_ps2.h
    hw/i8237_dma.cpp
    hw/i8237_dma.h
    hw/i82437fx.cpp
    hw/i82437fx.h
    hw/i8253_pit.cpp
    hw/i8253_pit.h
    hw/i8259_pic.cpp
    hw/i8259_pic.h
    hw/keyboard_scancodes.cpp
    hw/keyboard_scancodes.h
    hw/pci_bus.cpp
    hw/pci_bus.h
    hw/pci_device.cpp
    hw/pci_device.h
    hw/pci_ide.cpp
    hw/pci_ide.h
    hw/pcspeaker.cpp
    hw/pcspeaker.h
    hw/serial.cpp
    hw/serial.h
    hw/serial_mouse.cpp
    hw/serial_mouse.h
    hw/soundblaster.cpp
    hw/soundblaster.h
    hw/vga.cpp
    hw/vga.h
    hw/xt_ide.cpp
    hw/xt_ide.h
    hw/xt_ppi.cpp
    hw/xt_ppi.h
    hw/ymf262.cpp
    hw/ymf262.h
    interrupt_controller.cpp
    interrupt_controller.h
    mmio.cpp
    mmio.h
    save_state_version.h
    scancodes.h
    system.cpp
    system.h
    systems/ali1429.cpp
    systems/ali1429.h
    systems/ami386.cpp
    systems/ami386.h
    systems/bochs.cpp
    systems/bochs.h
    systems/i430fx.cpp
    systems/i430fx.h
    systems/ibmat.cpp
    systems/ibmat.h
    systems/ibmxt.cpp
    systems/ibmxt.h
    systems/isapc.cpp
    systems/isapc.h
    systems/pcipc.cpp
    systems/pcipc.h
    system_config_parser.cpp
    thirdparty/dosbox/dbopl.cpp
    thirdparty/dosbox/dbopl.h
    types.cpp
    types.h
)

if(${CPU_ARCH} STREQUAL "x64")
  set(JIT_SRCS
    cpu_x86/jitx64_backend.cpp
    cpu_x86/jitx64_backend.h
    cpu_x86/jitx64_code.cpp
    cpu_x86/jitx64_code.h
    cpu_x86/jitx64_codegen.cpp
    cpu_x86/jitx64_codegen.h
  )
  message("Building x64 recompiler")
else()
  set(JIT_SRCS "")
  message("Not building recompiler")
endif()

add_library(pce ${SRCS} ${JIT_SRCS})

if(ENABLE_VOODOO)
  target_sources(pce PRIVATE
    hw/voodoo.h
    hw/voodoo.cpp
    thirdparty/mame/osdcomm.h
    thirdparty/mame/osdcore.cpp
    thirdparty/mame/osdcore.h
    thirdparty/mame/osdsync.cpp
    thirdparty/mame/osdsync.h
    thirdparty/mame/palette.h
    thirdparty/mame/polylgcy.cpp
    thirdparty/mame/polylgcy.h
    thirdparty/mame/rectangle.h
    thirdparty/mame/rgbgen.cpp
    thirdparty/mame/rgbgen.h
    thirdparty/mame/rgbsse.cpp
    thirdparty/mame/rgbsse.h
    thirdparty/mame/rgbutil.h
    thirdparty/mame/vooddefs.h
    thirdparty/mame/voodoo.cpp
    thirdparty/mame/voodoo.h
    thirdparty/mame/voodoo_rast.hxx
  )
endif()

# Disable operators, since it breaks on xbyak.
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_target_properties(pce PROPERTIES COMPILE_FLAGS "-fno-operator-names")
endif()

target_include_directories(pce PRIVATE "${CMAKE_SOURCE_DIR}/dep/xbyak/xbyak")
target_include_directories(pce PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(pce PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(pce Threads::Threads YBaseLib common xxhash softfloat inih)

