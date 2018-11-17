#include "pce/cpu_x86/instruction.h"

class ByteStream;
class String;

namespace CPU_X86 {

class Decoder
{
public:
  // Decode with custom fetch handlers.
  template<typename fetchb_type, typename fetchw_type, typename fetchd_type>
  static bool DecodeInstruction(Instruction* instruction, AddressSize address_size, OperandSize operand_size,
                                VirtualMemoryAddress eip_addr, fetchb_type fetchb, fetchw_type fetchw,
                                fetchd_type fetchd);

  // Decode from stream.
  static bool DecodeInstruction(Instruction* instruction, AddressSize address_size, OperandSize operand_size,
                                VirtualMemoryAddress eip_addr, ByteStream* stream);

  // Disassemble instruction to string.
  static void DisassembleToString(const Instruction* instruction, String* out_string);

  // Mod R/M decoding.
  struct ModRMAddress
  {
    ModRMAddressingMode addressing_mode;
    uint8 base_register;
    uint8 index_register;
    uint8 displacement_size;
    Segment default_segment;
  };
  static const ModRMAddress* DecodeModRMAddress(AddressSize address_size, uint8 modrm);

  // Helpers
  static const char* GetOperationName(Operation op);
  static const char* GetRegisterName(Reg8 reg);
  static const char* GetRegisterName(Reg16 reg);
  static const char* GetRegisterName(Reg32 reg);
  static const char* GetSegmentName(Segment reg);

private:
  struct TableEntry
  {
    Operation operation;
    Instruction::Operand operands[3];
    Instruction::IntepreterHandler interpreter_handler;
    const TableEntry* next_table;
    // mincpu, etc.
  };

  // Pointers to instruction tables.
  static const size_t OPCODE_TABLE_SIZE = 256;
  static const size_t MODRM_EXTENSION_OPCODE_TABLE_SIZE = 8;
  static const size_t X87_EXTENSION_OPCODE_TABLE_SIZE = 8 + 64;
  static const TableEntry base[OPCODE_TABLE_SIZE];
  static const TableEntry prefix_0F[OPCODE_TABLE_SIZE];
  static const TableEntry prefix_0F00[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_0F01[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_0FBA[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_80[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_81[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_82[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_83[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_C0[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_C1[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D0[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D1[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D2[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D3[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D8[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_D9[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_DA[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_DB[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_DC[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_DD[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_DE[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_DF[X87_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_F6[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_F7[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_FE[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
  static const TableEntry prefix_FF[MODRM_EXTENSION_OPCODE_TABLE_SIZE];
};

} // namespace CPU_X86

#include "pce/cpu_x86/decoder.inl"
