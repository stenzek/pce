#pragma once

#include "YBaseLib/TaskQueue.h"
#include "common/audio.h"
#include "common/clock.h"
#include "pce/component.h"
#include "pce/system.h"
#include <array>

#define YMF262_USE_THREAD 1

namespace DBOPL {
struct Chip;
}

namespace HW {

class YMF262 final
{
public:
  static constexpr float OUTPUT_FREQUENCY = 44100.0f;
  static constexpr float GAIN = 4.0f; // in dB

  enum class Mode : u32
  {
    OPL2,
    DualOPL2,
    OPL3
  };

public:
  YMF262(Mode mode, const char* clock_prefix = "");
  ~YMF262();

  bool Initialize(System* system);
  bool LoadState(BinaryReader& reader);
  bool SaveState(BinaryWriter& writer) const;
  void Reset();

  u8 ReadAddressPort(size_t chip_index);
  u8 ReadDataPort(size_t chip_index);
  void WriteAddressPort(size_t chip_index, u8 value);
  void WriteDataPort(size_t chip_index, u8 value);
  void DualWriteAddressPort(u8 value);
  void DualWriteDataPort(u8 value);

  void SetVolume(size_t chip_index, float volume);

private:
  static constexpr u32 SERIALIZATION_ID = Component::MakeSerializationID('Y', '2', '6', '2');

  static constexpr float TIMER1_FREQUENCY = 12500.0f; // 80 microseconds
  static constexpr float TIMER2_FREQUENCY = 3125.0f;  // 320 microseconds
  static constexpr size_t NUM_TIMERS = 2;             // 2 timers per chip in dual mode

  struct ChipState
  {
    std::unique_ptr<DBOPL::Chip> dbopl;
    std::vector<s32> temp_buffer;
    u32 address_register = 0;
    float volume = 1.0f;

    struct Timer
    {
      u16 reload_value = 0;
      u16 value = 0;
      bool masked = true;
      bool expired = false;
      bool active = false;
      TimingEvent::Pointer event;
    };

    std::array<Timer, NUM_TIMERS> timers = {};
  };

  bool IsStereo() const { return m_mode != Mode::OPL2; }

  bool LoadChipState(size_t chip_index, BinaryReader& reader);
  bool SaveChipState(size_t chip_index, BinaryWriter& writer) const;

  void RenderSampleEvent(CycleCount cycles);
  void RenderSamples(u32 count);

  void TimerExpiredEvent(size_t chip_index, size_t timer_index, CycleCount cycles);
  void UpdateTimerEvent(size_t chip_index, size_t timer_index);
  void FlushWorkerThread() const;

  Clock m_clock;
  Mode m_mode = Mode::OPL2;

  System* m_system = nullptr;
  Audio::Channel* m_output_channel = nullptr;
  TimingEvent::Pointer m_render_sample_event;

  std::array<ChipState, 2> m_chips;
  size_t m_num_chips = 0;

#ifdef YMF262_USE_THREAD
  mutable TaskQueue m_worker_thread;
#endif
};

} // namespace HW