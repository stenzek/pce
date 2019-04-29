#pragma once
#include "YBaseLib/String.h"
#include "pce/cpu.h"
#include "pce/debugger_interface.h"
#include <array>

namespace CPUDebugger {

class RegisterWindow
{
public:
  RegisterWindow(DebuggerInterface* intf);
  ~RegisterWindow();

  void Update();
  void Draw();

private:
  const u32 INVALID_REGISTER_INDEX = static_cast<u32>(-1);

  struct Register
  {
    TinyString name;
    TinyString string_value;
    DebuggerInterface::RegisterType reg_type;
    DebuggerInterface::RegisterValue reg_value;
    bool changed;
  };

  void SetStringValues(bool check_for_change);
  void UpdateRegister(u32 index, const char* value);

  DebuggerInterface* m_interface;
  std::vector<Register> m_registers;
  u32 m_selected_index = INVALID_REGISTER_INDEX;
  u32 m_edit_index = INVALID_REGISTER_INDEX;
  bool m_hex_display = true;
  bool m_window_open = true;
  StackString<128> m_edit_value;
};

class CodeWindow
{
public:
  CodeWindow(DebuggerInterface* intf);
  ~CodeWindow();

  void Update();
  void Draw();

private:
  static const LinearMemoryAddress MAX_INSTRUCTION_SIZE = 16;
  static constexpr u32 DEFAULT_LINES_BEFORE = 100;
  static constexpr u32 DEFAULT_LINES_AFTER = 100;

  struct Line
  {
    LinearMemoryAddress instruction_pointer;
    LinearMemoryAddress linear_address;
    PhysicalMemoryAddress physical_address;
    u32 instruction_size;
    TinyString address_string;
    TinyString instruction_bytes;
    TinyString instruction_disassembly;
  };

  u32 GetLineForIP() const;
  void RefreshLines();
  bool GetInstruction(Line& line, LinearMemoryAddress ip);
  void AddInstructionsBeforeIP(LinearMemoryAddress ip, u32 max);
  void AddInstructionsAfterIP(LinearMemoryAddress ip, u32 max);

  std::vector<Line> m_lines;

  DebuggerInterface* m_interface;
  bool m_window_open = true;
  bool m_lines_refreshed = false;
};

class StackWindow
{
public:
  StackWindow(DebuggerInterface* intf);
  ~StackWindow();

  void Update();
  void Draw();

private:
  const u32 INVALID_REGISTER_INDEX = static_cast<u32>(-1);

  struct Register
  {
    TinyString name;
    TinyString string_value;
    DebuggerInterface::RegisterType reg_type;
    DebuggerInterface::RegisterValue reg_value;
    bool changed;
  };

  void SetStringValues(bool check_for_change);
  void UpdateRegister(u32 index, const char* value);

  DebuggerInterface* m_interface;
  std::vector<Register> m_registers;
  u32 m_selected_index = INVALID_REGISTER_INDEX;
  u32 m_edit_index = INVALID_REGISTER_INDEX;
  bool m_hex_display = true;
  bool m_window_open = true;
  StackString<128> m_edit_value;
};

class CPUDebugger
{
public:
  CPUDebugger(CPU* cpu, DebuggerInterface* intf);
  ~CPUDebugger();

  void Update();

  void Draw();

private:
  CPU* m_cpu;
  DebuggerInterface* m_interface;

  RegisterWindow m_register_window;
  CodeWindow m_code_window;
};
} // namespace CPUDebugger