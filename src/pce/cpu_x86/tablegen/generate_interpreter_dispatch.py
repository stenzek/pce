from common import *
from writer import Writer
import opcodes_8086
import opcodes_x86
import sys

MODULE = None
INTERPRETER_PREFIX = ""
DISPATCH_FUNCTION_NAME = ""

def gen_dispatch(writer):
    writer.write("void %s(CPU* cpu)" % DISPATCH_FUNCTION_NAME)
    writer.begin_scope()
    writer.write("for (;;)")
    writer.begin_scope()
    writer.write("u8 opcode = cpu->FetchInstructionByte();")
    writer.write("switch (opcode)")
    writer.begin_scope()
    gen_cases(writer, "base", False, "")
    writer.end_scope()  # end switch

    writer.write("// If we hit here, it means the opcode is invalid, as all other switch cases continue")
    writer.write("RaiseInvalidOpcode(cpu);")
    writer.write("return;")
    writer.end_scope()  # end for loop

    writer.end_scope()  # end function


def gen_cases(writer, table_name, modrm_fetched, prefix):
    table = getattr(MODULE, table_name)
    for encoding in table:
        opcode = table[encoding]
        if opcode.type == OpcodeType.Invalid or opcode.type == OpcodeType.InvalidX87:
            continue
        writer.write("case 0x%02X: // %s" % (encoding, str(opcode)))
        gen_opcode(writer, opcode, modrm_fetched, prefix)


def gen_opcode(writer, opcode, modrm_fetched, prefix):
    if opcode.type == OpcodeType.SegmentPrefix:
        writer.indent()
        writer.write("cpu->idata.segment = %s;" % opcode.operands[0].data.value)
        writer.write("cpu->idata.has_segment_override = true;")
        writer.write("continue;")
        writer.deindent()
    elif opcode.type == OpcodeType.OperandSizePrefix:
        writer.indent()
        writer.write(
            "cpu->idata.operand_size = (cpu->m_current_operand_size == OperandSize_16) ? OperandSize_32 : OperandSize_16;"
        )
        writer.write("continue;")
        writer.deindent()
    elif opcode.type == OpcodeType.AddressSizePrefix:
        writer.indent()
        writer.write(
            "cpu->idata.address_size = (cpu->m_current_address_size == AddressSize_16) ? AddressSize_32 : AddressSize_16;"
        )
        writer.write("continue;")
        writer.deindent()
    elif opcode.type == OpcodeType.LockPrefix:
        writer.indent()
        writer.write("cpu->idata.has_lock = true;")
        writer.write("continue;")
        writer.deindent()
    elif opcode.type == OpcodeType.RepPrefix:
        writer.indent()
        writer.write("cpu->idata.has_rep = true;")
        writer.write("continue;")
        writer.deindent()
    elif opcode.type == OpcodeType.RepNEPrefix:
        writer.indent()
        writer.write("cpu->idata.has_rep = true;")
        writer.write("cpu->idata.has_repne = true;")
        writer.write("continue;")
        writer.deindent()
    elif opcode.type == OpcodeType.Normal:
        writer.indent()
        for i in range(len(opcode.operands)):
            operand = opcode.operands[i]
            if operand.needs_modrm() and not modrm_fetched:
                writer.write("FetchModRM(cpu); // fetch modrm for operand %d (%s)" % (i, operand.mode.value))
                modrm_fetched = True
            if operand.mode == OperandMode.Immediate or operand.mode == OperandMode.Immediate2 or \
               operand.mode == OperandMode.Relative or operand.mode == OperandMode.Memory or \
               operand.mode == OperandMode.FarAddress or operand.mode == OperandMode.ModRM_RM:
                writer.write("FetchImmediate<%s>(cpu); // fetch immediate for operand %d (%s)" %
                             (operand.template_value(), i, operand.mode.value))

        line = INTERPRETER_PREFIX + "Execute_%s" % opcode.operation.value
        count = 0
        if opcode.cc is not None:
            line += "<" + opcode.cc.value
            count += 1

        for i in range(len(opcode.operands)):
            line += (", " if count > 0 else "<") + opcode.operands[i].template_value()
            count += 1

        if count > 0:
            line += ">"

        line += "(cpu);"
        writer.write(line)
        writer.write("return;")
        writer.deindent()
    elif opcode.type == OpcodeType.Extension or opcode.type == OpcodeType.ModRMRegExtension:
        prefix_bytes = "%s%02X" % (prefix, opcode.encoding)
        writer.begin_scope()
        if opcode.type == OpcodeType.Extension:
            writer.write("opcode = cpu->FetchInstructionByte();")
            writer.write("switch (opcode)")
        else:
            writer.write("FetchModRM(cpu); // fetch modrm for extension")
            writer.write("switch (cpu->idata.GetModRM_Reg() & 0x07)")
            modrm_fetched = True
        writer.begin_scope()
        gen_cases(writer, "prefix_" + prefix_bytes, modrm_fetched, prefix_bytes)
        writer.end_scope()  # switch
        writer.end_scope()  # case
        writer.write("break;")
    elif opcode.type == OpcodeType.X87Extension:
        prefix_bytes = "%s%02X" % (prefix, opcode.encoding)
        writer.begin_scope()
        writer.write("FetchModRM(cpu); // fetch modrm for X87 extension")
        modrm_fetched = True

        for suffix in ["reg", "mem"]:
            table_name = "prefix_" + prefix_bytes + "_" + suffix
            writer.write("if (!cpu->idata.ModRM_RM_IsReg())" if suffix == "reg" else "else")
            writer.begin_scope()
            writer.write("// %s" % table_name)
            writer.write("switch (%s) // %s" % (("cpu->idata.GetModRM_Reg() & 0x07"
                                                 if suffix == "reg" else "cpu->idata.modrm & 0x3F"), suffix))
            writer.begin_scope()
            gen_cases(writer, table_name, modrm_fetched, prefix_bytes)

            # Invalid x87 opcodes should still fetch the modrm operands, but fail silently.
            writer.write("default:")
            writer.indent()
            writer.write("FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);")
            writer.write(INTERPRETER_PREFIX + "StartX87Instruction(cpu);")
            writer.write("return;")
            writer.deindent()  # default
            writer.end_scope()  # switch
            writer.end_scope()  # if
        writer.end_scope()
    elif opcode.type == OpcodeType.Escape:
        # Ignore x87 for 8086, but we still need to fetch the operands
        writer.begin_scope()
        writer.write("FetchModRM(cpu); // fetch modrm for X87 extension")
        writer.write("FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);")
        writer.write("return;")
        writer.end_scope()


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("usage: %s <8086|x86> output_filename" % sys.argv[0])
        sys.exit(1)

    table = None
    if sys.argv[1] == "8086":
        MODULE = opcodes_8086
        DISPATCH_FUNCTION_NAME = "CPU_8086::Instructions::DispatchInstruction"
    elif sys.argv[1] == "x86":
        MODULE = opcodes_x86
        INTERPRETER_PREFIX = "Interpreter::"
        DISPATCH_FUNCTION_NAME = "CPU_X86::InterpreterBackend::Dispatch"

    writer = Writer(sys.argv[2])
    gen_dispatch(writer)
    writer.close()
