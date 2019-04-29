#include "cpu_debugger.h"
#include "imgui.h"
#include "pce/cpu.h"
#include "pce/debugger_interface.h"
#include <cinttypes>

namespace CPUDebugger {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const ImVec4 COLOR_WHITE{1.0f, 1.0f, 1.0f, 1.0f};
static const ImVec4 COLOR_LIGHT_GREY{0.6f, 0.6f, 0.6f, 1.0f};
static const ImVec4 COLOR_LIGHT_RED{0.85f, 0.4f, 0.4f, 1.0f};

CPUDebugger::CPUDebugger(CPU* cpu, DebuggerInterface* intf)
  : m_cpu(cpu), m_interface(intf), m_register_window(intf), m_code_window(intf)
{
}

CPUDebugger::~CPUDebugger() = default;

void CPUDebugger::Update()
{
  m_register_window.Update();
  m_code_window.Update();
}

void CPUDebugger::Draw()
{
  m_register_window.Draw();
  m_code_window.Draw();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Register Window
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RegisterWindow::RegisterWindow(DebuggerInterface* intf) : m_interface(intf) {}

RegisterWindow::~RegisterWindow() = default;

void RegisterWindow::Update()
{
  const u32 reg_count = m_interface->GetRegisterCount();
  const bool is_init = m_registers.empty();
  m_registers.resize(reg_count);

  for (u32 i = 0; i < reg_count; i++)
  {
    Register& reg = m_registers[i];
    reg.name.Assign(m_interface->GetRegisterName(i));
    reg.reg_type = m_interface->GetRegisterType(i);
    reg.reg_value = m_interface->GetRegisterValue(i);
    reg.changed = false;
  }

  SetStringValues(!is_init);

  if (m_selected_index >= m_registers.size())
    m_selected_index = INVALID_REGISTER_INDEX;
  if (m_edit_index >= m_registers.size())
    m_edit_index = INVALID_REGISTER_INDEX;
}

void RegisterWindow::SetStringValues(bool check_for_change)
{
  for (Register& reg : m_registers)
  {
    TinyString string_value;
    switch (reg.reg_type)
    {
      case DebuggerInterface::RegisterType::Byte:
        string_value.Format(m_hex_display ? "0x%02x" : "%u", ZeroExtend32(reg.reg_value.val_byte));
        break;
      case DebuggerInterface::RegisterType::Word:
        string_value.Format(m_hex_display ? "0x%04x" : "%u", ZeroExtend32(reg.reg_value.val_word));
        break;
      case DebuggerInterface::RegisterType::DWord:
        string_value.Format(m_hex_display ? "0x%08x" : "%u", reg.reg_value.val_dword);
        break;
      case DebuggerInterface::RegisterType::QWord:
        string_value.Format(m_hex_display ? "0x016" PRIx64 : PRIu64, reg.reg_value.val_qword);
        break;
    }
    if (check_for_change)
      reg.changed = reg.string_value != string_value;

    reg.string_value.AssignCopy(string_value);
  }
}

void RegisterWindow::UpdateRegister(u32 index, const char* value)
{
  s64 ivalue;
  if (value[0] == '0' && value[1] == 'x')
    ivalue = std::strtoull(value, nullptr, 16);
  else
    ivalue = std::strtol(value, nullptr, 10);

  Register& reg = m_registers[index];
  DebuggerInterface::RegisterValue new_value = {};
  switch (reg.reg_type)
  {
    case DebuggerInterface::RegisterType::Byte:
      new_value.val_byte = Truncate8(static_cast<u64>(ivalue));
      break;
    case DebuggerInterface::RegisterType::Word:
      new_value.val_word = Truncate16(static_cast<u64>(ivalue));
      break;
    case DebuggerInterface::RegisterType::DWord:
      new_value.val_dword = Truncate32(static_cast<u64>(ivalue));
      break;
    case DebuggerInterface::RegisterType::QWord:
      new_value.val_qword = static_cast<u64>(ivalue);
      break;
  }

  if (new_value.val_qword != reg.reg_value.val_qword)
  {
    m_interface->SetRegisterValue(index, new_value);
    reg.reg_value = new_value;
    reg.changed = true;
  }
}

void RegisterWindow::Draw()
{
  if (!ImGui::Begin("Registers", &m_window_open))
  {
    ImGui::End();
    return;
  }

  ImGui::Columns(2, "#debugger_regs");
  ImGui::Separator();
  ImGui::TextColored(COLOR_WHITE, "Register");
  ImGui::NextColumn();
  ImGui::TextColored(COLOR_WHITE, "Value");
  ImGui::NextColumn();
  ImGui::Separator();

  u32 reg_index = 0;
  for (const Register& reg : m_registers)
  {
    const bool is_selected = m_selected_index == reg_index;
    const bool is_editing = m_edit_index == reg_index;
    if (ImGui::Selectable(reg.name.GetCharArray(), is_selected,
                          (is_editing ? 0 : ImGuiSelectableFlags_SpanAllColumns) |
                            ImGuiSelectableFlags_AllowDoubleClick))
    {
      // these take effect *next* frame
      m_selected_index = reg_index;
      if (ImGui::IsMouseDoubleClicked(0))
      {
        m_edit_index = reg_index;
        m_edit_value.AssignCopy(reg.string_value.GetCharArray());
      }
      else if (m_edit_index != INVALID_REGISTER_INDEX)
      {
        // deselected
        m_edit_value.UpdateSize();
        if (!m_edit_value.IsEmpty())
          UpdateRegister(m_edit_index, m_edit_value);
        m_edit_index = INVALID_REGISTER_INDEX;
        m_edit_value.Clear();
      }
    }

    ImGui::NextColumn();

    if (is_editing)
    {
      ImGui::SetNextItemWidth(80.0f);
      if (ImGui::InputText("##debugger_reg_edit", m_edit_value.GetWriteableCharArray(),
                           m_edit_value.GetWritableBufferSize(),
                           ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
      {
        m_edit_value.UpdateSize();
        if (!m_edit_value.IsEmpty())
          UpdateRegister(m_edit_index, m_edit_value);

        m_edit_index = INVALID_REGISTER_INDEX;
        m_edit_value.Clear();
      }
    }
    else
    {
      ImGui::TextColored(reg.changed ? COLOR_LIGHT_RED : COLOR_LIGHT_GREY, "%s", reg.string_value.GetCharArray());
    }
    ImGui::NextColumn();
    reg_index++;
  }

  ImGui::Separator();
  ImGui::Columns(1);

  if (ImGui::Checkbox("Hexadecimal Display", &m_hex_display))
    SetStringValues(false);

  ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Code Window
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CodeWindow::CodeWindow(DebuggerInterface* intf) : m_interface(intf) {}

CodeWindow::~CodeWindow() = default;

void CodeWindow::Update()
{
  if (GetLineForIP() == m_lines.size())
    RefreshLines();
}

void CodeWindow::Draw()
{
  ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Code", &m_window_open, 0))
  {
    ImGui::End();
    return;
  }

  ImGui::Columns(4);
  ImGui::SetColumnWidth(0, 80.0f);
  ImGui::SetColumnWidth(1, 120.0f);
  ImGui::SetColumnWidth(2, 150.0f);

  ImGui::TextColored(COLOR_WHITE, "Linear");
  ImGui::NextColumn();
  ImGui::TextColored(COLOR_WHITE, "Address");
  ImGui::NextColumn();
  ImGui::TextColored(COLOR_WHITE, "Bytes");
  ImGui::NextColumn();
  ImGui::TextColored(COLOR_WHITE, "Disassembly");
  ImGui::NextColumn();
  ImGui::Separator();
  ImGui::Columns(1);

  if (ImGui::BeginChild("##debugger_code_scroll"))
  {
    const u32 ip_line = GetLineForIP();
    if (m_lines_refreshed)
    {
      ImGui::SetScrollFromPosY(ip_line * ImGui::GetTextLineHeightWithSpacing());
      m_lines_refreshed = false;
    }

#if 0
    if (!m_lines.empty())
    {
      if (ImGui::GetScrollY() <= 10.0f)
      {
        // add some more at the top
        AddInstructionsBeforeIP(m_lines[0].linear_address, DEFAULT_LINES_BEFORE);
      }
    }
#endif

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.5f, 0.1f, 1.0f));

    ImGuiListClipper clipper(static_cast<u32>(m_lines.size()));
    while (clipper.Step())
    {
      for (u32 line = static_cast<u32>(clipper.DisplayStart); line < static_cast<u32>(clipper.DisplayEnd); line++)
      {
        if (line >= m_lines.size())
          break;

        const Line& data = m_lines[line];
        ImGui::Columns(4);
        ImGui::SetColumnWidth(0, 80.0f);
        ImGui::SetColumnWidth(1, 120.0f);
        ImGui::SetColumnWidth(2, 150.0f);

        auto label = TinyString::FromFormat("%08X##debugger_code_line_%d", data.linear_address, line);
        ImGui::Selectable(label, line == ip_line, ImGuiSelectableFlags_SpanAllColumns);
        ImGui::NextColumn();

        ImGui::TextUnformatted(data.address_string);
        ImGui::NextColumn();
        ImGui::TextUnformatted(data.instruction_bytes);
        ImGui::NextColumn();
        ImGui::TextUnformatted(data.instruction_disassembly);
        ImGui::NextColumn();
        ImGui::Columns(1);
      }
    }

    ImGui::PopStyleColor();
  }
  ImGui::EndChild();

  ImGui::End();
}

u32 CodeWindow::GetLineForIP() const
{
  const LinearMemoryAddress ip = m_interface->GetInstructionPointer();
  for (u32 i = 0; i < static_cast<u32>(m_lines.size()); i++)
  {
    if (m_lines[i].instruction_pointer == ip)
      return i;
  }

  return static_cast<u32>(m_lines.size());
}

void CodeWindow::RefreshLines()
{
  m_lines.clear();
  m_lines_refreshed = true;

  LinearMemoryAddress current_ip = m_interface->GetInstructionPointer();
  AddInstructionsBeforeIP(current_ip, DEFAULT_LINES_BEFORE);
  AddInstructionsAfterIP(current_ip, DEFAULT_LINES_AFTER);
}

bool CodeWindow::GetInstruction(Line& line, LinearMemoryAddress ip)
{
  line.instruction_pointer = ip;
  if (!m_interface->DisassembleCode(ip, &line.linear_address, &line.address_string, &line.instruction_bytes,
                                    &line.instruction_disassembly, &line.instruction_size))
    return false;

  line.physical_address = line.linear_address;
  return true;
}

void CodeWindow::AddInstructionsBeforeIP(LinearMemoryAddress ip, u32 max)
{
  // we have to insert these in reverse order, so buffer them
  std::vector<Line> instructions_before;
  for (u32 i = 0; i < max && ip > 0; i++)
  {
    // find the previous instruction IP which lines up
    LinearMemoryAddress previous_ip = ip - 1;
    u32 previous_ip_size = 0;
    bool instruction_start_valid = false;
    while ((ip - previous_ip) < MAX_INSTRUCTION_SIZE)
    {
      if (m_interface->DisassembleCode(previous_ip, nullptr, nullptr, nullptr, nullptr, &previous_ip_size) &&
          (previous_ip + previous_ip_size) == ip)
      {
        // found it
        instruction_start_valid = true;
        break;
      }

      // don't wrap around..
      if (previous_ip > 0)
        previous_ip--;
      else
        break;
    }

    if (!instruction_start_valid)
      break;

    Line line;
    if (!GetInstruction(line, previous_ip))
      break;

    instructions_before.push_back(line);
    ip = previous_ip;
  }

  m_lines.insert(m_lines.begin(), instructions_before.rbegin(), instructions_before.rend());
}

void CodeWindow::AddInstructionsAfterIP(LinearMemoryAddress ip, u32 max)
{
  u32 lines_after_count = 0;
  for (u32 i = 0; i < max; i++)
  {
    Line line;
    if (!GetInstruction(line, ip))
      break;

    m_lines.push_back(line);

    // don't wrap around
    if ((ip + line.instruction_size) < ip)
      break;

    ip += line.instruction_size;
    lines_after_count++;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CPUDebugger