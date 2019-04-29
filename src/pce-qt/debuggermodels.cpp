#include "pce-qt/debuggermodels.h"
#include "pce/debugger_interface.h"
#include <QtGui/QColor>

const int MAX_CODE_ROWS = 1024;
const LinearMemoryAddress MAX_CODE_DISTANCE = 4096;
const int NUM_COLUMNS = 3;

DebuggerCodeModel::DebuggerCodeModel(DebuggerInterface* intf, QObject* parent /*= nullptr*/)
  : QAbstractTableModel(parent), m_interface(intf)

{
  resetCodeView(0);
}

DebuggerCodeModel::~DebuggerCodeModel() {}

int DebuggerCodeModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  // TODO: Don't make this fixed..
  return MAX_CODE_ROWS;
}

int DebuggerCodeModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return NUM_COLUMNS;
}

QVariant DebuggerCodeModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
  u32 reg_index = static_cast<u32>(index.row());
  if (index.column() < 0 || index.column() >= NUM_COLUMNS)
    return QVariant();

  if (role == Qt::DisplayRole)
  {
    LinearMemoryAddress address;
    if (!getAddressForRow(&address, index.row()))
      return "<invalid>";

    switch (index.column())
    {
      case 0:
      {
        // Address
        return QVariant(QString::asprintf("0x%08X", address));
      }

      case 1:
      {
        // Bytes
        u32 instruction_length;
        if (!m_interface->DisassembleCode(address, nullptr, nullptr, nullptr, nullptr, &instruction_length))
          return "<invalid>";

        SmallString value;
        for (u32 i = 0; i < instruction_length; i++)
        {
          u8 byte_value;
          if (!m_interface->ReadMemoryByte(address + i, &byte_value))
            break;
          value.AppendFormattedString("%s%02X", (i == 0) ? "" : " ", ZeroExtend32(byte_value));
        }
        return value.GetCharArray();
      }

      case 2:
      {
        // Instruction
        String instruction;
        if (!m_interface->DisassembleCode(address, nullptr, nullptr, nullptr, &instruction, nullptr))
          return "<invalid>";

        return instruction.GetCharArray();
      }

      default:
        return QVariant();
    }
  }
  else if (role == Qt::BackgroundColorRole)
  {
    int row = index.row();
    LinearMemoryAddress address;
    if (!getAddressForRow(&address, row))
      return QVariant();

    // breakpoint
    // return QVariant(QColor(171, 97, 107));
    if (address == m_last_instruction_pointer)
      return QVariant(QColor(255, 241, 129));
    else
      return QVariant();
  }
  else
  {
    return QVariant();
  }
}

QVariant DebuggerCodeModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (orientation != Qt::Horizontal)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  static const char* header_names[] = {"Address", "Bytes", "Instruction"};
  if (section < 0 || section >= countof(header_names))
    return QVariant();

  return header_names[section];
}

void DebuggerCodeModel::resetCodeView(LinearMemoryAddress start_address)
{
  beginResetModel();
  m_start_instruction_pointer = start_address;
  m_row_ips.clear();
  m_row_ips[0] = start_address;
  endResetModel();
}

int DebuggerCodeModel::getRowForAddress(LinearMemoryAddress address)
{
  // We can move forward, but not backwards
  if (address < m_start_instruction_pointer)
  {
    resetCodeView(address);
    return 0;
  }

  // Find row with matching address
  int row = 0;
  auto iter = m_row_ips.begin();
  for (; iter != m_row_ips.end(); iter++)
  {
    // Do we already have this row cached?
    if (iter->second == address)
      return iter->first;

    // Keep moving forward while we are ahead of the current cached instruction
    if (address > iter->second)
    {
      row = iter->first;
      continue;
    }

    // If this row is behind the current instruction, use the last row instead
    break;
  }

  // Add row entries until we hit the instruction, or go past it
  iter = m_row_ips.find(row);
  DebugAssert(iter != m_row_ips.end());
  LinearMemoryAddress current_address = iter->second;
  while (current_address < address)
  {
    // Make sure we don't try to create too many rows
    if (row > MAX_CODE_ROWS)
    {
      resetCodeView(address);
      return 0;
    }

    // Get the instruction length
    u32 instruction_size;
    if (!m_interface->DisassembleCode(current_address, nullptr, nullptr, nullptr, nullptr, &instruction_size))
    {
      resetCodeView(address);
      return 0;
    }

    row++;
    current_address += instruction_size;
    m_row_ips[row] = current_address;
    if (current_address == address)
    {
      // Found the row
      return row;
    }
  }

  // Somehow we skipped over the address. This can happen when we jump to the middle of an instruction.
  resetCodeView(address);
  return 0;
}

bool DebuggerCodeModel::getAddressForRow(LinearMemoryAddress* address, int row) const
{
  Assert(row >= 0);
  auto iter = m_row_ips.find(row);
  if (iter != m_row_ips.end())
  {
    *address = iter->second;
    return true;
  }

  // iter = m_row_ips.lower_bound(row);
  iter = m_row_ips.begin();
  while (iter != m_row_ips.end() && iter->first < row)
  {
    auto prev_iter = iter++;
    if (iter == m_row_ips.end())
    {
      iter = prev_iter;
      break;
    }
  }
  if (iter == m_row_ips.end())
    return false;

  // Work forward until we match the row
  int last_row = iter->first;
  LinearMemoryAddress last_address = iter->second;
  while (last_row < row)
  {
    // Get the instruction length
    u32 instruction_size;
    if (!m_interface->DisassembleCode(last_address, nullptr, nullptr, nullptr, nullptr, &instruction_size))
      return false;

    last_address += static_cast<LinearMemoryAddress>(instruction_size);
    last_row++;
    m_row_ips[last_row] = last_address;
  }

  *address = last_address;
  return true;
}

int DebuggerCodeModel::updateInstructionPointer()
{
  LinearMemoryAddress ip = m_interface->GetInstructionPointer();
  int new_row = getRowForAddress(ip);
  if (m_last_instruction_pointer == ip)
    return new_row;

  m_last_instruction_pointer = ip;

  int old_row = getRowForAddress(m_last_instruction_pointer);
  if (old_row >= 0)
    emit dataChanged(index(old_row, 0), index(old_row, NUM_COLUMNS - 1));

  if (new_row >= 0)
    emit dataChanged(index(new_row, 0), index(new_row, NUM_COLUMNS - 1));

  return new_row;
}

DebuggerRegistersModel::DebuggerRegistersModel(DebuggerInterface* intf, QObject* parent /*= nullptr*/)
  : QAbstractListModel(parent), m_interface(intf)
{
}

DebuggerRegistersModel::~DebuggerRegistersModel() {}

int DebuggerRegistersModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return static_cast<int>(m_interface->GetRegisterCount());
}

int DebuggerRegistersModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant DebuggerRegistersModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
  u32 reg_index = static_cast<u32>(index.row());
  if (reg_index > m_interface->GetRegisterCount())
    return QVariant();

  if (index.column() < 0 || index.column() > 1)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  if (index.column() == 0)
  {
    return m_interface->GetRegisterName(reg_index);
  }
  else
  {
    DebuggerInterface::RegisterType type = m_interface->GetRegisterType(reg_index);
    DebuggerInterface::RegisterValue value = m_interface->GetRegisterValue(reg_index);
    switch (type)
    {
      case DebuggerInterface::RegisterType::Byte:
        return QString::asprintf("0x%02X", ZeroExtend32(value.val_byte));
      case DebuggerInterface::RegisterType::Word:
        return QString::asprintf("0x%04X", ZeroExtend32(value.val_word));
      case DebuggerInterface::RegisterType::DWord:
        return QString::asprintf("0x%08X", value.val_dword);
      default:
        return QString();
    }
  }
}

QVariant DebuggerRegistersModel::headerData(int section, Qt::Orientation orientation,
                                            int role /*= Qt::DisplayRole*/) const
{
  if (orientation != Qt::Horizontal)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  static const char* header_names[] = {"Register", "Value"};
  if (section < 0 || section >= countof(header_names))
    return QVariant();

  return header_names[section];
}

void DebuggerRegistersModel::invalidateView()
{
  beginResetModel();
  endResetModel();
}

DebuggerStackModel::DebuggerStackModel(DebuggerInterface* intf, QObject* parent /*= nullptr*/)
  : QAbstractListModel(parent), m_interface(intf)
{
}

DebuggerStackModel::~DebuggerStackModel() {}

int DebuggerStackModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  LinearMemoryAddress stack_bottom = m_interface->GetStackBottom();
  LinearMemoryAddress stack_top = m_interface->GetStackTop();
  if (stack_bottom < stack_top)
    return 0;

  DebuggerInterface::RegisterType width = m_interface->GetStackValueType();
  LinearMemoryAddress value_size = (width == DebuggerInterface::RegisterType::Word) ? 2 : 4;
  int num_rows = static_cast<int>((stack_bottom - stack_top) / value_size) + 1;
  return std::min(num_rows, 1024);
}

int DebuggerStackModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant DebuggerStackModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
  if (index.column() < 0 || index.column() > 1)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  DebuggerInterface::RegisterType width = m_interface->GetStackValueType();
  LinearMemoryAddress value_size = (width == DebuggerInterface::RegisterType::Word) ? 2 : 4;
  LinearMemoryAddress address = m_interface->GetStackTop() + static_cast<LinearMemoryAddress>(index.row()) * value_size;

  if (index.column() == 0)
    return QString::asprintf("0x%08X", address);

  if (width == DebuggerInterface::RegisterType::Word)
  {
    u16 value;
    if (!m_interface->ReadMemoryWord(address, &value))
      return QVariant();
    return QString::asprintf("0x%04X", ZeroExtend32(value));
  }
  else
  {
    u32 value;
    if (!m_interface->ReadMemoryDWord(address, &value))
      return QVariant();
    return QString::asprintf("0x%08X", ZeroExtend32(value));
  }
}

QVariant DebuggerStackModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (orientation != Qt::Horizontal)
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  static const char* header_names[] = {"Address", "Value"};
  if (section < 0 || section >= countof(header_names))
    return QVariant();

  return header_names[section];
}

void DebuggerStackModel::invalidateView()
{
  beginResetModel();
  endResetModel();
}
