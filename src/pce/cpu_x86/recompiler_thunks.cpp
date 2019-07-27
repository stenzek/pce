#include "recompiler_thunks.h"

namespace CPU_X86::Recompiler {

u8 Thunks::ReadSegmentMemoryByte(CPU* cpu, Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(segment, address);
  cpu->CheckSegmentAccess<sizeof(u8), AccessType::Read>(segment, address, true);
  return cpu->ReadMemoryByte(linear_address);
}

u16 Thunks::ReadSegmentMemoryWord(CPU* cpu, Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(segment, address);
  cpu->CheckSegmentAccess<sizeof(u16), AccessType::Read>(segment, address, true);
  return cpu->ReadMemoryWord(linear_address);
}

u32 Thunks::ReadSegmentMemoryDWord(CPU* cpu, Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(segment, address);
  cpu->CheckSegmentAccess<sizeof(u32), AccessType::Read>(segment, address, true);
  return cpu->ReadMemoryDWord(linear_address);
}

void Thunks::WriteSegmentMemoryByte(CPU* cpu, Segment segment, VirtualMemoryAddress address, u8 value)
{
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(segment, address);
  cpu->CheckSegmentAccess<sizeof(u8), AccessType::Write>(segment, address, true);
  cpu->WriteMemoryByte(linear_address, value);
}

void Thunks::WriteSegmentMemoryWord(CPU* cpu, Segment segment, VirtualMemoryAddress address, u16 value)
{
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(segment, address);
  cpu->CheckSegmentAccess<sizeof(u16), AccessType::Write>(segment, address, true);
  cpu->WriteMemoryWord(linear_address, value);
}

void Thunks::WriteSegmentMemoryDWord(CPU* cpu, Segment segment, VirtualMemoryAddress address, u32 value)
{
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(segment, address);
  cpu->CheckSegmentAccess<sizeof(u32), AccessType::Write>(segment, address, true);
  cpu->WriteMemoryDWord(linear_address, value);
}

void Thunks::RaiseException(CPU* cpu, u32 exception, u32 error_code)
{
  cpu->RaiseException(exception, error_code);
}

} // namespace CPU_X86::Recompiler