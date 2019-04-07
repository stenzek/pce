#include "pce/cpu_8086/instruction.h"

class ByteStream;
class String;

namespace CPU_8086 {

class Decoder
{
public:
  // Decode with custom fetch handlers.
  static bool DecodeInstruction(Instruction* instruction, VirtualMemoryAddress eip_addr,
                                std::function<bool(u8*)> fetchb, std::function<bool(u16*)> fetchw);

  // Decode from stream.
  static bool DecodeInstruction(Instruction* instruction, VirtualMemoryAddress eip_addr, ByteStream* stream);

  // Disassemble instruction to string.
  static void DisassembleToString(const Instruction* instruction, String* out_string);

  // Mod R/M decoding.
  struct ModRMAddress
  {
    ModRMAddressingMode addressing_mode;
    u8 base_register;
    u8 index_register;
    u8 displacement_size;
    Segment default_segment;
  };
  static const ModRMAddress* DecodeModRMAddress(u8 modrm);

  // Helpers
  static const char* GetOperationName(Operation op);
  static const char* GetRegisterName(Reg8 reg);
  static const char* GetRegisterName(Reg16 reg);
  static const char* GetSegmentName(Segment reg);

private:
  struct TableEntry
  {
    Operation operation;
    Instruction::Operand operands[3];
    const TableEntry* next_table;
  };

  // Pointers to instruction tables.
  static const size_t OPCODE_TABLE_SIZE = 256;
  static const size_t MODRM_EXTENSION_OPCODE_TABLE_SIZE = 8;
  static const TableEntry base[OPCODE_TABLE_SIZE];
  static const TableEntry prefix_80[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_81[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_82[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_83[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D0[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D1[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D2[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D3[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_F6[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_F7[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_FE[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_FF[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
};

} // namespace CPU_8086
