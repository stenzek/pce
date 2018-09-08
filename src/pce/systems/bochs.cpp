#include "pce/systems/bochs.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/hw/pci_bus.h"
#include <memory>
Log_SetChannel(Systems::Bochs);

namespace Systems {
DEFINE_OBJECT_TYPE_INFO(Bochs);
DEFINE_OBJECT_GENERIC_FACTORY(Bochs);
BEGIN_OBJECT_PROPERTY_MAP(Bochs)
END_OBJECT_PROPERTY_MAP()

Bochs::Bochs(CPU_X86::Model model /* = CPU_X86::MODEL_PENTIUM */, float cpu_frequency /* = 8000000.0f */,
             uint32 memory_size /* = 16 * 1024 * 1024 */, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(model, cpu_frequency, memory_size, type_info)
{
  m_bios_file_path = "romimages/BIOS-bochs-latest";
}

Bochs::~Bochs() = default;

bool Bochs::Initialize()
{
  if (!BaseClass::Initialize())
    return false;

  ConnectSystemIOPorts();
  return true;
}

void Bochs::Reset()
{
  BaseClass::Reset();

  // Hack: Set the CMOS variables on reset, so things like floppies are picked up.
  // We should probably set this last, or have a PostInitialize function or something.
  SetCMOSVariables();
}

bool Bochs::LoadSystemState(BinaryReader& reader)
{
  if (!BaseClass::LoadSystemState(reader))
    return false;

  return !reader.GetErrorState();
}

bool Bochs::SaveSystemState(BinaryWriter& writer)
{
  if (!BaseClass::SaveSystemState(writer))
    return false;

  return !writer.InErrorState();
}

void Bochs::ConnectSystemIOPorts()
{
  // Debug ports.
  static const char* debug_port_names[5] = {"panic", "panic2", "info", "debug", "unknown"};
  static const uint32 debug_port_numbers[5] = {0x0400, 0x0401, 0x0402, 0x0403, 0x0500};
  for (uint32 i = 0; i < countof(debug_port_names); i++)
  {
    const char* port_name = debug_port_names[i];
    std::shared_ptr<std::string> str = std::make_shared<std::string>();
    m_bus->ConnectIOPortWrite(debug_port_numbers[i], this, [port_name, str](uint32 port, uint8 value) {
      if (value == '\n')
      {
        Log_DevPrintf("%s port message (%04X): %s", port_name, port, str->c_str());
        str->clear();
      }
      else
      {
        *str += static_cast<char>(value);
      }
    });
  }
}

void Bochs::SetCMOSVariables()
{
  // Bochs CMOS map
  //
  // Idx  Len   Description
  // 0x10   1   floppy drive types
  // 0x11   1   configuration bits
  // 0x12   1   harddisk types
  // 0x13   1   advanced configuration bits
  // 0x15   2   base memory in 1k
  // 0x17   2   memory size above 1M in 1k
  // 0x19   2   extended harddisk types
  // 0x1b   9   harddisk configuration (hd0)
  // 0x24   9   harddisk configuration (hd1)
  // 0x2d   1   boot sequence (fd/hd)
  // 0x30   2   memory size above 1M in 1k
  // 0x34   2   memory size above 16M in 64k
  // 0x38   1   eltorito boot sequence (#3) + bootsig check
  // 0x39   2   ata translation policy (ata0...ata3)
  // 0x3d   1   eltorito boot sequence (#1 + #2)
  //
  // Qemu CMOS map
  //
  // Idx  Len   Description
  // 0x5b   3   extra memory above 4GB
  // 0x5f   1   number of processors

  PhysicalMemoryAddress base_memory_in_k = GetBaseMemorySize() / 1024;
  m_cmos->SetWordVariable(0x15, Truncate16(base_memory_in_k));
  Log_DevPrintf("Base memory in KB: %u", Truncate32(base_memory_in_k));

  PhysicalMemoryAddress extended_memory_in_k = GetTotalMemorySize() / 1024 - 1024;
  if (extended_memory_in_k > 0xFC00)
    extended_memory_in_k = 0xFC00;
  m_cmos->SetWordVariable(0x17, Truncate16(extended_memory_in_k));
  m_cmos->SetWordVariable(0x30, Truncate16(extended_memory_in_k));
  Log_DevPrintf("Extended memory in KB: %u", Truncate32(extended_memory_in_k));

  PhysicalMemoryAddress extended_memory_in_64k = GetTotalMemorySize() / 1024; // TODO: Fix this
  if (extended_memory_in_64k > 16384)
    extended_memory_in_64k = (extended_memory_in_64k - 16384) / 64;
  else
    extended_memory_in_64k = 0;
  if (extended_memory_in_64k > 0xBF00)
    extended_memory_in_64k = 0xBF00;

  m_cmos->SetWordVariable(0x34, Truncate16(extended_memory_in_64k));
  Log_DevPrintf("Extended memory above 16MB in KB: %u", Truncate32(extended_memory_in_64k * 64));

  m_cmos->SetFloppyCount(m_fdd_controller->GetDriveCount());
  for (uint32 i = 0; i < HW::FDC::MAX_DRIVES; i++)
  {
    if (m_fdd_controller->IsDrivePresent(i))
      m_cmos->SetFloppyType(i, m_fdd_controller->GetDriveType_(i));
  }

  // Equipment byte
  uint8 equipment_byte = 0;
  equipment_byte |= (1 << 1); // coprocessor installed
  equipment_byte |= (1 << 2); // ps/2 device/mouse installed
  if (m_fdd_controller->GetDriveCount() > 0)
  {
    equipment_byte |= (1 << 0); // disk available for boot
    if (m_fdd_controller->GetDriveCount() > 1)
      equipment_byte |= (0b01 << 7); // 2 drives installed
  }
  m_cmos->SetVariable(0x14, equipment_byte);

  // Boot from HDD first
  // Legacy - 0 - C: -> A:, 1 - A: -> C:
  m_cmos->SetVariable(0x2D, (0 << 5));
  // 0x00 - undefined, 0x01 - first floppy, 0x02 - first HDD, 0x03 - first cdrom
  if (m_primary_hdd_controller->GetDeviceCount() > 0)
    m_cmos->SetVariable(0x3D, 0x02);
  else
    m_cmos->SetVariable(0x3D, 0x01);

  // Skip IPL validity check
  m_cmos->SetVariable(0x38, 0x01);

  // HDD information
  if (m_primary_hdd_controller->IsHDDPresent(0))
  {
    m_cmos->SetVariable(0x12, m_cmos->GetVariable(0x12) | 0xF0);
    m_cmos->SetVariable(0x19, 47); // user-defined type
    m_cmos->SetVariable(0x1B, Truncate8(m_primary_hdd_controller->GetHDDCylinders(0)));
    m_cmos->SetVariable(0x1C, Truncate8(m_primary_hdd_controller->GetHDDCylinders(0) >> 8));
    m_cmos->SetVariable(0x1D, Truncate8(m_primary_hdd_controller->GetHDDHeads(0)));
    m_cmos->SetVariable(0x1E, 0xFF);
    m_cmos->SetVariable(0x1F, 0xFF);
    m_cmos->SetVariable(0x20, 0xC0 | ((m_primary_hdd_controller->GetHDDHeads(0) > 8) ? 8 : 0));
    m_cmos->SetVariable(0x21, m_cmos->GetVariable(0x1B));
    m_cmos->SetVariable(0x22, m_cmos->GetVariable(0x1C));
    m_cmos->SetVariable(0x23, Truncate8(m_primary_hdd_controller->GetHDDSectors(0)));
  }
  if (m_primary_hdd_controller->IsHDDPresent(1))
  {
    m_cmos->SetVariable(0x12, m_cmos->GetVariable(0x12) | 0x0F);
    m_cmos->SetVariable(0x1A, 48); // user-defined type
    m_cmos->SetVariable(0x24, Truncate8(m_primary_hdd_controller->GetHDDCylinders(1)));
    m_cmos->SetVariable(0x25, Truncate8(m_primary_hdd_controller->GetHDDCylinders(1) >> 8));
    m_cmos->SetVariable(0x26, Truncate8(m_primary_hdd_controller->GetHDDHeads(1)));
    m_cmos->SetVariable(0x27, 0xFF);
    m_cmos->SetVariable(0x28, 0xFF);
    m_cmos->SetVariable(0x29, 0xC0 | ((m_primary_hdd_controller->GetHDDHeads(1) > 8) ? 8 : 0));
    m_cmos->SetVariable(0x2A, m_cmos->GetVariable(0x1B));
    m_cmos->SetVariable(0x2B, m_cmos->GetVariable(0x1C));
    m_cmos->SetVariable(0x2C, Truncate8(m_primary_hdd_controller->GetHDDSectors(1)));
  }
}

} // namespace Systems