#pragma once
#include "pce/hw/hdc.h"
#include "pce/hw/pci_device.h"

class ByteStream;

namespace HW {

class PCIIDE : public HDC, private PCIDevice
{
  DECLARE_OBJECT_TYPE_INFO(PCIIDE, HDC);
  DECLARE_GENERIC_COMPONENT_FACTORY(PCIIDE);
  DECLARE_OBJECT_PROPERTY_MAP(PCIIDE);

public:
  enum class Model
  {
    PIIX,
    PIIX3
  };

  PCIIDE(const String& identifier, Model model = Model::PIIX, const ObjectTypeInfo* type_info = &s_type_info);
  ~PCIIDE();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  void ResetConfigSpace(u8 function) override;
  u8 ReadConfigSpace(u8 function, u8 offset) override;
  void WriteConfigSpace(u8 function, u8 offset, u8 value) override;

  bool SupportsDMA() const override;
  bool IsDMARequested(u32 channel) const override;
  void SetDMARequest(u32 channel, u32 drive, bool request) override;
  u32 DMATransfer(u32 channel, u32 drive, bool is_write, void* data, u32 size) override;

protected:
  static constexpr u32 INVALID_PRDT_INDEX = 0;

  struct DMAState
  {
    union CommandRegister
    {
      BitField<u8, bool, 0, 1> transfer_start;
      BitField<u8, bool, 3, 1> is_write;
      u8 bits = 0;
    } command;
    union StatusRegister
    {
      BitField<u8, bool, 7, 1> simplex;
      BitField<u8, u8, 5, 2> user;
      BitField<u8, bool, 2, 1> irq_requested;
      BitField<u8, bool, 1, 1> transfer_failed;
      BitField<u8, bool, 0, 1> bus_dma_mode;
      u8 bits = 0;
    } status;
    u32 prdt_address = 0;

    u32 active_drive_number = DEVICES_PER_CHANNEL; // DEVICES_PER_CHANNEL if not active
    u32 next_prdt_entry_index = INVALID_PRDT_INDEX;
    PhysicalMemoryAddress current_physical_address = 0;
    u32 remaining_byte_count = 0;
    bool eot = true;
  };

  void ConnectIOPorts(Bus* bus) override;
  void DoReset(u32 channel, bool hardware_reset) override;
  void UpdateHostInterruptLine(u32 channel) override;

  void OnCommandRegisterChanged(u8 function) override;
  void OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active) override;

  u8 IOReadBusMasterCommandRegister(u8 channel);
  u8 IOReadBusMasterStatusRegister(u8 channel);
  u8 IOReadBusMasterPRDTAddress(u8 channel, u8 offset);

  void IOWriteBusMasterCommandRegister(u8 channel, u8 value);
  void IOWriteBusMasterStatusRegister(u8 channel, u8 value);
  void IOWriteBusMasterPRDTAddress(u8 channel, u8 offset, u8 value);

  bool IsChannelEnabled(u32 channel) const;

  void OnDMAStateChanged(u32 channel);
  void ReadNextPRDT(u32 channel);

  Model m_model;
  DMAState m_dma_state[MAX_CHANNELS];

private:
  static constexpr u32 SERIALIZATION_ID = Component::MakeSerializationID('P', 'I', 'I', 'X');
};

} // namespace HW