#include "pce/bus.h"
#include "pce/mmio.h"

template<typename T>
// #ifdef Y_COMPILER_MSVC
// __forceinline
// #elif Y_COMPILER_GCC || Y_COMPILER_CLANG
// __attribute__((always_inline))
// #endif
T Bus::ReadMemoryTyped(PhysicalMemoryAddress address)
{
  T value;
  address &= m_physical_memory_address_mask;

  // Since we allocate the page array based on the address mask, this should never overflow.
  const u32 page_number = address >> MEMORY_PAGE_NUMBER_SHIFT;
  const u32 page_offset = address & MEMORY_PAGE_OFFSET_MASK;
  DebugAssert(page_number < m_num_physical_memory_pages);

  // Fast path - page is RAM.
  const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  if (page.type & PhysicalMemoryPage::kReadableRAM)
  {
    std::memcpy(&value, page.ram_ptr + page_offset, sizeof(T));
  }
  // Slow path - page is MMIO.
  else if (page.mmio_handler && address >= page.mmio_handler->GetStartAddress() &&
           (address + sizeof(T) - 1) <= page.mmio_handler->GetEndAddress())
  {

    // Pass to MMIO
    if constexpr (std::is_same<T, u8>::value)
      value = page.mmio_handler->ReadByte(address);
    else if constexpr (std::is_same<T, u16>::value)
      value = page.mmio_handler->ReadWord(address);
    else if constexpr (std::is_same<T, u32>::value)
      value = page.mmio_handler->ReadDWord(address);
    else if constexpr (std::is_same<T, u64>::value)
      value = page.mmio_handler->ReadQWord(address);
    else
      value = static_cast<T>(-1);
  }
  else
  {
    value = static_cast<T>(-1);
  }

#if defined(Y_BUILD_CONFIG_DEBUG) || defined(Y_BUILD_CONFIG_DEBUGFAST)
  CheckForMemoryBreakpoint(address, sizeof(T), false, static_cast<u32>(value));
#endif

  return value;
}

template<typename T>
// #ifdef Y_COMPILER_MSVC
// __forceinline
// #elif Y_COMPILER_GCC || Y_COMPILER_CLANG
// __attribute__((always_inline))
// #endif
void Bus::WriteMemoryTyped(PhysicalMemoryAddress address, T value)
{
  address &= m_physical_memory_address_mask;

#if defined(Y_BUILD_CONFIG_DEBUG) || defined(Y_BUILD_CONFIG_DEBUGFAST)
  CheckForMemoryBreakpoint(address, sizeof(T), true, static_cast<u32>(value));
#endif

  // Since we allocate the page array based on the address mask, this should never overflow.
  const u32 page_number = address >> MEMORY_PAGE_NUMBER_SHIFT;
  const u32 page_offset = address & MEMORY_PAGE_OFFSET_MASK;
  DebugAssert(page_number < m_num_physical_memory_pages);

  // Fast path - page is RAM.
  PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  if (page.type & PhysicalMemoryPage::kWritableRAM)
  {
    if (!(page.type & PhysicalMemoryPage::kCachedCode))
    {
      std::memcpy(page.ram_ptr + page_offset, &value, sizeof(value));
      return;
    }

    if (std::memcmp(page.ram_ptr + page_offset, &value, sizeof(value)) == 0)
    {
      // Not modified, so don't fire the callback.
      return;
    }

    // Copy value in and fire callback.
    std::memcpy(page.ram_ptr + page_offset, &value, sizeof(value));
    m_code_invalidate_callback(address & MEMORY_PAGE_MASK);
    return;
  }

  // Slow path - page is MMIO.
  if (page.mmio_handler && address >= page.mmio_handler->GetStartAddress() &&
      (address + sizeof(value) - 1) <= page.mmio_handler->GetEndAddress())
  {
    // Pass to MMIO
    if constexpr (std::is_same<T, u8>::value)
      page.mmio_handler->WriteByte(address, static_cast<u8>(value));
    else if constexpr (std::is_same<T, u16>::value)
      page.mmio_handler->WriteWord(address, static_cast<u16>(value));
    else if constexpr (std::is_same<T, u32>::value)
      page.mmio_handler->WriteDWord(address, static_cast<u32>(value));
    else if constexpr (std::is_same<T, u64>::value)
      page.mmio_handler->WriteQWord(address, static_cast<u64>(value));

    return;
  }
}
