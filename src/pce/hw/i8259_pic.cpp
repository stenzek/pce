#include "pce/hw/i8259_pic.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
#include "pce/system.h"
Log_SetChannel(i8259_PIC);

namespace HW {

i8259_PIC::i8259_PIC() {}

i8259_PIC::~i8259_PIC() {}

void i8259_PIC::Initialize(System* system, Bus* bus)
{
  m_system = system;
  ConnectIOPorts(bus);
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
  UpdateCPUInterruptLineState();
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
    reader.SafeReadUInt8(&pic->vector_offset);
    reader.SafeReadUInt8(&pic->interrupt_line_status);
    reader.SafeReadUInt8(&pic->interrupt_active_status);
    reader.SafeReadBytes(pic->icw_values, sizeof(pic->icw_values));
    reader.SafeReadUInt8(&pic->icw_index);
    reader.SafeReadUInt8(&pic->latch);
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
    writer.SafeWriteUInt8(pic->vector_offset);
    writer.SafeWriteUInt8(pic->interrupt_line_status);
    writer.SafeWriteUInt8(pic->interrupt_active_status);
    writer.SafeWriteBytes(pic->icw_values, sizeof(pic->icw_values));
    writer.SafeWriteUInt8(pic->icw_index);
    writer.SafeWriteUInt8(pic->latch);
  }

  return true;
}

uint32 i8259_PIC::GetPendingInterruptNumber() const
{
  // Master pic has higher priority
  for (uint32 index = 0; index < NUM_PICS; index++)
  {
    const PICState* pic = &m_state[index];
    uint8 pending_mask = pic->request_register & ~pic->mask_register & ~pic->in_service_register;
    if (pending_mask == 0)
      continue;

    uint32 bit;
    if (Y_bitscanforward(pending_mask, &bit))
      return uint32(pic->vector_offset) + bit;
  }

  // TODO: Handle spurious IRQs for slave PIC
  Log_WarningPrintf("No interrupts available");
  m_system->GetCPU()->SetIRQState(false);
  return m_state[0].vector_offset + 7;
}

void i8259_PIC::TriggerInterrupt(uint32 interrupt)
{
  uint32 pic_index = interrupt / NUM_INTERRUPTS_PER_PIC;
  uint32 interrupt_number = interrupt % NUM_INTERRUPTS_PER_PIC;
  if (pic_index >= NUM_PICS)
    return;

  PICState* pic = &m_state[pic_index];
  uint8 bit = (1 << interrupt_number);

  // If the interrupt is masked, forget it entirely.
  if (pic->mask_register & bit)
  {
    Log_WarningPrintf("Lost directly triggered interrupt %u due to mask", interrupt);
    return;
  }

  // Alter the IRR directly.
  // We don't bother modifying the active state, since each call to TriggerInterrupt should be
  // considered a new "active" trigger. This is bad practice, but meh.
  pic->request_register |= bit;
  UpdateCPUInterruptLineState();
}

void i8259_PIC::RaiseInterrupt(uint32 interrupt)
{
  uint32 pic_index = interrupt / NUM_INTERRUPTS_PER_PIC;
  uint32 interrupt_number = interrupt % NUM_INTERRUPTS_PER_PIC;
  if (pic_index >= NUM_PICS)
    return;

  PICState* pic = &m_state[pic_index];
  uint8 bit = (1 << interrupt_number);

  // If the line status hasn't changed, don't do anything.
  if (pic->interrupt_active_status & bit)
    return;

  // Trigger the interrupt if it wasn't already triggered.
  pic->interrupt_line_status |= bit;

  // We don't update the request register if it's masked.
  // If the mask is cleared later, we'll pick it up from the line status.
  if (pic->mask_register & bit)
    return;

  // We're not masked, so trigger this interrupt now.
  pic->interrupt_active_status |= bit;
  pic->request_register |= bit;
  UpdateCPUInterruptLineState(interrupt);
}

void i8259_PIC::LowerInterrupt(uint32 interrupt)
{
  uint32 pic_index = interrupt / NUM_INTERRUPTS_PER_PIC;
  uint32 interrupt_number = interrupt % NUM_INTERRUPTS_PER_PIC;
  if (pic_index >= NUM_PICS)
    return;

  PICState* pic = &m_state[pic_index];
  uint8 bit = (1 << interrupt_number);

  // If the line status hasn't changed, don't do anything.
  if (!(pic->interrupt_active_status & bit))
    return;

  // Update line state.
  pic->interrupt_line_status &= ~bit;

  // If this interrupt had not been triggered, log a message.
  // This happens when it is masked and raise -> lower without unmasking.
  if (!(pic->interrupt_active_status & bit))
  {
    // The IRR should be clear of this bit.
    Log_WarningPrintf("Lost interrupt %u due to masking?", interrupt);
    Assert(!(pic->request_register & bit));
    return;
  }

  // We can re-trigger this interrupt now, so clear the active bit.
  pic->interrupt_active_status &= ~bit;

  // Should we be clearing the IRR here?
  pic->request_register &= ~bit;

  // Reset interrupt line in case this was keeping it high.
  UpdateCPUInterruptLineState();
}

i8259_PIC::PICState* i8259_PIC::GetPICForVector(uint32 interrupt, uint32* irq)
{
  for (uint32 i = 0; i < NUM_PICS; i++)
  {
    PICState* pic = &m_state[i];
    if (interrupt >= pic->vector_offset && interrupt < (pic->vector_offset + NUM_INTERRUPTS_PER_PIC))
    {
      *irq = interrupt - pic->vector_offset;
      return pic;
    }
  }

  return nullptr;
}

void i8259_PIC::AcknowledgeInterrupt(uint32 interrupt)
{
  uint32 irq_number;
  PICState* pic = GetPICForVector(interrupt, &irq_number);
  if (!pic)
    return;

  uint8 bit = (1 << irq_number);
  if (!(pic->request_register & bit))
    return;

  // In 8086 mode the IRR is cleared when the ISR is set
  pic->in_service_register |= bit;
  pic->request_register &= ~bit;

  // Auto-EOI enabled?
  if (pic->icw4 & ICW4_AUTO_EOI)
  {
    // Clear the ISR
    pic->in_service_register &= ~bit;
  }

  // In case we have another interrupt of lower priority, reset IRQ line.
  UpdateCPUInterruptLineState();
}

void i8259_PIC::InterruptServiced(uint32 interrupt)
{
  Panic("Unused");
}

void i8259_PIC::ConnectIOPorts(Bus* bus)
{
  // Command ports read latched data and have a complex write handler for initialization
  bus->ConnectIOPortRead(
    IOPORT_MASTER_COMMAND, this,
    std::bind(&i8259_PIC::CommandPortReadHandler, this, std::placeholders::_1, std::placeholders::_2));
  bus->ConnectIOPortRead(
    IOPORT_SLAVE_COMMAND, this,
    std::bind(&i8259_PIC::CommandPortReadHandler, this, std::placeholders::_1, std::placeholders::_2));
  bus->ConnectIOPortWrite(
    IOPORT_MASTER_COMMAND, this,
    std::bind(&i8259_PIC::CommandPortWriteHandler, this, std::placeholders::_1, std::placeholders::_2));
  bus->ConnectIOPortWrite(
    IOPORT_SLAVE_COMMAND, this,
    std::bind(&i8259_PIC::CommandPortWriteHandler, this, std::placeholders::_1, std::placeholders::_2));

  // Data ports read/write the IMR
  bus->ConnectIOPortReadToPointer(IOPORT_MASTER_DATA, this, &m_state[MASTER_PIC].mask_register);
  bus->ConnectIOPortReadToPointer(IOPORT_SLAVE_DATA, this, &m_state[SLAVE_PIC].mask_register);
  bus->ConnectIOPortWrite(
    IOPORT_MASTER_DATA, this,
    std::bind(&i8259_PIC::DataPortWriteHandler, this, std::placeholders::_1, std::placeholders::_2));
  bus->ConnectIOPortWrite(
    IOPORT_SLAVE_DATA, this,
    std::bind(&i8259_PIC::DataPortWriteHandler, this, std::placeholders::_1, std::placeholders::_2));
}

void i8259_PIC::CommandPortReadHandler(uint32 port, uint8* value)
{
  uint32 pic_index;
  if (port == IOPORT_MASTER_COMMAND)
    pic_index = MASTER_PIC;
  else if (port == IOPORT_SLAVE_COMMAND)
    pic_index = SLAVE_PIC;
  else
    return;

  PICState* pic = &m_state[pic_index];

  // Not sure if this should be updated each time?
  //*value = pic->latch;
  if (pic->latch == 0)
    *value = pic->request_register;
  else // if (pic->latch == 1)
    *value = pic->in_service_register;
}

void i8259_PIC::CommandPortWriteHandler(uint32 port, uint8 value)
{
  uint32 pic_index;
  if (port == IOPORT_MASTER_COMMAND)
    pic_index = MASTER_PIC;
  else if (port == IOPORT_SLAVE_COMMAND)
    pic_index = SLAVE_PIC;
  else
    return;

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
          // TODO: If PIC is 0, IRQ 2 is the cascaded one, implement this properly.
          if (interrupt != 2)
          {
            Log_WarningPrintf("Specific EOI %u received on PIC %u without ISR set", ZeroExtend32(interrupt), pic_index);
          }
          return;
        }
      }
      else
      {
        // Find the highest priority interrupt, this will be the one being EOI'ed
        uint32 bit;
        if (!Y_bitscanforward(pic->in_service_register, &bit))
        {
          Log_WarningPrintf("EOI received on PIC %u without ISR set", pic_index);
          return;
        }
        interrupt = Truncate8(bit);
      }

      Log_TracePrintf("EOI interrupt %u", ZeroExtend32(interrupt));
      pic->in_service_register &= ~(UINT8_C(1) << interrupt);
      UpdateCPUInterruptLineState();
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
        pic->latch = 0; // pic->request_register;
        break;
      case OCW3_READ_ISR:
        pic->latch = 1; // pic->in_service_register;
        break;
      default:
        Log_ErrorPrintf("Unknown OCW3 command: 0x%02X", ZeroExtend32(value));
        break;
    }
  }
}

void i8259_PIC::DataPortWriteHandler(uint32 port, uint8 value)
{
  uint32 pic_index;
  if (port == IOPORT_MASTER_DATA)
    pic_index = MASTER_PIC;
  else if (port == IOPORT_SLAVE_DATA)
    pic_index = SLAVE_PIC;
  else
    return;

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
      UpdateCPUInterruptLineState();
    }
  }
  else
  {
    // Update IMR
    Log_TracePrintf("PIC %u mask 0x%02X", pic_index, ZeroExtend32(value));
    pic->mask_register = value;

    // Check for interrupts that are held high but were masked previously.
    // Windows 95 does this for hard drive interrupts.
    uint8 late_trigger_interrupts = pic->interrupt_line_status &    // Interrupts that are currently high
                                    ~pic->interrupt_active_status & // Minus those that have already been triggered
                                    ~pic->request_register &        // Minus those pending to the CPU
                                    ~pic->mask_register;            // Minus those still masked
    if (late_trigger_interrupts != 0)
    {
      pic->interrupt_active_status |= late_trigger_interrupts;
      pic->request_register |= late_trigger_interrupts;
    }

    UpdateCPUInterruptLineState();
  }
}

void i8259_PIC::UpdateCPUInterruptLineState()
{
  // Re-assert IRQ line if there are any other interrupts pending (this will be lower priority)
  for (uint32 i = 0; i < NUM_PICS; i++)
  {
    uint8 mask = m_state[i].request_register & ~m_state[i].in_service_register & ~m_state[i].mask_register;
    if (mask != 0)
    {
      m_system->GetCPU()->SetIRQState(true);
      return;
    }
  }

  m_system->GetCPU()->SetIRQState(false);
}

void i8259_PIC::UpdateCPUInterruptLineState(uint32 triggered_interrupt)
{
  // If this interrupt has a higher priority than the current interrupt in-service, assert the IRQ line
  uint32 highest_interrupt = ~0u;
  for (uint32 i = 0; i < NUM_PICS; i++)
  {
    uint8 mask = m_state[i].in_service_register;
    uint32 index;
    if (Y_bitscanforward(mask, &index))
      highest_interrupt = std::min(highest_interrupt, (i * NUM_INTERRUPTS_PER_PIC) + index);
  }

  if (triggered_interrupt < highest_interrupt)
    m_system->GetCPU()->SetIRQState(true);
}

} // namespace HW
