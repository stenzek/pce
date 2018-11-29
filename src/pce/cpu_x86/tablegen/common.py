from enum import Enum


class Model(Enum):
    i8088 = "Model_8088"
    i8086 = "Model_8086"
    v20 = "Model_V20"
    v30 = "Model_V30"
    i80188 = "Model_80188"
    i80186 = "Model_80186"
    i386 = "Model_386"
    i486 = "Model_486"
    Pentium = "Model_Pentium"


class Reg8(Enum):
    AL = "Reg8_AL"
    CL = "Reg8_CL"
    DL = "Reg8_DL"
    BL = "Reg8_BL"
    AH = "Reg8_AH"
    CH = "Reg8_CH"
    DH = "Reg8_DH"
    BH = "Reg8_BH"


class Reg16(Enum):
    AX = "Reg16_AX"
    CX = "Reg16_CX"
    DX = "Reg16_DX"
    BX = "Reg16_BX"
    SP = "Reg16_SP"
    BP = "Reg16_BP"
    SI = "Reg16_SI"
    DI = "Reg16_DI"
    IP = "Reg16_IP"
    FLAGS = "Reg16_FLAGS"


class Reg32(Enum):
    EAX = "Reg32_EAX"
    ECX = "Reg32_ECX"
    EDX = "Reg32_EDX"
    EBX = "Reg32_EBX"
    ESP = "Reg32_ESP"
    EBP = "Reg32_EBP"
    ESI = "Reg32_ESI"
    EDI = "Reg32_EDI"
    EIP = "Reg32_EIP"
    EFLAGS = "Reg32_EFLAGS"
    CR0 = "Reg32_CR0"
    CR2 = "Reg32_CR2"
    CR3 = "Reg32_CR3"
    CR4 = "Reg32_CR4"
    DR0 = "Reg32_DR0"
    DR1 = "Reg32_DR1"
    DR2 = "Reg32_DR2"
    DR3 = "Reg32_DR3"
    DR4 = "Reg32_DR4"
    DR5 = "Reg32_DR5"
    DR6 = "Reg32_DR6"
    DR7 = "Reg32_DR7"
    TR3 = "Reg32_TR3"
    TR4 = "Reg32_TR4"
    TR5 = "Reg32_TR5"
    TR6 = "Reg32_TR6"
    TR7 = "Reg32_TR7"


class Segment(Enum):
    ES = "Segment_ES"
    CS = "Segment_CS"
    SS = "Segment_SS"
    DS = "Segment_DS"
    FS = "Segment_FS"
    GS = "Segment_GS"


class Operation(Enum):
    Invalid = "Operation_Invalid"
    Extension = "Operation_Extension"
    Extension_ModRM_Reg = "Operation_Extension_ModRM_Reg"
    Extension_ModRM_X87 = "Operation_Extension_ModRM_X87"
    Segment_Prefix = "Operation_Segment_Prefix"
    Rep_Prefix = "Operation_Rep_Prefix"
    RepNE_Prefix = "Operation_RepNE_Prefix"
    Lock_Prefix = "Operation_Lock_Prefix"
    OperandSize_Prefix = "Operation_OperandSize_Prefix"
    AddressSize_Prefix = "Operation_AddressSize_Prefix"
    Escape = "Operation_Escape"

    AAA = "Operation_AAA"
    AAD = "Operation_AAD"
    AAM = "Operation_AAM"
    AAS = "Operation_AAS"
    ADC = "Operation_ADC"
    ADD = "Operation_ADD"
    AND = "Operation_AND"
    CALL_Near = "Operation_CALL_Near"
    CALL_Far = "Operation_CALL_Far"
    CBW = "Operation_CBW"
    CLC = "Operation_CLC"
    CLD = "Operation_CLD"
    CLI = "Operation_CLI"
    CMC = "Operation_CMC"
    CMP = "Operation_CMP"
    CMPS = "Operation_CMPS"
    CWD = "Operation_CWD"
    DAA = "Operation_DAA"
    DAS = "Operation_DAS"
    DEC = "Operation_DEC"
    DIV = "Operation_DIV"
    ESC = "Operation_ESC"
    HLT = "Operation_HLT"
    IDIV = "Operation_IDIV"
    IMUL = "Operation_IMUL"
    IN = "Operation_IN"
    INC = "Operation_INC"
    INT = "Operation_INT"
    INT3 = "Operation_INT3"
    INTO = "Operation_INTO"
    IRET = "Operation_IRET"
    Jcc = "Operation_Jcc"
    JCXZ = "Operation_JCXZ"
    JMP_Near = "Operation_JMP_Near"
    JMP_Far = "Operation_JMP_Far"
    LAHF = "Operation_LAHF"
    LDS = "Operation_LDS"
    LEA = "Operation_LEA"
    LES = "Operation_LES"
    LOCK = "Operation_LOCK"
    LODS = "Operation_LODS"
    LOOP = "Operation_LOOP"
    LXS = "Operation_LXS"
    MOV = "Operation_MOV"
    MOVS = "Operation_MOVS"
    MOV_Sreg = "Operation_MOV_Sreg"
    MUL = "Operation_MUL"
    NEG = "Operation_NEG"
    NOP = "Operation_NOP"
    NOT = "Operation_NOT"
    OR = "Operation_OR"
    OUT = "Operation_OUT"
    POP = "Operation_POP"
    POP_Sreg = "Operation_POP_Sreg"
    POPF = "Operation_POPF"
    PUSH = "Operation_PUSH"
    PUSH_Sreg = "Operation_PUSH_Sreg"
    PUSHF = "Operation_PUSHF"
    RCL = "Operation_RCL"
    RCR = "Operation_RCR"
    REPxx = "Operation_REPxx"
    RET_Near = "Operation_RET_Near"
    RET_Far = "Operation_RET_Far"
    ROL = "Operation_ROL"
    ROR = "Operation_ROR"
    SAHF = "Operation_SAHF"
    SAL = "Operation_SAL"
    SALC = "Operation_SALC"
    SAR = "Operation_SAR"
    SBB = "Operation_SBB"
    SCAS = "Operation_SCAS"
    SHL = "Operation_SHL"
    SHR = "Operation_SHR"
    STC = "Operation_STC"
    STD = "Operation_STD"
    STI = "Operation_STI"
    STOS = "Operation_STOS"
    SUB = "Operation_SUB"
    TEST = "Operation_TEST"
    WAIT = "Operation_WAIT"
    XCHG = "Operation_XCHG"
    XLAT = "Operation_XLAT"
    XOR = "Operation_XOR"

    # 80286+
    BOUND = "Operation_BOUND"
    BSF = "Operation_BSF"
    BSR = "Operation_BSR"
    BT = "Operation_BT"
    BTS = "Operation_BTS"
    BTR = "Operation_BTR"
    BTC = "Operation_BTC"
    SHLD = "Operation_SHLD"
    SHRD = "Operation_SHRD"
    INS = "Operation_INS"
    OUTS = "Operation_OUTS"
    CLTS = "Operation_CLTS"
    ENTER = "Operation_ENTER"
    LEAVE = "Operation_LEAVE"
    LSS = "Operation_LSS"
    LGDT = "Operation_LGDT"
    SGDT = "Operation_SGDT"
    LIDT = "Operation_LIDT"
    SIDT = "Operation_SIDT"
    LLDT = "Operation_LLDT"
    SLDT = "Operation_SLDT"
    LTR = "Operation_LTR"
    STR = "Operation_STR"
    LMSW = "Operation_LMSW"
    SMSW = "Operation_SMSW"
    VERR = "Operation_VERR"
    VERW = "Operation_VERW"
    ARPL = "Operation_ARPL"
    LAR = "Operation_LAR"
    LSL = "Operation_LSL"
    PUSHA = "Operation_PUSHA"
    POPA = "Operation_POPA"

    # 80386+
    LFS = "Operation_LFS"
    LGS = "Operation_LGS"
    MOVSX = "Operation_MOVSX"
    MOVZX = "Operation_MOVZX"
    MOV_CR = "Operation_MOV_CR"
    MOV_DR = "Operation_MOV_DR"
    MOV_TR = "Operation_MOV_TR"
    SETcc = "Operation_SETcc"

    # 80486+
    BSWAP = "Operation_BSWAP"
    CMPXCHG = "Operation_CMPXCHG"
    CMOVcc = "Operation_CMOVcc"
    INVD = "Operation_INVD"
    WBINVD = "Operation_WBINVD"
    INVLPG = "Operation_INVLPG"
    XADD = "Operation_XADD"

    # Pentium+
    CPUID = "Operation_CPUID"
    RDTSC = "Operation_RDTSC"
    CMPXCHG8B = "Operation_CMPXCHG8B"

    # 8087+
    F2XM1 = "Operation_F2XM1"
    FABS = "Operation_FABS"
    FADD = "Operation_FADD"
    FADDP = "Operation_FADDP"
    FBLD = "Operation_FBLD"
    FBSTP = "Operation_FBSTP"
    FCHS = "Operation_FCHS"
    FCOM = "Operation_FCOM"
    FCOMP = "Operation_FCOMP"
    FCOMPP = "Operation_FCOMPP"
    FDECSTP = "Operation_FDECSTP"
    FDIV = "Operation_FDIV"
    FDIVP = "Operation_FDIVP"
    FDIVR = "Operation_FDIVR"
    FDIVRP = "Operation_FDIVRP"
    FFREE = "Operation_FFREE"
    FIADD = "Operation_FIADD"
    FICOM = "Operation_FICOM"
    FICOMP = "Operation_FICOMP"
    FIDIV = "Operation_FIDIV"
    FIDIVR = "Operation_FIDIVR"
    FILD = "Operation_FILD"
    FIMUL = "Operation_FIMUL"
    FINCSTP = "Operation_FINCSTP"
    FIST = "Operation_FIST"
    FISTP = "Operation_FISTP"
    FISUB = "Operation_FISUB"
    FISUBR = "Operation_FISUBR"
    FLD = "Operation_FLD"
    FLD1 = "Operation_FLD1"
    FLDCW = "Operation_FLDCW"
    FLDENV = "Operation_FLDENV"
    FLDL2E = "Operation_FLDL2E"
    FLDL2T = "Operation_FLDL2T"
    FLDLG2 = "Operation_FLDLG2"
    FLDLN2 = "Operation_FLDLN2"
    FLDPI = "Operation_FLDPI"
    FLDZ = "Operation_FLDZ"
    FMUL = "Operation_FMUL"
    FMULP = "Operation_FMULP"
    FNCLEX = "Operation_FNCLEX"
    FNDISI = "Operation_FNDISI"
    FNENI = "Operation_FNENI"
    FNINIT = "Operation_FNINIT"
    FNOP = "Operation_FNOP"
    FNSAVE = "Operation_FNSAVE"
    FNSTCW = "Operation_FNSTCW"
    FNSTENV = "Operation_FNSTENV"
    FNSTSW = "Operation_FNSTSW"
    FPATAN = "Operation_FPATAN"
    FPREM = "Operation_FPREM"
    FPTAN = "Operation_FPTAN"
    FRNDINT = "Operation_FRNDINT"
    FRSTOR = "Operation_FRSTOR"
    FSCALE = "Operation_FSCALE"
    FSQRT = "Operation_FSQRT"
    FST = "Operation_FST"
    FSTP = "Operation_FSTP"
    FSUB = "Operation_FSUB"
    FSUBP = "Operation_FSUBP"
    FSUBR = "Operation_FSUBR"
    FSUBRP = "Operation_FSUBRP"
    FTST = "Operation_FTST"
    FXAM = "Operation_FXAM"
    FXCH = "Operation_FXCH"
    FXTRACT = "Operation_FXTRACT"
    FYL2X = "Operation_FYL2X"
    FYL2XP1 = "Operation_FYL2XP1"

    # 80287+
    FSETPM = "Operation_FSETPM"

    # 80387+
    FCOS = "Operation_FCOS"
    FPREM1 = "Operation_FPREM1"
    FSIN = "Operation_FSIN"
    FSINCOS = "Operation_FSINCOS"
    FUCOM = "Operation_FUCOM"
    FUCOMP = "Operation_FUCOMP"
    FUCOMPP = "Operation_FUCOMPP"


class JumpCondition(Enum):
    Always = "JumpCondition_Always"
    Overflow = "JumpCondition_Overflow"
    NotOverflow = "JumpCondition_NotOverflow"
    Sign = "JumpCondition_Sign"
    NotSign = "JumpCondition_NotSign"
    Equal = "JumpCondition_Equal"
    NotEqual = "JumpCondition_NotEqual"
    Below = "JumpCondition_Below"
    AboveOrEqual = "JumpCondition_AboveOrEqual"
    BelowOrEqual = "JumpCondition_BelowOrEqual"
    Above = "JumpCondition_Above"
    Less = "JumpCondition_Less"
    GreaterOrEqual = "JumpCondition_GreaterOrEqual"
    LessOrEqual = "JumpCondition_LessOrEqual"
    Greater = "JumpCondition_Greater"
    Parity = "JumpCondition_Parity"
    NotParity = "JumpCondition_NotParity"
    CXZero = "JumpCondition_CXZero"


class OperandSize(Enum):
    Byte = "OperandSize_8"  # byte
    Word = "OperandSize_16"  # word
    DWord = "OperandSize_32"  # dword
    QWord = "OperandSize_64"  # double-precision float
    TWord = "OperandSize_80"  # extended-precision float
    Auto = "OperandSize_Count"  # TODO: Rename to "inherit"


class OperandMode(Enum):
    Nothing = "OperandMode_None"
    Constant = "OperandMode_Constant"
    Register = "OperandMode_Register"
    RegisterIndirect = "OperandMode_RegisterIndirect"
    SegmentRegister = "OperandMode_SegmentRegister"
    Immediate = "OperandMode_Immediate"
    Immediate2 = "OperandMode_Immediate2"
    Relative = "OperandMode_Relative"
    Memory = "OperandMode_Memory"
    FarAddress = "OperandMode_FarAddress"
    ModRM_Reg = "OperandMode_ModRM_Reg"
    ModRM_RM = "OperandMode_ModRM_RM"
    ModRM_SegmentRegister = "OperandMode_ModRM_SegmentReg"
    ModRM_ControlRegister = "OperandMode_ModRM_ControlRegister"
    ModRM_DebugRegister = "OperandMode_ModRM_DebugRegister"
    ModRM_TestRegister = "OperandMode_ModRM_TestRegister"
    FPRegister = "OperandMode_FPRegister"
    JumpCondition = "OperandMode_JumpCondition"


class Operand:
    def __init__(self, text, size, mode, data=None):
        self.text = text
        self.size = size
        self.mode = mode
        self.data = data

    def needs_modrm(self):
        return self.mode == OperandMode.ModRM_ControlRegister or \
               self.mode == OperandMode.ModRM_DebugRegister or \
               self.mode == OperandMode.ModRM_Reg or \
               self.mode == OperandMode.ModRM_RM or \
               self.mode == OperandMode.ModRM_SegmentRegister or \
               self.mode == OperandMode.ModRM_TestRegister

    def template_value(self):
        if isinstance(self.data, Enum):
            return "%s, %s, %s" % (self.size.value, self.mode.value, self.data.value)
        elif self.data is not None:
            return "%s, %s, %s" % (self.size.value, self.mode.value, str(self.data))
        else:
            return "%s, %s, 0" % (self.size.value, self.mode.value)


class OpcodeType(Enum):
    Invalid = "Invalid"
    Normal = "Normal"
    SegmentPrefix = "SegmentPrefix"
    OperandSizePrefix = "OperandSizePrefix"
    AddressSizePrefix = "AddressSizePrefix"
    LockPrefix = "LockPrefix"
    RepPrefix = "RepPrefix"
    RepNEPrefix = "RepNEPrefix"
    Nop = "Nop"
    Extension = "Extension"
    ModRMRegExtension = "ModRMRegExtension"
    X87Extension = "X86Extension"
    InvalidX87 = "InvalidX87"


class Opcode:
    def __init__(self,
                 encoding,
                 type,
                 operation,
                 op1=None,
                 op2=None,
                 op3=None,
                 cc=None,
                 min_model=Model.i386):
        self.encoding = encoding
        self.type = type
        self.operation = operation
        self.operands = []
        if op1 is not None:
            self.operands.append(op1)
            if op2 is not None:
                self.operands.append(op2)
                if op3 is not None:
                    self.operands.append(op3)
        self.cc = cc
        self.min_model = min_model


    def override_operand_size(self, size):
        op = Opcode(self.encoding, self.type, self.operation, cc=self.cc, min_model=self.min_model)
        for operand in self.operands:
            if operand.size == OperandSize.Auto:
                op.operands.append(Operand(operand.text, size, operand.mode, operand.data))
            else:
                op.operands.append(operand)
        return op


    def __str__(self):
        if self.type == OpcodeType.Invalid:
            return "Invalid 0x%02X" % self.encoding
        elif self.type == OpcodeType.InvalidX87:
            return "Invalid X87 0x%02X" % self.encoding
        elif self.type == OpcodeType.SegmentPrefix:
            return "Prefix %s" % self.operands[0].data
        elif self.type == OpcodeType.AddressSizePrefix:
            return "Address-Size Prefix"
        elif self.type == OpcodeType.OperandSizePrefix:
            return "Operand-Size Prefix"
        elif self.type == OpcodeType.LockPrefix:
            return "Lock Prefix"
        elif self.type == OpcodeType.RepPrefix:
            return "Rep Prefix"
        elif self.type == OpcodeType.RepNEPrefix:
            return "RepNE Prefix"
        elif self.type == OpcodeType.Extension:
            return "Extension 0x%02X" % self.encoding
        elif self.type == OpcodeType.ModRMRegExtension:
            return "ModRM-Reg-Extension 0x%02X" % self.encoding
        elif self.type == OpcodeType.X87Extension:
            return "X87 Extension 0x%02X" % self.encoding
        else:
            comment = self.operation.value[10:]
            for i in range(len(self.operands)):
                comment += ", " if i > 0 else " "
                comment += self.operands[i].text
            return comment

