add_library(pce
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
    cpu_x86/code_cache_types.cpp
    cpu_x86/code_cache_types.h
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
    hw/bochs_vga.cpp
    hw/bochs_vga.h
    hw/cdrom.cpp
    hw/cdrom.h
    hw/cga.cpp
    hw/cga.h
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
    hw/vga_base.cpp
    hw/vga_base.h
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
    timing_event.cpp
    timing_event.h
    thirdparty/dosbox/dbopl.cpp
    thirdparty/dosbox/dbopl.h
    types.cpp
    types.h
)

set(RECOMPILER_GENERIC_SRCS
  cpu_x86/recompiler_backend.cpp
  cpu_x86/recompiler_backend.h
  cpu_x86/recompiler_code_generator.cpp
  cpu_x86/recompiler_code_generator.h
  cpu_x86/recompiler_code_generator_generic.cpp
  cpu_x86/recompiler_register_cache.cpp
  cpu_x86/recompiler_register_cache.h
  cpu_x86/recompiler_thunks.cpp
  cpu_x86/recompiler_thunks.h
  cpu_x86/recompiler_types.h
)

if(${CPU_ARCH} STREQUAL "x64")
  target_sources(pce PRIVATE ${RECOMPILER_GENERIC_SRCS} cpu_x86/recompiler_code_generator_x64.cpp)
  message("Building x64 recompiler")
else()
  message("Not building recompiler")
endif()

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

# Disable the SSA optimizer on MSVC, it takes forever to compile the interpreter otherwise.
if(MSVC)
  set_source_files_properties(cpu_x86/interpreter.cpp PROPERTIES COMPILE_FLAGS "/d2SSAOptimizer-")
endif()

target_include_directories(pce PRIVATE "${CMAKE_SOURCE_DIR}/dep/xbyak/xbyak")
target_include_directories(pce PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(pce PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(pce Threads::Threads YBaseLib common xxhash softfloat inih)

