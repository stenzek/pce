#include "YBaseLib/AutoReleasePtr.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/String.h"
#include "YBaseLib/StringConverter.h"
#include "pce/cpu_x86/decoder.h"
#include <cstdio>
#include <cstring>
Log_SetChannel(Main);

static bool ReadFileToArray(PODArray<byte>* dest_array, const char* filename)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", filename);
    return false;
  }

  dest_array->Resize(static_cast<u32>(stream->GetSize()));
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
u32 origin_address = 0;

static bool ParseArguments(int argc, char* argv[])
{
#define CHECK_ARG(str) !std::strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !std::strcmp(argv[i], str) && ((i + 1) < argc)

  for (int i = 1; i < argc; i++)
  {
    if (CHECK_ARG_PARAM("-b"))
    {
      u32 bit_width = StringConverter::StringToUInt32(argv[++i]);
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
        origin_address = Truncate32(strtol(arg + 2, nullptr, 16));
      else
        origin_address = Truncate32(strtol(arg, nullptr, 10));
    }
    else if (CHECK_ARG_PARAM("-hex"))
    {
      const char* data = argv[++i];
      u32 length = Truncate32(std::strlen(data));
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

static void PrintInstruction(const u32 offset, const CPU_X86::Instruction* instruction)
{
  SmallString hex_string;
  SmallString instr_string;

  u32 instruction_length = (instruction) ? instruction->length : 16;
  for (u32 i = 0; i < instruction_length && (offset + i) < input_code.GetSize(); i++)
    hex_string.AppendFormattedString("%02X ", ZeroExtend32(input_code[offset + i]));
  for (u32 i = instruction_length; i < 6; i++)
    hex_string.AppendString("   ");

  if (instruction)
  {
    CPU_X86::Decoder::DisassembleToString(instruction, &instr_string);
  }
  else
  {
    fprintf(stderr, "Failed to decode instruction at address 0x%08X (offset 0x%08X)\n", origin_address + offset,
            offset);
    fprintf(stderr, "Bytes at failure point: %s\n", hex_string.GetCharArray());
    instr_string = "<decode error>";
  }

  fprintf(stdout, "0x%08X | %s | %s\n", origin_address + offset, hex_string.GetCharArray(),
          instr_string.GetCharArray());
}

static bool RunDisassembler()
{
  if (address_size == CPU_X86::AddressSize_16)
    Log_DevPrintf("Assuming 16-bit code");
  else
    Log_DevPrintf("Assuming 32-bit code");

  Log_DevPrintf("Origin address: 0x%08X", origin_address);

  AutoReleasePtr<ByteStream> memory_stream =
    ByteStream_CreateReadOnlyMemoryStream(input_code.GetBasePointer(), input_code.GetSize());
  bool error = false;

  while (memory_stream->GetPosition() < memory_stream->GetSize() && !memory_stream->InErrorState())
  {
    u64 instruction_offset = memory_stream->GetPosition();

    CPU_X86::Instruction instruction;
    if (CPU_X86::Decoder::DecodeInstruction(&instruction, address_size, operand_size,
                                            origin_address + Truncate32(instruction_offset), memory_stream))
    {
      PrintInstruction(Truncate32(instruction_offset), &instruction);
    }
    else
    {
      // Skip one byte and try again
      PrintInstruction(Truncate32(instruction_offset), nullptr);
      memory_stream->SeekAbsolute(instruction_offset + 1);
      error = true;
    }
  }

  error |= memory_stream->InErrorState();
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
