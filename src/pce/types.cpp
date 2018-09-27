#include "pce/bus.h"
#include "pce/cpu.h"
#include "pce/cpu_8086/cpu.h"
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/hw/adlib.h"
#include "pce/hw/ata_cdrom.h"
#include "pce/hw/ata_hdd.h"
#include "pce/hw/cdrom.h"
#include "pce/hw/cga.h"
#include "pce/hw/cmos.h"
#include "pce/hw/et4000.h"
#include "pce/hw/fdc.h"
#include "pce/hw/floppy.h"
#include "pce/hw/hdc.h"
#include "pce/hw/i8042_ps2.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/hw/pci_bus.h"
#include "pce/hw/pci_device.h"
#include "pce/hw/pci_ide.h"
#include "pce/hw/pcspeaker.h"
#include "pce/hw/serial.h"
#include "pce/hw/serial_mouse.h"
#include "pce/hw/soundblaster.h"
#include "pce/hw/vga.h"
#include "pce/hw/xt_ide.h"
#include "pce/hw/xt_ppi.h"
#include "pce/system.h"
#include "pce/systems/ali1429.h"
#include "pce/systems/ami386.h"
#include "pce/systems/bochs.h"
#include "pce/systems/i430fx.h"
#include "pce/systems/ibmat.h"
#include "pce/systems/ibmxt.h"
#include "pce/systems/isapc.h"
#include "pce/systems/pcipc.h"

#define REGISTER_TYPE(type) type::StaticMutableTypeInfo()->RegisterType()
#define REGISTER_HW_TYPE(type) REGISTER_TYPE(HW::type)
#define REGISTER_SYSTEM_TYPE(type) REGISTER_TYPE(Systems::type)

void RegisterAllTypes()
{
  REGISTER_TYPE(Object);
  REGISTER_TYPE(Component);
  REGISTER_TYPE(Bus);
  REGISTER_TYPE(System);
  REGISTER_TYPE(CPU);
  REGISTER_TYPE(InterruptController);
  REGISTER_TYPE(DMAController);

  REGISTER_TYPE(PCIBus);
  REGISTER_TYPE(PCIDevice);

  REGISTER_TYPE(CPU_8086::CPU);
  REGISTER_TYPE(CPU_X86::CPU);

  REGISTER_HW_TYPE(ATADevice);

  REGISTER_HW_TYPE(AdLib);
  REGISTER_HW_TYPE(ATAHDD);
  REGISTER_HW_TYPE(ATACDROM);
  REGISTER_HW_TYPE(CDROM);
  REGISTER_HW_TYPE(CGA);
  REGISTER_HW_TYPE(CMOS);
  REGISTER_HW_TYPE(ET4000);
  REGISTER_HW_TYPE(FDC);
  REGISTER_HW_TYPE(Floppy);
  REGISTER_HW_TYPE(HDC);
  REGISTER_HW_TYPE(i8042_PS2);
  REGISTER_HW_TYPE(i8237_DMA);
  REGISTER_HW_TYPE(i8253_PIT);
  REGISTER_HW_TYPE(i8259_PIC);
  REGISTER_HW_TYPE(PCIIDE);
  REGISTER_HW_TYPE(PCSpeaker);
  REGISTER_HW_TYPE(Serial);
  REGISTER_HW_TYPE(SerialMouse);
  REGISTER_HW_TYPE(SoundBlaster);
  REGISTER_HW_TYPE(VGA);
  REGISTER_HW_TYPE(XT_IDE);
  REGISTER_HW_TYPE(XT_PPI);

  REGISTER_SYSTEM_TYPE(ISAPC);
  REGISTER_SYSTEM_TYPE(PCIPC);

  REGISTER_SYSTEM_TYPE(ALi1429);
  REGISTER_SYSTEM_TYPE(AMI386);
  REGISTER_SYSTEM_TYPE(Bochs);
  REGISTER_SYSTEM_TYPE(i430FX);
  REGISTER_SYSTEM_TYPE(IBMAT);
  REGISTER_SYSTEM_TYPE(IBMXT);
}
