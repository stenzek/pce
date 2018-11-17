from common import *
from writer import Writer
import opcodes_8086
import opcodes_x86
import sys

MODULE = None
TABLE_DECLARATION = ""
ADD_INTERPRETER_POINTER = False

def get_tables(base, parent):
    tables = []
    if base == getattr(MODULE, "base"):
        tables.append(("", "base", "OPCODE_TABLE_SIZE", base, None))

    for encoding in base:
        opcode = base[encoding]
        encoding_str = "%02X" % encoding
        varname = "prefix_" + parent + encoding_str
        if opcode.type == OpcodeType.Extension:
            table = getattr(MODULE, varname)
            tables.append((parent + encoding_str, varname, "OPCODE_TABLE_SIZE", table, None))
            tables.extend(get_tables(table, parent + encoding_str))
        elif opcode.type == OpcodeType.ModRMRegExtension:
            table = getattr(MODULE, varname)
            tables.append((parent + encoding_str, varname, "MODRM_EXTENSION_OPCODE_TABLE_SIZE", table, None))
        elif opcode.type == OpcodeType.X87Extension:
            reg_table = getattr(MODULE, varname + "_reg")
            mem_table = getattr(MODULE, varname + "_mem")
            tables.append((parent + encoding_str, varname, "X87_EXTENSION_OPCODE_TABLE_SIZE", reg_table, mem_table))
    return tables


def gen_tables(writer):
    tables = get_tables(getattr(MODULE, "base"), "")
    for parent, varname, length, reg_table, mem_table in tables:
        writer.write(TABLE_DECLARATION + "%s[%s] =" % (varname, length))
        writer.write("{")
        writer.indent()
        #print(reg_table)
        for encoding in reg_table:
            gen_opcode(writer, reg_table[encoding], parent)
        if mem_table is not None:
            for encoding in mem_table:
                gen_opcode(writer, mem_table[encoding], parent)
        writer.deindent()
        writer.write("};")

def gen_opcode(writer, opcode, parent):
    if opcode.type == OpcodeType.Invalid or opcode.type == OpcodeType.InvalidX87:
        writer.write("{ Operation_Invalid },")
    elif opcode.type == OpcodeType.SegmentPrefix:
        writer.write("{ Operation_Segment_Prefix, { { OperandSize_Count, OperandMode_SegmentRegister, %s } } }," % opcode.operands[0].data.value)
    elif opcode.type == OpcodeType.OperandSizePrefix:
        writer.write("{ Operation_OperandSize_Prefix },")
    elif opcode.type == OpcodeType.AddressSizePrefix:
        writer.write("{ Operation_AddressSize_Prefix },")
    elif opcode.type == OpcodeType.LockPrefix:
        writer.write("{ Operation_Lock_Prefix },")
    elif opcode.type == OpcodeType.RepPrefix:
        writer.write("{ Operation_Rep_Prefix },")
    elif opcode.type == OpcodeType.RepNEPrefix:
        writer.write("{ Operation_RepNE_Prefix },")
    elif opcode.type == OpcodeType.Normal:
        line = "{ " + opcode.operation.value + ", {"
        count = 0
        if opcode.cc is not None:
            line += " { OperandSize_Count, OperandMode_JumpCondition, %s}" % opcode.cc.value
            count += 1
        for operand in opcode.operands:
            line += (", " if count > 0 else "") + operand.template_value()
            count += 1
        line += "}"
        if ADD_INTERPRETER_POINTER:
            line += ", &Interpreter::Execute_" + opcode.operation.value
            count = 0
            if opcode.cc is not None:
                line += "<" + opcode.cc.value
                count += 1

            for i in range(len(opcode.operands)):
                line += (", " if count > 0 else "<") + opcode.operands[i].template_value()
                count += 1

            if count > 0:
                line += ">"

        line += " },"
        writer.write(line)
    elif opcode.type == OpcodeType.Extension or opcode.type == OpcodeType.ModRMRegExtension or \
         opcode.type == OpcodeType.X87Extension:
        writer.write("{ %s, {}, %sprefix_%s%02X }," % (opcode.operation.value, ", nullptr" if ADD_INTERPRETER_POINTER else "", parent, opcode.encoding))


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("usage: %s <8086|x86> output_filename" % sys.argv[0])
        sys.exit(1)

    if sys.argv[1] == "8086":
        MODULE = opcodes_8086
        TABLE_DECLARATION = "const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::"
    elif sys.argv[1] == "x86":
        MODULE = opcodes_x86
        TABLE_DECLARATION = "const CPU_X86::Decoder::TableEntry CPU_X86::Decoder::"
        ADD_INTERPRETER_POINTER = True
    writer = Writer(sys.argv[2])
    gen_tables(writer)
    writer.close()
