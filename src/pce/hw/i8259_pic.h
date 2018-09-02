#pragma once
#include "pce/interrupt_controller.h"
#include "pce/types.h"

namespace HW {

// i8259 Programmable Interrupt Controller
// This simulates the configuration of the two PICs in the AT in one class.
class i8259_PIC : public InterruptController
{
  DECLARE_OBJECT_TYPE_INFO(i8259_PIC, InterruptController);
  DECLARE_OBJECT_NO_FACTORY(i8259_PIC);
  DECLARE_OBJECT_PROPERTY_MAP(i8259_PIC);

public:
  i8259_PIC(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~i8259_PIC();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  uint32 GetInterruptNumber() override;
  void SetInterruptState(uint32 interrupt, bool active) override;

private:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('8', '2', '5', '9');
  static const uint32 NUM_INTERRUPTS = 16; // IRQs
  static const uint32 NUM_INTERRUPTS_PER_PIC = 8;

  static const uint32 MASTER_PIC = 0;
  static const uint32 SLAVE_PIC = 1;
  static const uint32 NUM_PICS = 2;

  static const uint32 IOPORT_MASTER_COMMAND = 0x20;
  static const uint32 IOPORT_MASTER_DATA = 0x21;
  static const uint32 IOPORT_SLAVE_COMMAND = 0xA0;
  static const uint32 IOPORT_SLAVE_DATA = 0xA1;

  static const uint8 NUM_ICW_VALUES = 4;

  // Slave PIC is connected to IRQ 2 on the master.
  static const uint8 SLAVE_IRQ_ON_MASTER = 2;

  enum ICW_FLAGS : uint8
  {
    COMMAND_MASK = 0x18,
    COMMAND_OCW2 = 0x00,
    COMMAND_OCW3 = 0x08,
    COMMAND_ICW1 = 0x10,

    ICW1_ICW4 = 0x01,
    ICW1_SINGLE = 0x02,
    ICW1_INTERVAL4 = 0x04,
    ICW1_LEVEL = 0x08,

    ICW4_AUTO_EOI = 0x02,

    OCW2_EOI = 0x20,
    OCW2_EOI_SPECIFIC = 0x60,
    OCW2_EOI_ROT = 0xA0,
    OCW2_EOI_ROT_SPEC = 0xE0,
    OCW2_SET_ROT_AUTO = 0x80,
    OCW2_CLEAR_ROT_AUTO = 0x00,

    OCW3_READ_IRR = 0x02,
    OCW3_READ_ISR = 0x03,
  };

  void ConnectIOPorts(Bus* bus);
  void CommandPortReadHandler(uint32 pic_index, uint8* value);
  void CommandPortWriteHandler(uint32 pic_index, uint8 value);
  void DataPortWriteHandler(uint32 pic_index, uint8 value);
  void UpdateInterruptRequest();

  struct PICState
  {
    uint8 request_register = 0;    // IRR
    uint8 in_service_register = 0; // ISR
    uint8 mask_register = 0;       // IMR
    uint8 level_triggered = 0;

    // Offset for interrupt numbers
    uint8 vector_offset = 0;

    // Interrupt line status
    uint8 interrupt_line_status = 0;

    // ICW bytes
    union
    {
      uint8 icw_values[4] = {};
      struct
      {
        uint8 icw1, icw2, icw3, icw4;
      };
    };
    uint8 icw_index = NUM_ICW_VALUES;

    // Latched data byte for read-backs
    bool read_isr = 0;

    // Helpers
    uint8 GetHighestPriorityInterruptRequest() const;
    uint8 GetHighestPriorityInServiceInterrupt() const;
    bool HasInterruptRequest() const;
    bool IsAutoEOI() const;
    bool IsLevelTriggered(uint8 irq) const;
  } m_state[2];
};

} // namespace HW
