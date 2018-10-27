#include "system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "bus.h"
#include "component.h"
#include "cpu.h"
#include "save_state_version.h"
Log_SetChannel(System);

DEFINE_OBJECT_TYPE_INFO(System);
BEGIN_OBJECT_PROPERTY_MAP(System)
END_OBJECT_PROPERTY_MAP()

System::System(const ObjectTypeInfo* type_info /* = &s_type_info */) : BaseClass(type_info) {}

System::~System()
{
  // We should be stopped first.
  Assert(m_state == State::Initializing || m_state == State::Stopped);
  for (Component* component : m_components)
    delete component;

  delete m_bus;
}

void System::SetState(State state)
{
  m_state = state;
  if (state == State::Paused)
    m_cpu->StopExecution();
}

void System::AddComponent(Component* component)
{
  m_components.Add(component);
}

bool System::Initialize()
{
  Assert(m_state == State::Initializing);

  m_bus->Initialize(this);
  for (Component* component : m_components)
  {
    if (!component->Initialize(this, m_bus))
    {
      Log_ErrorPrintf("Component failed to initialize.");
      return false;
    }
  }

  return true;
}

void System::Reset()
{
  m_cpu->Reset();

  for (Component* component : m_components)
    component->Reset();
}

template<class Callback>
static bool LoadComponentStateHelper(BinaryReader& reader, Callback callback)
{
  uint32 component_state_size;
  if (!reader.SafeReadUInt32(&component_state_size))
    return false;

  uint64 expected_offset = reader.GetStreamPosition() + component_state_size;
  if (!callback())
    return false;

  if (reader.GetStreamPosition() != expected_offset)
  {
    Log_ErrorPrintf("Incorrect offset after reading component");
    return false;
  }

  if (reader.GetErrorState())
    return false;

  return true;
}

template<class Callback>
static bool SaveComponentStateHelper(BinaryWriter& writer, Callback callback)
{
  uint64 size_offset = writer.GetStreamPosition();

  // Reserve space for component size
  if (!writer.SafeWriteUInt32(0))
    return false;

  uint64 start_offset = writer.GetStreamPosition();
  if (!callback())
    return false;

  uint64 end_offset = writer.GetStreamPosition();
  uint32 component_size = Truncate32(end_offset - start_offset);
  if (!writer.SafeSeekAbsolute(size_offset) || !writer.SafeWriteUInt32(component_size) ||
      !writer.SafeSeekAbsolute(end_offset))
  {
    return false;
  }

  return true;
}

bool System::LoadState(BinaryReader& reader)
{
  uint32 signature, version;
  if (!reader.SafeReadUInt32(&signature) || !reader.SafeReadUInt32(&version))
    return false;

  if (signature != SAVE_STATE_SIGNATURE)
  {
    Log_ErrorPrintf("Incorrect save state signature");
    return false;
  }
  if (version != SAVE_STATE_VERSION)
  {
    Log_ErrorPrintf("Incorrect save state version");
    return false;
  }

  // Load system (this class) state
  if (!LoadComponentStateHelper(reader, [&]() { return LoadSystemState(reader); }))
    return false;

  // Load bus state next
  if (!LoadComponentStateHelper(reader, [&]() { return m_bus->LoadState(reader); }))
    return false;

  // And finally the components
  if (!LoadComponentsState(reader))
    return false;

  // Make sure we're not in an error state and failed a read somewhere
  if (reader.GetErrorState())
    return false;

  // Load clock events
  if (!m_timing_manager.LoadState(reader))
    return false;

  return !reader.GetErrorState();
}

bool System::SaveState(BinaryWriter& writer)
{
  if (!writer.SafeWriteUInt32(SAVE_STATE_SIGNATURE) || !writer.SafeWriteUInt32(SAVE_STATE_VERSION))
  {
    return false;
  }

  // Save system (this class) state
  if (!SaveComponentStateHelper(writer, [&]() { return SaveSystemState(writer); }))
    return false;

  // Save bus state next
  if (!SaveComponentStateHelper(writer, [&]() { return m_bus->SaveState(writer); }))
    return false;

  // And finally the components
  if (!SaveComponentsState(writer))
    return false;

  // Make sure we're not in an error state and failed a write somewhere
  if (writer.InErrorState())
    return false;

  // Write clock events
  if (!m_timing_manager.SaveState(writer))
    return false;

  return !writer.InErrorState();
}

bool System::LoadSystemState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  return true;
}

bool System::SaveSystemState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  return true;
}

bool System::LoadComponentsState(BinaryReader& reader)
{
  uint32 num_components = 0;
  if (!reader.SafeReadUInt32(&num_components) || num_components != m_components.GetSize())
  {
    Log_ErrorPrintf("Incorrect number of components");
    return false;
  }

  for (uint32 i = 0; i < num_components; i++)
  {
    Component* component = m_components[i];
    auto callback = [component, &reader]() { return component->LoadState(reader); };
    if (!LoadComponentStateHelper(reader, callback))
      return false;
  }

  return true;
}

bool System::SaveComponentsState(BinaryWriter& writer)
{
  writer.WriteUInt32(Truncate32(m_components.GetSize()));

  for (uint32 i = 0; i < m_components.GetSize(); i++)
  {
    Component* component = m_components[i];
    auto callback = [component, &writer]() { return component->SaveState(writer); };
    if (!SaveComponentStateHelper(writer, callback))
      return false;
  }

  return true;
}

SimulationTime System::ExecuteSlice(SimulationTime time)
{
  const SimulationTime start_timestamp = m_timing_manager.GetTotalEmulatedTime();

  // Convert time into CPU cycles, since that drives things currently.
  const CycleCount slice_cycles = CycleCount(time / m_cpu->GetCyclePeriod());

  // CPU will call back to us to run components.
  if (slice_cycles > 0)
    m_cpu->ExecuteSlice(slice_cycles);

  return m_timing_manager.GetEmulatedTimeDifference(start_timestamp);
}

std::pair<std::unique_ptr<byte[]>, uint32> System::ReadFileToBuffer(const char* filename, u32 offset, u32 expected_size)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open ROM file: %s", filename);
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  const uint32 size = Truncate32(stream->GetSize());
  if (expected_size != 0 && (offset >= size || (size - offset) != expected_size))
  {
    Log_ErrorPrintf("ROM file %s mismatch - expected %u bytes, got %u bytes", filename, expected_size, size);
    stream->Release();
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  std::unique_ptr<byte[]> data = std::make_unique<byte[]>(size - offset);
  if ((offset > 0 && !stream->SeekAbsolute(offset)) || !stream->Read2(data.get(), size - offset))
  {
    Log_ErrorPrintf("Failed to read %u bytes from ROM file %s", size - offset, filename);
    stream->Release();
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  stream->Release();
  return std::make_pair(std::move(data), size - offset);
}

String System::GetMiscDataFilename(const char* suffix) const
{
  return suffix ? String::FromFormat("%s%s", m_base_path.GetCharArray(), suffix) : m_base_path;
}
