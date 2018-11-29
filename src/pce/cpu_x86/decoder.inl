#include "pce/types.h"
#include <cstring>

namespace CPU_X86 {
template<typename fetchb_type, typename fetchw_type, typename fetchd_type>
bool CPU_X86::Decoder::DecodeInstruction(Instruction* instruction, AddressSize address_size, OperandSize operand_size,
                                         VirtualMemoryAddress eip_addr, fetchb_type fetchb, fetchw_type fetchw,
                                         fetchd_type fetchd)
{
  std::memset(instruction, 0, sizeof(*instruction));
  instruction->data.address_size = address_size;
  instruction->data.operand_size = operand_size;
  instruction->data.segment = Segment_DS;
  instruction->address = eip_addr;

  const TableEntry* table = base;
  bool has_modrm = false;
  for (;;)
  {
    // Fetch first byte.
    uint8 opcode;
    if (!fetchb(&opcode))
      return false;

    instruction->length += sizeof(uint8);
    const TableEntry* te = &table[opcode];

    // Handle special cases (prefixes, overrides).
    switch (te->operation)
    {
      case Operation_Segment_Prefix:
        instruction->data.segment = te->operands[0].segreg;
        instruction->data.has_segment_override = true;
        continue;
      case Operation_OperandSize_Prefix:
        instruction->data.operand_size = (operand_size == OperandSize_16) ? OperandSize_32 : OperandSize_16;
        continue;
      case Operation_AddressSize_Prefix:
        instruction->data.address_size = (address_size == AddressSize_16) ? AddressSize_32 : AddressSize_16;
        continue;
      case Operation_Lock_Prefix:
        instruction->data.has_lock = true;
        continue;
      case Operation_Rep_Prefix:
        instruction->data.has_rep = true;
        continue;
      case Operation_RepNE_Prefix:
        instruction->data.has_rep = true;
        instruction->data.has_repne = true;
        continue;
      case Operation_Extension:
        table = te->next_table;
        continue;
      case Operation_Extension_ModRM_Reg:
      {
        has_modrm = true;
        if (!fetchb(&instruction->data.modrm))
          return false;
        instruction->length += sizeof(uint8);
        te = &te->next_table[instruction->data.GetModRM_Reg()];
      }
      break;
      case Operation_Extension_ModRM_X87:
      {
        has_modrm = true;
        if (!fetchb(&instruction->data.modrm))
          return false;
        instruction->length += sizeof(uint8);
        if (!instruction->data.ModRM_RM_IsReg())
          te = &te->next_table[instruction->data.GetModRM_Reg() & 0x07];
        else
          te = &te->next_table[8 + (instruction->data.modrm & 0x3F)];
      }
      break;
      default:
        break;
    }

    // Check for invalid opcodes.
    if (te->operation == Operation_Invalid)
      return false;

    // Copy operands from this table entry, if any.
    instruction->operation = te->operation;
    for (uint32 i = 0; i < countof(te->operands); i++)
    {
      const Instruction::Operand& table_operand = te->operands[i];
      if (table_operand.mode == OperandMode_None)
        continue;

      Instruction::Operand& operand = instruction->operands[i];
      std::memcpy(&operand, &table_operand, sizeof(Instruction::Operand));
      if (table_operand.size == OperandSize_Count)
        operand.size = instruction->data.operand_size;

      // Fetch modrm and immediates.
      switch (operand.mode)
      {
        case OperandMode_ModRM_Reg:
        case OperandMode_ModRM_ControlRegister:
        case OperandMode_ModRM_DebugRegister:
        case OperandMode_ModRM_TestRegister:
        {
          if (!has_modrm)
          {
            has_modrm = true;
            if (!fetchb(&instruction->data.modrm))
              return false;
            instruction->length += sizeof(uint8);
          }
        }
        break;

        case OperandMode_ModRM_RM:
        {
          if (!has_modrm)
          {
            has_modrm = true;
            if (!fetchb(&instruction->data.modrm))
              return false;
            instruction->length += sizeof(uint8);
          }

          const Decoder::ModRMAddress* addr =
            Decoder::DecodeModRMAddress(instruction->GetAddressSize(), instruction->data.modrm);
          if (addr->addressing_mode == ModRMAddressingMode::Register)
          {
            // not a memory address
            instruction->data.modrm_rm_register = true;
          }
          else
          {
            uint8 displacement_size = addr->displacement_size;
            if (addr->addressing_mode == ModRMAddressingMode::SIB)
            {
              // SIB has a displacement instead of base if set to EBP
              if (!fetchb(&instruction->data.sib))
                return false;
              instruction->length += sizeof(uint8);
              const Reg32 base_reg = instruction->data.GetSIBBaseRegister();
              if (!instruction->data.HasSIBBase())
                displacement_size = 4;
              else if (!instruction->data.has_segment_override && (base_reg == Reg32_ESP || base_reg == Reg32_EBP))
                instruction->data.segment = Segment_SS;
            }
            else
            {
              if (!instruction->data.has_segment_override)
                instruction->data.segment = addr->default_segment;
            }

            switch (displacement_size)
            {
              case 1:
              {
                uint8 tmpbyte;
                if (!fetchb(&tmpbyte))
                  return false;
                instruction->data.disp32 = SignExtend32(tmpbyte);
                instruction->length += sizeof(uint8);
              }
              break;
              case 2:
              {
                uint16 tmpword;
                if (!fetchw(&tmpword))
                  return false;
                instruction->data.disp32 = SignExtend32(tmpword);
                instruction->length += sizeof(uint16);
              }
              break;
              case 4:
              {
                if (!fetchd(&instruction->data.disp32))
                  return false;
                instruction->length += sizeof(uint32);
              }
              break;
            }
          }
        }
        break;

        case OperandMode_Immediate:
        {
          switch (operand.size)
          {
            case OperandSize_8:
            {
              if (!fetchb(&instruction->data.imm8))
                return false;
              instruction->length += sizeof(uint8);
            }
            break;
            case OperandSize_16:
            {
              if (!fetchw(&instruction->data.imm16))
                return false;
              instruction->length += sizeof(uint16);
            }
            break;
            case OperandSize_32:
            {
              if (!fetchd(&instruction->data.imm32))
                return false;
              instruction->length += sizeof(uint32);
            }
            break;
          }
        }
        break;

        case OperandMode_Immediate2:
        {
          switch (operand.size)
          {
            case OperandSize_8:
            {
              if (!fetchb(&instruction->data.imm2_8))
                return false;
              instruction->length += sizeof(uint8);
            }
            break;
            case OperandSize_16:
            {
              if (!fetchw(&instruction->data.imm2_16))
                return false;
              instruction->length += sizeof(uint16);
            }
            break;
            case OperandSize_32:
            {
              if (!fetchd(&instruction->data.imm2_32))
                return false;

              instruction->length += sizeof(uint32);
            }
            break;
          }
        }
        break;

        case OperandMode_Relative:
        {
          switch (operand.size)
          {
            case OperandSize_8:
            {
              uint8 tmpbyte;
              if (!fetchb(&tmpbyte))
                return false;
              instruction->data.disp32 = SignExtend32(tmpbyte);
              instruction->length += sizeof(uint8);
            }
            break;
            case OperandSize_16:
            {
              uint16 tmpword;
              if (!fetchw(&tmpword))
                return false;
              instruction->data.disp32 = SignExtend32(tmpword);
              instruction->length += sizeof(uint16);
            }
            break;
            case OperandSize_32:
            {
              if (!fetchd(&instruction->data.disp32))
                return false;
              instruction->length += sizeof(uint32);
            }
            break;
          }
        }
        break;

        case OperandMode_Memory:
        {
          if (instruction->data.address_size == AddressSize_16)
          {
            uint16 tmpword;
            if (!fetchw(&tmpword))
              return false;
            instruction->data.disp32 = ZeroExtend32(tmpword);
            instruction->length += sizeof(uint16);
          }
          else
          {
            if (!fetchd(&instruction->data.disp32))
              return false;
            instruction->length += sizeof(uint32);
          }
        }
        break;

        case OperandMode_FarAddress:
        {
          if (instruction->data.operand_size == OperandSize_16)
          {
            uint16 tmpword;
            if (!fetchw(&tmpword))
              return false;
            instruction->data.disp32 = ZeroExtend32(tmpword);
            instruction->length += sizeof(uint16);
          }
          else
          {
            if (!fetchd(&instruction->data.disp32))
              return false;
            instruction->length += sizeof(uint32);
          }
          if (!fetchw(&instruction->data.imm16))
            return false;
          instruction->length += sizeof(uint16);
        }
        break;

        default:
          break;
      }
    }

    // Done!
    return true;
  }
}

} // namespace CPU_X86