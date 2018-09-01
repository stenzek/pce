#include "pce/hw/i8259_pic.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
#include "pce/system.h"
Log_SetChannel(i8259_PIC);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(i8259_PIC);
BEGIN_OBJECT_PROPERTY_MAP(i8259_PIC)
END_OBJECT_PROPERTY_MAP()

i8259_PIC::i8259_PIC() {}

i8259_PIC::~i8259_PIC() {}

bool i8259_PIC::Initialize(System* system, Bus* bus)
{
  m_system = system;
  ConnectIOPorts(bus);
  return true;
}

void i8259_PIC::Reset()
{
  for (uint32 index = 0; index < NUM_PICS; index++)
  {
    PICState* pic = &m_state[index];
    pic->in_service_register = 0;
    pic->request_register = 0;
    pic->mask_register = 0;
    pic->vector_offset = 0;
  }

  // For HLE bios, remove later..
  m_state[0].vector_offset = 0x08;
  m_state[1].vector_offset = 0x70;
  UpdateInterruptRequest();
}

bool i8259_PIC::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  for (uint32 i = 0; i < NUM_PICS; i++)
  {
    PICState* pic = &m_state[i];

    reader.SafeReadUInt8(&pic->request_register);
    reader.SafeReadUInt8(&pic->in_service_register);
    reader.SafeReadUInt8(&pic->mask_register);
    reader.SafeReadUInt8(&pic->level_triggered);
    reader.SafeReadUInt8(&pic->vector_offset);
    reader.SafeReadUInt8(&pic->interrupt_line_status);
    reader.SafeReadBytes(pic->icw_values, sizeof(pic->icw_values));
    reader.SafeReadUInt8(&pic->icw_index);
    reader.SafeReadBool(&pic->read_isr);
  }

  return true;
}

bool i8259_PIC::SaveState(BinaryWriter& writer)
{
  writer.SafeWriteUInt32(SERIALIZATION_ID);

  for (uint32 i = 0; i < NUM_PICS; i++)
  {
    PICState* pic = &m_state[i];

    writer.SafeWriteUInt8(pic->request_register);
    writer.SafeWriteUInt8(pic->in_service_register);
    writer.SafeWriteUInt8(pic->mask_register);
    writer.SafeWriteUInt8(pic->level_triggered);
    writer.SafeWriteUInt8(pic->vector_offset);
    writer.SafeWriteUInt8(pic->interrupt_line_status);
    writer.SafeWriteBytes(pic->icw_values, sizeof(pic->icw_values));
    writer.SafeWriteUInt8(pic->icw_index);
    writer.SafeWriteBool(pic->read_isr);
  }

  return true;
}

uint8 i8259_PIC::PICState::GetHighestPriorityInterruptRequest() const
{
  const uint8 pending_mask = request_register & ~mask_register & ~in_service_register;
  if (pending_mask == 0)
    return NUM_INTERRUPTS_PER_PIC;

  uint32 bit;
  Y_bitscanforward(pending_mask, &bit);
  return uint8(bit);
}

uint8 i8259_PIC::PICState::GetHighestPriorityInServiceInterrupt() const
{
  if (in_service_register == 0)
    return NUM_INTERRUPTS_PER_PIC;

  uint32 bit;
  Y_bitscanforward(in_service_register, &bit);
  return uint8(bit);
}

bool i8259_PIC::PICState::HasInterruptRequest() const
{
  const uint8 pending_mask = request_register & ~mask_register & ~in_service_register;
  if (pending_mask == 0)
    return false;

  uint32 irq_number, in_service_irq_number;
  Y_bitscanforward(pending_mask, &irq_number);

  // The interrupt request must have a lower priority than the current in-service interrupt.
  if (in_service_register != 0)
  {
    Y_bitscanforward(in_service_register, &in_service_irq_number);
    return (in_service_irq_number > irq_number);
  }
  else
  {
    return true;
  }
}

bool i8259_PIC::PICState::IsAutoEOI() const
{
  return (icw4 & ICW4_AUTO_EOI) != 0;
}

bool i8259_PIC::PICState::IsLevelTriggered(uint8 irq) const
{
  return (level_triggered & (1 << irq)) != 0;
}

uint32 i8259_PIC::GetInterruptNumber()
{
  PICState* master_pic = &m_state[MASTER_PIC];

  // Master pic has higher priority
  if (!master_pic->HasInterruptRequest())
  {
    Log_WarningPrintf("Spurious IRQ on master PIC");
    UpdateInterruptRequest();
    return master_pic->vector_offset + 7;
  }

  // Auto-EOI would set then clear the ISR, so let's just not set it.
  uint8 irq = master_pic->GetHighestPriorityInterruptRequest();
  uint8 interrupt_number = master_pic->vector_offset + irq;
  uint8 bit = (1 << irq);
  if (!master_pic->IsAutoEOI())
    master_pic->in_service_register |= bit;

  // Level-triggered IRR is changed externally.
  if (!master_pic->IsLevelTriggered(irq))
    master_pic->request_register &= ~bit;

  // Cascaded IRQ?
  if (irq == SLAVE_IRQ_ON_MASTER)
  {
    PICState* slave_pic = &m_state[SLAVE_PIC];

    // Get IRQ from slave PIC.
    if (!slave_pic->HasInterruptRequest())
    {
      Log_WarningPrintf("Spurious IRQ on slave PIC");
      UpdateInterruptRequest();
      return slave_pic->vector_offset + 7;
    }

    irq = slave_pic->GetHighestPriorityInterruptRequest();
    interrupt_number = slave_pic->vector_offset + irq;
    bit = (1 << irq);
    if (!slave_pic->IsAutoEOI())
      slave_pic->in_service_register |= bit;
    if (!slave_pic->IsLevelTriggered(irq))
      slave_pic->request_register &= ~bit;
  }

  UpdateInterruptRequest();
  return interrupt_number;
}

void i8259_PIC::SetInterruptState(uint32 interrupt, bool active)
{
  uint8 pic_index = uint8(interrupt / NUM_INTERRUPTS_PER_PIC);
  uint8 interrupt_number = uint8(interrupt % NUM_INTERRUPTS_PER_PIC);
  if (pic_index >= NUM_PICS)
    return;

  PICState* pic = &m_state[pic_index];
  uint8 bit = (1 << interrupt_number);

  // If the line status hasn't changed, don't do anything.
  const bool current_state = ConvertToBoolUnchecked((pic->interrupt_line_status >> interrupt_number) & 1);
  if (current_state == active)
    return;

  // Update state.
  if (active)
    pic->interrupt_line_status |= bit;
  else
    pic->interrupt_line_status &= ~bit;

  // Set IRR on positive edge only in edge-triggered mode.
  // Update IRR whenever there is a change in level-triggered mode.
  if (active)
    pic->request_register |= bit;
  else if (pic->IsLevelTriggered(interrupt_number))
    pic->request_register &= ~bit;

  // Update INTR.
  UpdateInterruptRequest();
}

void i8259_PIC::ConnectIOPorts(Bus* bus)
{
  // Command ports read latched data and have a complex write handler for initialization
  bus->ConnectIOPortRead(IOPORT_MASTER_COMMAND, this,
                         std::bind(&i8259_PIC::CommandPortReadHandler, this, MASTER_PIC, std::placeholders::_2));
  bus->ConnectIOPortRead(IOPORT_SLAVE_COMMAND, this,
                         std::bind(&i8259_PIC::CommandPortReadHandler, this, SLAVE_PIC, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_MASTER_COMMAND, this,
                          std::bind(&i8259_PIC::CommandPortWriteHandler, this, MASTER_PIC, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_SLAVE_COMMAND, this,
                          std::bind(&i8259_PIC::CommandPortWriteHandler, this, SLAVE_PIC, std::placeholders::_2));

  // Data ports read/write the IMR
  bus->ConnectIOPortReadToPointer(IOPORT_MASTER_DATA, this, &m_state[MASTER_PIC].mask_register);
  bus->ConnectIOPortReadToPointer(IOPORT_SLAVE_DATA, this, &m_state[SLAVE_PIC].mask_register);
  bus->ConnectIOPortWrite(IOPORT_MASTER_DATA, this,
                          std::bind(&i8259_PIC::DataPortWriteHandler, this, MASTER_PIC, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_SLAVE_DATA, this,
                          std::bind(&i8259_PIC::DataPortWriteHandler, this, SLAVE_PIC, std::placeholders::_2));
}

void i8259_PIC::CommandPortReadHandler(uint32 pic_index, uint8* value)
{
  PICState* pic = &m_state[pic_index];
  *value = pic->read_isr ? pic->in_service_register : pic->request_register;
}

void i8259_PIC::CommandPortWriteHandler(uint32 pic_index, uint8 value)
{
  PICState* pic = &m_state[pic_index];
  uint8 command_type = value & COMMAND_MASK;

  // Re-initializing?
  if (command_type == COMMAND_ICW1)
  {
    // ICW1_ICW4 = 0x01
    // ICW1_SINGLE = 0x02
    // ICW1_INTERVAL4 = 0x04
    // ICW1_LEVEL = 0x08
    Log_DevPrintf("Re-initializing PIC %u, ICW1 = 0x%02X", pic_index, ZeroExtend32(value));
    pic->icw_values[0] = value;
    pic->icw_index = 1;
    pic->mask_register = 0;
    pic->in_service_register = 0;
    pic->request_register = 0;
  }
  else if (command_type == COMMAND_OCW2)
  {
    uint8 ocw2_type = value & 0xE0;
    if (ocw2_type & OCW2_EOI)
    {
      uint8 interrupt = 0xFF;
      if ((ocw2_type & OCW2_EOI_SPECIFIC) == OCW2_EOI_SPECIFIC)
      {
        // Specific EOI
        interrupt = value & 0x07;
        if (!(pic->in_service_register & (UINT8_C(1) << interrupt)))
        {
          Log_WarningPrintf("Specific EOI %u received on PIC %u without ISR set", ZeroExtend32(interrupt), pic_index);
          return;
        }
      }
      else
      {
        // Find the highest priority interrupt, this will be the one being EOI'ed
        if (pic->in_service_register == 0)
        {
          Log_WarningPrintf("Auto EOI received on PIC %u without ISR set", pic_index);
          return;
        }

        interrupt = pic->GetHighestPriorityInServiceInterrupt();
      }

      Log_TracePrintf("EOI interrupt %u", ZeroExtend32(interrupt));
      pic->in_service_register &= ~(UINT8_C(1) << interrupt);
      UpdateInterruptRequest();
    }
    else
    {
      Log_ErrorPrintf("Unknown OCW2 command: 0x%02X", ZeroExtend32(value));
    }
  }
  else if (command_type == COMMAND_OCW3)
  {
    // Read registers
    // This should return the current value every time it's read..
    switch (value & ~COMMAND_MASK)
    {
      case OCW3_READ_IRR:
        pic->read_isr = false;
        break;
      case OCW3_READ_ISR:
        pic->read_isr = true;
        break;
      default:
        Log_ErrorPrintf("Unknown OCW3 command: 0x%02X", ZeroExtend32(value));
        break;
    }
  }
}

void i8259_PIC::DataPortWriteHandler(uint32 pic_index, uint8 value)
{
  PICState* pic = &m_state[pic_index];

  // Waiting for re-initialization?
  if (pic->icw_index != NUM_ICW_VALUES)
  {
    pic->icw_values[pic->icw_index] = value;

    switch (pic->icw_index)
    {
      case 1:
        Log_DevPrintf("PIC %u vector offset 0x%02X", pic_index, ZeroExtend32(value));
        break;
      case 2:
        Log_DevPrintf("PIC %u cascade state 0x%02X", pic_index, ZeroExtend32(value));
        break;
      case 3:
        // ICW4 & 0x02 == auto-eoi
        Log_DevPrintf("PIC %u operational mode 0x%02X", pic_index, ZeroExtend32(value));
        break;
      default:
        break;
    }

    pic->icw_index++;

    // The PC doesn't send ICW3 when in single mode.
    if (pic->icw1 & 0x02 && pic->icw_index == 2)
    {
      pic->icw3 = 0;
      pic->icw_index++;
    }

    // Initialization complete?
    if ((!(pic->icw_values[0] & 0x01) && pic->icw_index == (NUM_ICW_VALUES - 1)) || pic->icw_index == NUM_ICW_VALUES)
    {
      Log_DevPrintf("PIC %u init complete", pic_index);

      // Update the state of everything
      pic->icw_index = NUM_ICW_VALUES;
      pic->vector_offset = pic->icw_values[1];
      UpdateInterruptRequest();
    }
  }
  else
  {
    // Update IMR
    Log_TracePrintf("PIC %u mask 0x%02X", pic_index, ZeroExtend32(value));
    pic->mask_register = value;
    UpdateInterruptRequest();
  }
}

void i8259_PIC::UpdateInterruptRequest()
{
  // Slave PIC changes IRQ2 on master. TODO: Optimize this bit.
  SetInterruptState(SLAVE_IRQ_ON_MASTER, m_state[SLAVE_PIC].HasInterruptRequest());

  // Master PIC drives the CPU INTR line.
  m_system->GetCPU()->SetIRQState(m_state[MASTER_PIC].HasInterruptRequest());
}
} // namespace HW
