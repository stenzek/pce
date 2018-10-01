#pragma once
#include "common/bitfield.h"
#include "pce/component.h"
#include <vector>

class PCIBus;

class PCIDevice : public Component
{
  DECLARE_OBJECT_TYPE_INFO(PCIDevice, Component);
  DECLARE_OBJECT_NO_FACTORY(PCIDevice);
  DECLARE_OBJECT_PROPERTY_MAP(PCIDevice);

public:
  static constexpr uint32 NUM_CONFIG_REGISTERS = 64;

  PCIDevice(const String& identifier, u8 num_functions = 1, const ObjectTypeInfo* type_info = &s_type_info);
  ~PCIDevice();

  u32 GetPCIBusNumber() const { return m_pci_bus_number; }
  u32 GetPCIDeviceNumber() const { return m_pci_device_number; }
  u8 GetNumFunctions() const { return m_num_functions; }
  void SetLocation(u32 pci_bus_number, u32 pci_device_number);

  // Returns the PCI bus which this device is attached to.
  PCIBus* GetPCIBus() const;

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;

  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  virtual u8 ReadConfigSpace(u8 function, u8 offset);
  virtual void WriteConfigSpace(u8 function, u8 offset, u8 value);

protected:
  enum MemoryRegion : u8
  {
    MemoryRegion_BAR0,
    MemoryRegion_BAR1,
    MemoryRegion_BAR2,
    MemoryRegion_BAR3,
    MemoryRegion_BAR4,
    MemoryRegion_BAR5,
    MemoryRegion_ExpansionROM,
    NumMemoryRegions
  };

  struct ConfigSpaceHeader
  {
    u16 vendor_id; // 0x00
    u16 device_id; // 0x02
    union
    {
      BitField<u16, bool, 0, 1> enable_io_space;
      BitField<u16, bool, 1, 1> enable_memory_space;
      BitField<u16, bool, 2, 1> enable_bus_master;
      BitField<u16, bool, 3, 1> special_cycles;
      BitField<u16, bool, 4, 1> enable_memory_write_and_invalidate;
      BitField<u16, bool, 5, 1> vga_palette_snoop;
      BitField<u16, bool, 6, 1> parity_error_response;
      BitField<u16, bool, 8, 1> serr_enable;
      BitField<u16, bool, 9, 1> fast_back_to_back_enable;
      BitField<u16, bool, 10, 1> interrupt_disable;
      u16 bits;
    } command; // 0x04
    union
    {
      BitField<u16, bool, 3, 1> interrupt_status;
      BitField<u16, bool, 4, 1> capabilities_list;
      BitField<u16, bool, 5, 1> b66mhz_capable;
      BitField<u16, bool, 7, 1> fast_back_to_back_capable;
      BitField<u16, bool, 8, 1> master_data_parity_error;
      BitField<u16, u8, 9, 2> devsel_timing;
      BitField<u16, bool, 11, 1> signaled_target_abort;
      BitField<u16, bool, 12, 1> received_target_abort;
      BitField<u16, bool, 13, 1> received_master_abort;
      BitField<u16, bool, 14, 1> signaled_system_error;
      BitField<u16, bool, 15, 1> detected_parity_error;
      u16 bits;
    } status;                // 0x06
    u8 rev_id;               // 0x08
    u8 prog_if;              // 0x09
    u8 subclass_code;        // 0x0A
    u8 class_code;           // 0x0B
    u8 cache_line_size;      // 0x0C
    u8 latency_timer;        // 0x0D
    u8 header_type;          // 0x0E
    u8 bist;                 // 0x0F
    u32 bar0;                // 0x10
    u32 bar1;                // 0x14
    u32 bar2;                // 0x18
    u32 bar3;                // 0x1C
    u32 bar4;                // 0x20
    u32 bar5;                // 0x24
    u32 cardbus_cis_ptr;     // 0x28
    u16 subsystem_vendor_id; // 0x2C
    u16 subsystem_id;        // 0x2E
    u32 rom_base_address;    // 0x30
    u8 capabilities_ptr;     // 0x34
    u8 reserved1;            // 0x35
    u8 reserved2;            // 0x36
    u8 reserved3;            // 0x37
    u8 reserved4;            // 0x38
    u8 reserved5;            // 0x39
    u8 reserved6;            // 0x3A
    u8 reserved7;            // 0x3B
    u8 interrupt_line;       // 0x3C
    u8 interrupt_pin;        // 0x3D
    u8 min_grant;            // 0x3E
    u8 max_latency;          // 0x3F
  };

  struct ConfigSpaceData
  {
    union
    {
      u32 dwords[NUM_CONFIG_REGISTERS] = {};
      u16 words[NUM_CONFIG_REGISTERS * 2];
      u8 bytes[NUM_CONFIG_REGISTERS * 4];
      ConfigSpaceHeader header;
    };
    struct
    {
      PhysicalMemoryAddress default_address;
      u32 size;
      bool is_io;
      bool is_prefetchable;
    } memory_regions[NumMemoryRegions] = {};
  };

  void InitPCIID(u8 function, u16 vendor_id, u16 device_id);
  void InitPCIClass(u8 function, u8 class_code, u8 subclass_code, u8 prog_if, u8 rev_id);
  void InitPCIMemoryRegion(u8 function, MemoryRegion region, PhysicalMemoryAddress default_address, u32 size, bool io,
                           bool prefetchable = false);

  u8 GetConfigSpaceByte(u8 function, u8 byte_offset) const;
  u16 GetConfigSpaceWord(u8 function, u8 byte_offset) const;
  u32 GetConfigSpaceDWord(u8 function, u8 byte_offset) const;

  void SetConfigSpaceByte(u8 function, u8 byte_offset, u8 value);
  void SetConfigSpaceWord(u8 function, u8 byte_offset, u16 value);
  void SetConfigSpaceDWord(u8 function, u8 byte_offset, u32 value);

  PhysicalMemoryAddress GetMemoryRegionBaseAddress(u8 function, MemoryRegion region) const;

  // Memory region change notifications.
  virtual void OnCommandRegisterChanged(u8 function);
  virtual void OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active);

  u32 m_pci_bus_number = 0xFFFFFFFFu;
  u32 m_pci_device_number = 0xFFFFFFFFu;
  u8 m_num_functions = 0;

  std::vector<ConfigSpaceData> m_config_space;

private:
  static const u32 SERIALIZATION_ID = MakeSerializationID('P', 'C', 'I', '-');
};
