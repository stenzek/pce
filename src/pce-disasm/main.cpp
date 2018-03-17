#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/String.h"
#include "YBaseLib/StringConverter.h"
#include "pce/cpu_x86/decode.h"
#include <cstdio>
#include <cstring>
Log_SetChannel(Main);

struct ArrayFetchInstructionByteCallback : CPU_X86::InstructionFetchCallback
{
  ArrayFetchInstructionByteCallback(uint8* data, size_t offset, size_t length)
    : m_data(data), m_offset(offset), m_length(length)
  {
  }

  virtual uint8 FetchByte() override
  {
    if (m_offset < m_length)
      return m_data[m_offset++];
    else
      return 0;
  }

  uint8* m_data;
  size_t m_offset;
  size_t m_length;
};

static bool ReadFileToArray(PODArray<byte>* dest_array, const char* filename)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", filename);
    return false;
  }

  dest_array->Resize(static_cast<uint32>(stream->GetSize()));
  if (!stream->Read2(dest_array->GetBasePointer(), dest_array->GetSize()))
  {
    Log_ErrorPrintf("Failed to read code file %s", filename);
    stream->Release();
    return false;
  }
  stream->Release();

  return true;
}

// Disgustingly lazy
String input_file_name;
PODArray<byte> input_code;
CPU_X86::AddressSize address_size = CPU_X86::AddressSize_16;
CPU_X86::OperandSize operand_size = CPU_X86::OperandSize_16;
uint32 origin_address = 0;

static bool ParseArguments(int argc, char* argv[])
{
#define CHECK_ARG(str) !std::strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !std::strcmp(argv[i], str) && ((i + 1) < argc)

  for (int i = 1; i < argc; i++)
  {
    if (CHECK_ARG_PARAM("-b"))
    {
      uint32 bit_width = StringConverter::StringToUInt32(argv[++i]);
      if (bit_width == 16)
      {
        address_size = CPU_X86::AddressSize_16;
        operand_size = CPU_X86::OperandSize_16;
      }
      else if (bit_width == 32)
      {
        address_size = CPU_X86::AddressSize_32;
        operand_size = CPU_X86::OperandSize_32;
      }
      else
      {
        Log_ErrorPrintf("Invalid bit width: %u", bit_width);
        return false;
      }
    }
    else if (CHECK_ARG_PARAM("-o"))
    {
      const char* arg = argv[++i];
      if (arg[0] == '0' && arg[1] == 'x')
        origin_address = Truncate32(strtol(arg, nullptr, 10));
      else
        origin_address = Truncate32(strtol(arg, nullptr, 16));
    }
    else if (CHECK_ARG_PARAM("-hex"))
    {
      const char* data = argv[++i];
      uint32 length = Truncate32(std::strlen(data));
      input_code.Resize(length / 2 + 1);
      length = StringConverter::HexStringToBytes(input_code.GetBasePointer(), input_code.GetSize(), data);
      input_code.Resize(length);
      if (length == 0)
      {
        Log_ErrorPrintf("Failed to parse hex string");
        return false;
      }

      Log_InfoPrintf("Parsed %u bytes of hex string", length);
      input_file_name = "<hex string>";
    }
    else
    {
      // Assume it is the filename
      input_file_name = argv[i];
    }
  }

  if (input_file_name.IsEmpty())
  {
    Log_ErrorPrintf("Missing input file");
    return false;
  }

  return true;
}

static void PrintInstruction(uint32 address, const CPU_X86::OldInstruction* instruction)
{
  SmallString hex_string;
  SmallString instr_string;

  uint32 instruction_length = (instruction) ? instruction->length : 6;
  for (uint32 i = 0; i < instruction_length && (address + i) < input_code.GetSize(); i++)
    hex_string.AppendFormattedString("%02X ", ZeroExtend32(input_code[address + i]));
  for (uint32 i = instruction_length; i < 6; i++)
    hex_string.AppendString("   ");

  if (instruction)
  {
    if (!CPU_X86::DisassembleToString(instruction, address, &instr_string))
      instr_string = "<disassembly failed>";

    fprintf(stdout, "0x%08X | %s | %s\n", address, hex_string.GetCharArray(), instr_string.GetCharArray());
  }
  else
  {
    fprintf(stderr, "Failed to decode instruction at offset 0x%08X\n", address);
    fprintf(stderr, "Bytes at failure point: %s\n", hex_string.GetCharArray());
  }
}

static bool RunDisassembler()
{
  if (address_size == CPU_X86::AddressSize_16)
    Log_DevPrintf("Assuming 16-bit code");
  else
    Log_DevPrintf("Assuming 32-bit code");

  Log_DevPrintf("Origin address: 0x%08X", origin_address);

  uint32 offset = 0;
  bool error = false;
  while (offset < input_code.GetSize())
  {
    CPU_X86::OldInstruction instruction;
    ArrayFetchInstructionByteCallback fetch_callback(input_code.GetBasePointer(), offset, input_code.GetSize());
    if (CPU_X86::DecodeInstruction(&instruction, address_size, operand_size, fetch_callback))
    {
      PrintInstruction(offset, &instruction);
      offset += instruction.length;
    }
    else
    {
      // Skip one byte and try again
      PrintInstruction(offset, nullptr);
      offset++;
      error = true;
    }
  }

  error |= (offset > input_code.GetSize());
  return !error;
}

int main(int argc, char* argv[])
{
  g_pLog->SetConsoleOutputParams(true);
  g_pLog->SetDebugOutputParams(true);

  if (!ParseArguments(argc, argv))
    return -1;

  if (input_code.IsEmpty())
  {
    Log_DevPrintf("Reading input file: %s", input_file_name.GetCharArray());
    if (!ReadFileToArray(&input_code, input_file_name))
      return -2;
  }

  if (!RunDisassembler())
    return -3;

  return 0;
}
