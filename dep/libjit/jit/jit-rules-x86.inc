/* Automatically generated from ../../jit/jit-rules-x86.ins - DO NOT EDIT */
/*
 * Copyright (C) 2004, 2006-2007  Southern Storm Software, Pty Ltd.
 *
 * This file is part of the libjit library.
 *
 * The libjit library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * The libjit library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the libjit library.  If not, see
 * <http://www.gnu.org/licenses/>.
*/

#if defined(JIT_INCLUDE_RULES)

case JIT_OP_TRUNC_SBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_breg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_widen_reg(inst, reg, reg2, 1, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_TRUNC_UBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_breg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_widen_reg(inst, reg, reg2, 0, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_TRUNC_SHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_widen_reg(inst, reg, reg2, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_TRUNC_USHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_widen_reg(inst, reg, reg2, 0, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CHECK_SBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			unsigned char *patch1;
			unsigned char *patch2;
			x86_alu_reg_imm(inst, X86_CMP, reg, -128);
			patch1 = inst;
			x86_branch8(inst, X86_CC_LE, 0, 1);
			x86_alu_reg_imm(inst, X86_CMP, reg, 127);
			patch2 = inst;
			x86_branch8(inst, X86_CC_LE, 0, 1);
			x86_patch(patch1, inst);
			inst = throw_builtin(inst, func, JIT_RESULT_OVERFLOW);
			x86_patch(patch2, inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CHECK_UBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			unsigned char *patch1;
			x86_alu_reg_imm(inst, X86_CMP, reg, 256);
			patch1 = inst;
			x86_branch8(inst, X86_CC_LT, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_OVERFLOW);
			x86_patch(patch1, inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CHECK_SHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			unsigned char *patch1;
			unsigned char *patch2;
			x86_alu_reg_imm(inst, X86_CMP, reg, -32768);
			patch1 = inst;
			x86_branch8(inst, X86_CC_LE, 0, 1);
			x86_alu_reg_imm(inst, X86_CMP, reg, 32767);
			patch2 = inst;
			x86_branch8(inst, X86_CC_LE, 0, 1);
			x86_patch(patch1, inst);
			inst = throw_builtin(inst, func, JIT_RESULT_OVERFLOW);
			x86_patch(patch2, inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CHECK_USHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			unsigned char *patch1;
			x86_alu_reg_imm(inst, X86_CMP, reg, 65536);
			patch1 = inst;
			x86_branch8(inst, X86_CC_LT, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_OVERFLOW);
			x86_patch(patch1, inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CHECK_INT:
case JIT_OP_CHECK_UINT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			unsigned char *patch1;
			x86_alu_reg_imm(inst, X86_CMP, reg, 0);
			patch1 = inst;
			x86_branch8(inst, X86_CC_GE, 0, 1);
			inst = throw_builtin(inst, func, JIT_RESULT_OVERFLOW);
			x86_patch(patch1, inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOW_WORD:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value1->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		{
			jit_uint value = ((jit_uint *)(imm_value))[0];
			x86_mov_reg_imm(inst, reg, value);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			x86_mov_reg_membase(inst, reg, X86_EBP, local_offset, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			if(reg != reg2)
			{
				x86_mov_reg_reg(inst, reg, reg2, 4);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_EXPAND_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_dest_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			if(reg != reg2)
			{
				x86_mov_reg_reg(inst, reg, reg2, 4);
			}
			x86_mov_reg_reg(inst, other_reg, reg, 4);
			x86_shift_reg_imm(inst, X86_SAR, other_reg, 31);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_EXPAND_UINT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_dest_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			if(reg != reg2)
			{
				x86_mov_reg_reg(inst, reg, reg2, 4);
			}
			x86_clear_reg(inst, other_reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FLOAT32_TO_INT:
case JIT_OP_FLOAT64_TO_INT:
case JIT_OP_NFLOAT_TO_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* allocate space on the stack for 2 shorts and 1 int */
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, 8);
			/* store FPU control word */
			x86_fnstcw_membase(inst, X86_ESP, 0);
			/* set "round toward zero" mode */
			x86_mov_reg_membase(inst, reg, X86_ESP, 0, 2);
			x86_alu_reg16_imm(inst, X86_OR, reg, 0xc00);
			x86_mov_membase_reg(inst, X86_ESP, 2, reg, 2);
			x86_fldcw_membase(inst, X86_ESP, 2);
			/* convert float to int */
			x86_fist_pop_membase(inst, X86_ESP, 4, 0);
			/* restore FPU control word */
			x86_fldcw_membase(inst, X86_ESP, 0);
			/* move result to the destination */
			x86_mov_reg_membase(inst, reg, X86_ESP, 4, 4);
			/* restore the stack */
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, 8);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FLOAT32_TO_LONG:
case JIT_OP_FLOAT64_TO_LONG:
case JIT_OP_NFLOAT_TO_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_dest_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* allocate space on the stack for 2 shorts and 1 long */
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, 12);
			/* store FPU control word */
			x86_fnstcw_membase(inst, X86_ESP, 0);
			/* set "round toward zero" mode */
			x86_mov_reg_membase(inst, reg, X86_ESP, 0, 2);
			x86_alu_reg16_imm(inst, X86_OR, reg, 0xc00);
			x86_mov_membase_reg(inst, X86_ESP, 2, reg, 2);
			x86_fldcw_membase(inst, X86_ESP, 2);
			/* convert float to long */
			x86_fist_pop_membase(inst, X86_ESP, 4, 1);
			/* restore FPU control word */
			x86_fldcw_membase(inst, X86_ESP, 0);
			/* move result to the destination */
			x86_mov_reg_membase(inst, reg, X86_ESP, 4, 4);
			x86_mov_reg_membase(inst, other_reg, X86_ESP, 8, 4);
			/* restore the stack */
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, 12);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INT_TO_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint local_offset;
	if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(void *));
			x86_fild_membase(inst, X86_EBP, local_offset, 0);
			x86_fst_membase(inst, X86_ESP, 0, 0, 1);
			x86_fld_membase(inst, X86_ESP, 0, 0);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(void *));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 0);
			x86_fst_membase(inst, X86_ESP, 0, 0, 1);
			x86_fld_membase(inst, X86_ESP, 0, 0);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_nint));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INT_TO_FLOAT64:
case JIT_OP_INT_TO_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint local_offset;
	if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			x86_fild_membase(inst, X86_EBP, local_offset, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 0);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_nint));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_UINT_TO_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_clear_reg(inst, reg3);
			x86_push_reg(inst, reg3);
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 1);
			x86_fst_membase(inst, X86_ESP, 0, 0, 1);
			x86_fld_membase(inst, X86_ESP, 0, 0);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_long));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_UINT_TO_FLOAT64:
case JIT_OP_UINT_TO_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_clear_reg(inst, reg3);
			x86_push_reg(inst, reg3);
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 1);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_long));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LONG_TO_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	jit_nint local_offset;
	if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(jit_float32));
			x86_fild_membase(inst, X86_EBP, local_offset, 1);
			x86_fst_membase(inst, X86_ESP, 0, 0, 1);
			x86_fld_membase(inst, X86_ESP, 0, 0);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_float32));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_push_reg(inst, other_reg2);
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 1);
			x86_fst_membase(inst, X86_ESP, 0, 0, 1);
			x86_fld_membase(inst, X86_ESP, 0, 0);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_long));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LONG_TO_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	jit_nint local_offset;
	if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(jit_float64));
			x86_fild_membase(inst, X86_EBP, local_offset, 1);
			x86_fst_membase(inst, X86_ESP, 0, 1, 1);
			x86_fld_membase(inst, X86_ESP, 0, 1);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_float64));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_push_reg(inst, other_reg2);
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 1);
			x86_fst_membase(inst, X86_ESP, 0, 1, 1);
			x86_fld_membase(inst, X86_ESP, 0, 1);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_long));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LONG_TO_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	jit_nint local_offset;
	if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			x86_fild_membase(inst, X86_EBP, local_offset, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_push_reg(inst, other_reg2);
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 1);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_long));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ULONG_TO_FLOAT32:
case JIT_OP_ULONG_TO_FLOAT64:
case JIT_OP_ULONG_TO_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			/* TODO: review wrt relocation for elf pre-compilation */
			static float f2pow64;
			static int inited;
			unsigned char *patch;
			if(!inited)
			{
				f2pow64 = jit_float32_pow(2.0, 64);
				inited = 1;
			}
			x86_push_reg(inst, other_reg2);
			x86_push_reg(inst, reg2);
			x86_fild_membase(inst, X86_ESP, 0, 1);
			x86_test_reg_reg(inst, other_reg2, other_reg2);
			patch = inst;
			x86_branch8(inst, X86_CC_NS, 0, 1);
			x86_fp_op_mem(inst, X86_FADD, &f2pow64, 0);
			x86_patch(patch, inst);
			if(insn->opcode == JIT_OP_ULONG_TO_FLOAT32)
			{
				x86_fst_membase(inst, X86_ESP, 0, 0, 1);
				x86_fld_membase(inst, X86_ESP, 0, 0);
			}
			else if(insn->opcode == JIT_OP_ULONG_TO_FLOAT64)
			{
				x86_fst_membase(inst, X86_ESP, 0, 1, 1);
				x86_fld_membase(inst, X86_ESP, 0, 1);
			}
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_long));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FLOAT64_TO_FLOAT32:
case JIT_OP_NFLOAT_TO_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(void *));
			x86_fst_membase(inst, X86_ESP, 0, 0, 1);
			x86_fld_membase(inst, X86_ESP, 0, 0);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(void *));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_NFLOAT_TO_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(jit_float64));
			x86_fst_membase(inst, X86_ESP, 0, 1, 1);
			x86_fld_membase(inst, X86_ESP, 0, 1);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, sizeof(jit_float64));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FLOAT32_TO_NFLOAT:
case JIT_OP_FLOAT32_TO_FLOAT64:
case JIT_OP_FLOAT64_TO_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* Nothing to do: loading the value onto the FP stack is sufficient */
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IADD:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_ADD, reg, imm_value);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_ADD, reg, X86_EBP, local_offset);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_ADD, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISUB:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_SUB, reg, imm_value);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_SUB, reg, X86_EBP, local_offset);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_SUB, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IMUL:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Handle special cases of immediate multiplies */
			switch(imm_value)
			{
				case 0:
				{
					x86_clear_reg(inst, reg);
				}
				break;
	
				case 1: break;
	
				case -1:
				{
					x86_neg_reg(inst, reg);
				}
				break;
	
				case 2:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 1);
				}
				break;
	
				case 3:
				{
					/* lea reg, [reg + reg * 2] */
					x86_lea_memindex(inst, reg, reg, 0, reg, 1);
				}
				break;
	
				case 4:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 2);
				}
				break;
	
				case 5:
				{
					/* lea reg, [reg + reg * 4] */
					x86_lea_memindex(inst, reg, reg, 0, reg, 2);
				}
				break;
	
				case 6:
				{
					/* lea reg, [reg + reg * 2]; add reg, reg */
					x86_lea_memindex(inst, reg, reg, 0, reg, 1);
					x86_alu_reg_reg(inst, X86_ADD, reg, reg);
				}
				break;
	
				case 8:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 3);
				}
				break;
	
				case 9:
				{
					/* lea reg, [reg + reg * 8] */
					x86_lea_memindex(inst, reg, reg, 0, reg, 3);
				}
				break;
	
				case 10:
				{
					/* lea reg, [reg + reg * 4]; add reg, reg */
					x86_lea_memindex(inst, reg, reg, 0, reg, 2);
					x86_alu_reg_reg(inst, X86_ADD, reg, reg);
				}
				break;
	
				case 12:
				{
					/* lea reg, [reg + reg * 2]; shl reg, 2 */
					x86_lea_memindex(inst, reg, reg, 0, reg, 1);
					x86_shift_reg_imm(inst, X86_SHL, reg, 2);
				}
				break;
	
				case 16:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 4);
				}
				break;
	
				case 25:
				{
					/* lea reg, [reg + reg * 4]; lea reg, [reg + reg * 4] */
					x86_lea_memindex(inst, reg, reg, 0, reg, 2);
					x86_lea_memindex(inst, reg, reg, 0, reg, 2);
				}
				break;
	
				case 32:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 5);
				}
				break;
	
				case 64:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 6);
				}
				break;
	
				case 100:
				{
					/* lea reg, [reg + reg * 4]; shl reg, 2;
					   lea reg, [reg + reg * 4] */
					x86_lea_memindex(inst, reg, reg, 0, reg, 2);
					x86_shift_reg_imm(inst, X86_SHL, reg, 2);
					x86_lea_memindex(inst, reg, reg, 0, reg, 2);
				}
				break;
	
				case 128:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 7);
				}
				break;
	
				case 256:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 8);
				}
				break;
	
				case 512:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 9);
				}
				break;
	
				case 1024:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 10);
				}
				break;
	
				case 2048:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 11);
				}
				break;
	
				case 4096:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 12);
				}
				break;
	
				case 8192:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 13);
				}
				break;
	
				case 16384:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 14);
				}
				break;
	
				case 32768:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 15);
				}
				break;
	
				case 65536:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 16);
				}
				break;
	
				case 0x00020000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 17);
				}
				break;
	
				case 0x00040000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 18);
				}
				break;
	
				case 0x00080000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 19);
				}
				break;
	
				case 0x00100000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 20);
				}
				break;
	
				case 0x00200000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 21);
				}
				break;
	
				case 0x00400000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 22);
				}
				break;
	
				case 0x00800000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 23);
				}
				break;
	
				case 0x01000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 24);
				}
				break;
	
				case 0x02000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 25);
				}
				break;
	
				case 0x04000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 26);
				}
				break;
	
				case 0x08000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 27);
				}
				break;
	
				case 0x10000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 28);
				}
				break;
	
				case 0x20000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 29);
				}
				break;
	
				case 0x40000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 30);
				}
				break;
	
				case (jit_nint)0x80000000:
				{
					x86_shift_reg_imm(inst, X86_SHL, reg, 31);
				}
				break;
	
				default:
				{
					x86_imul_reg_reg_imm(inst, reg, reg, imm_value);
				}
				break;
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_imul_reg_membase(inst, reg, X86_EBP, local_offset);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_imul_reg_reg(inst, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IDIV:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_gen_check_space(gen, 128);
		inst = (unsigned char *)(gen->ptr);
		{
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->value2->is_constant && (insn->value2->address == 1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address == -1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Dividing by -1 gives an exception if the argument
			   is minint, or simply negates for other values */
			unsigned char *patch;
			x86_alu_reg_imm(inst, X86_CMP, reg, jit_min_int);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_ARITHMETIC);
			x86_patch(patch, inst);
			x86_neg_reg(inst, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address == 2))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_mov_reg_reg(inst, reg2, reg, 4);
			x86_shift_reg_imm(inst, X86_SHR, reg2, 0x1f);
			x86_alu_reg_reg(inst, X86_ADD, reg, reg2);
			x86_shift_reg_imm(inst, X86_SAR, reg, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && ((insn->value2->address > 0) && (((jit_nuint)insn->value2->address) & (((jit_nuint)insn->value2->address) - 1)) == 0))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			/* x & (x - 1) is equal to zero if x is a power of 2  */
			/* This code is generated by gcc for pentium. */
			/* We use this code because cmov is not available on all i386 cpus */
			jit_nuint shift, temp, value = imm_value >> 1;
			for(shift = 0; value; value >>= 1)
			{
			    ++shift;
			}
			temp = 32 - shift;
			x86_mov_reg_reg(inst, reg2, reg, 4);
			x86_shift_reg_imm(inst, X86_SAR, reg2, 0x1f);
			x86_shift_reg_imm(inst, X86_SHR, reg2, temp);
			x86_alu_reg_reg(inst, X86_ADD, reg, reg2);
			x86_shift_reg_imm(inst, X86_SAR, reg, shift);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 1, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 1)].cpu_reg;
		{
			x86_mov_reg_imm(inst, reg2, imm_value);
			x86_cdq(inst);
			x86_div_reg(inst, reg2, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 0, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			unsigned char *patch, *patch2;
	#ifndef JIT_USE_SIGNALS
			x86_alu_reg_reg(inst, X86_OR, reg2, reg2);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
			x86_patch(patch, inst);
	#endif
			x86_alu_reg_imm(inst, X86_CMP, reg2, -1);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			x86_alu_reg_imm(inst, X86_CMP, reg, jit_min_int);
			patch2 = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_ARITHMETIC);
			x86_patch(patch, inst);
			x86_patch(patch2, inst);
			x86_cdq(inst);
			x86_div_reg(inst, reg2, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IDIV_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_gen_check_space(gen, 128);
		inst = (unsigned char *)(gen->ptr);
		{
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->value2->is_constant && (insn->value2->address == 1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && ((((jit_nuint)insn->value2->address) & (((jit_nuint)insn->value2->address) - 1)) == 0))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* x & (x - 1) is equal to zero if x is a power of 2  */
			jit_nuint shift, value = imm_value >> 1;
			for(shift = 0; value; value >>= 1)
			{
			    ++shift;
			}
			x86_shift_reg_imm(inst, X86_SHR, reg, shift);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 1, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 1)].cpu_reg;
		{
			x86_mov_reg_imm(inst, reg2, imm_value);
			x86_clear_reg(inst, X86_EDX);
			x86_div_reg(inst, reg2, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 0, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
	#ifndef JIT_USE_SIGNALS
			unsigned char *patch;
			x86_alu_reg_reg(inst, X86_OR, reg2, reg2);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
			x86_patch(patch, inst);
	#endif
			x86_clear_reg(inst, X86_EDX);
			x86_div_reg(inst, reg2, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IREM:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_gen_check_space(gen, 128);
		inst = (unsigned char *)(gen->ptr);
		{
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->value2->is_constant && (insn->value2->address == 1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_clear_reg(inst, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address == -1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Dividing by -1 gives an exception if the argument
			   is minint, or simply gives a remainder of zero */
			unsigned char *patch;
			x86_alu_reg_imm(inst, X86_CMP, reg, jit_min_int);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_ARITHMETIC);
			x86_patch(patch, inst);
			x86_clear_reg(inst, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_set_dest(gen, &regs, _jit_regs_lookup("edx"), -1);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_CLOBBER, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 1, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 1)].cpu_reg;
		{
			x86_mov_reg_imm(inst, reg3, imm_value);
			x86_cdq(inst);
			x86_div_reg(inst, reg3, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_set_dest(gen, &regs, _jit_regs_lookup("edx"), -1);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_CLOBBER, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 0, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			unsigned char *patch, *patch2;
	#ifndef JIT_USE_SIGNALS
			x86_alu_reg_reg(inst, X86_OR, reg3, reg3);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
			x86_patch(patch, inst);
	#endif
			x86_alu_reg_imm(inst, X86_CMP, reg3, -1);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			x86_alu_reg_imm(inst, X86_CMP, reg2, jit_min_int);
			patch2 = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_ARITHMETIC);
			x86_patch(patch, inst);
			x86_patch(patch2, inst);
			x86_cdq(inst);
			x86_div_reg(inst, reg3, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IREM_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_gen_check_space(gen, 128);
		inst = (unsigned char *)(gen->ptr);
		{
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->value2->is_constant && (insn->value2->address == 1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_clear_reg(inst, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && ((((jit_nuint)insn->value2->address) & (((jit_nuint)insn->value2->address) - 1)) == 0))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* x & (x - 1) is equal to zero if x is a power of 2  */
			x86_alu_reg_imm(inst, X86_AND, reg, imm_value - 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_set_dest(gen, &regs, _jit_regs_lookup("edx"), -1);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_CLOBBER, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 1, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 1)].cpu_reg;
		{
			x86_mov_reg_imm(inst, reg3, imm_value);
			x86_clear_reg(inst, X86_EDX);
			x86_div_reg(inst, reg3, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_set_dest(gen, &regs, _jit_regs_lookup("edx"), -1);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_CLOBBER, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 0, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
	#ifndef JIT_USE_SIGNALS
			unsigned char *patch;
			x86_alu_reg_reg(inst, X86_OR, reg3, reg3);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_DIVISION_BY_ZERO);
			x86_patch(patch, inst);
	#endif
			x86_clear_reg(inst, X86_EDX);
			x86_div_reg(inst, reg3, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INEG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_neg_reg(inst, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LADD:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			jit_int value1 = ((jit_int *)(imm_value))[0];
			jit_int value2 = ((jit_int *)(imm_value))[1];
			if(value1 != 0)
			{
				x86_alu_reg_imm(inst, X86_ADD, reg, value1);
				x86_alu_reg_imm(inst, X86_ADC, other_reg, value2);
			}
			else
			{
				x86_alu_reg_imm(inst, X86_ADD, other_reg, value2);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_ADD, reg, X86_EBP, local_offset);
			x86_alu_reg_membase(inst, X86_ADC, other_reg, X86_EBP, local_offset + 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value2(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_ADD, reg, reg2);
			x86_alu_reg_reg(inst, X86_ADC, other_reg, other_reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LSUB:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			jit_int value1 = ((jit_int *)(imm_value))[0];
			jit_int value2 = ((jit_int *)(imm_value))[1];
			if(value1 != 0)
			{
				x86_alu_reg_imm(inst, X86_SUB, reg, value1);
				x86_alu_reg_imm(inst, X86_SBB, other_reg, value2);
			}
			else
			{
				x86_alu_reg_imm(inst, X86_SUB, other_reg, value2);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_SUB, reg, X86_EBP, local_offset);
			x86_alu_reg_membase(inst, X86_SBB, other_reg, X86_EBP, local_offset + 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value2(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_SUB, reg, reg2);
			x86_alu_reg_reg(inst, X86_SBB, other_reg, other_reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LNEG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			/* TODO: gcc generates the first variant while
			   AoA suggests the second. Figure out if one
			   is better than other. */
	#if 1
			x86_neg_reg(inst, reg);
			x86_alu_reg_imm(inst, X86_ADC, other_reg, 0);
			x86_neg_reg(inst, other_reg);
	#else
			x86_neg_reg(inst, other_reg);
			x86_neg_reg(inst, reg);
			x86_alu_reg_imm(inst, X86_SBB, other_reg, 0);
	#endif
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FADD:
case JIT_OP_DADD:
case JIT_OP_NFADD:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK | _JIT_REGS_COMMUTATIVE | _JIT_REGS_X87_ARITH);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			int flags;
	
			flags = _jit_regs_select(&regs);
	
			if((flags & _JIT_REGS_NO_POP) == 0)
			{
				x86_fp_op_reg(inst, X86_FADD,
					fp_stack_index(gen, reg + JIT_REG_STACK_START), 1);
			}
			else if((flags & _JIT_REGS_FLIP_ARGS) != 0)
			{
				x86_fp_op_reg(inst, X86_FADD,
					fp_stack_index(gen, reg + JIT_REG_STACK_START), 0);
			}
			else
			{
				x86_fp_op(inst, X86_FADD,
					fp_stack_index(gen, reg2 + JIT_REG_STACK_START));
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FSUB:
case JIT_OP_DSUB:
case JIT_OP_NFSUB:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK | _JIT_REGS_X87_ARITH | _JIT_REGS_REVERSIBLE);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			int flags;
	
			flags = _jit_regs_select(&regs);
	
			if((flags & _JIT_REGS_NO_POP) == 0)
			{
				if((flags & _JIT_REGS_REVERSE) == 0)
				{
					x86_fp_op_reg(inst, X86_FSUB,
						fp_stack_index(gen, reg + JIT_REG_STACK_START), 1);
				}
				else
				{
					x86_fp_op_reg(inst, X86_FSUBR,
						fp_stack_index(gen, reg2 + JIT_REG_STACK_START), 1);
				}
			}
			else if((flags & _JIT_REGS_FLIP_ARGS) != 0)
			{
				if((flags & _JIT_REGS_REVERSE) == 0)
				{
					x86_fp_op_reg(inst, X86_FSUB,
						fp_stack_index(gen, reg + JIT_REG_STACK_START), 0);
				}
				else
				{
					x86_fp_op(inst, X86_FSUBR,
						fp_stack_index(gen, reg + JIT_REG_STACK_START));
				}
			}
			else
			{
				if((flags & _JIT_REGS_REVERSE) == 0)
				{
					x86_fp_op(inst, X86_FSUB,
						fp_stack_index(gen, reg2 + JIT_REG_STACK_START));
				}
				else
				{
					x86_fp_op_reg(inst, X86_FSUBR,
						fp_stack_index(gen, reg2 + JIT_REG_STACK_START), 0);
				}
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FMUL:
case JIT_OP_DMUL:
case JIT_OP_NFMUL:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK | _JIT_REGS_COMMUTATIVE | _JIT_REGS_X87_ARITH);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			int flags;
	
			flags = _jit_regs_select(&regs);
	
			if((flags & _JIT_REGS_NO_POP) == 0)
			{
				x86_fp_op_reg(inst, X86_FMUL, fp_stack_index(gen, reg + JIT_REG_STACK_START), 1);
			}
			else if((flags & _JIT_REGS_FLIP_ARGS) != 0)
			{
				x86_fp_op_reg(inst, X86_FMUL, fp_stack_index(gen, reg + JIT_REG_STACK_START), 0);
			}
			else
			{
				x86_fp_op(inst, X86_FMUL, fp_stack_index(gen, reg2 + JIT_REG_STACK_START));
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FDIV:
case JIT_OP_DDIV:
case JIT_OP_NFDIV:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK | _JIT_REGS_X87_ARITH | _JIT_REGS_REVERSIBLE);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			int flags;
	
			flags = _jit_regs_select(&regs);
	
			if((flags & _JIT_REGS_NO_POP) == 0)
			{
				if((flags & _JIT_REGS_REVERSE) == 0)
				{
					x86_fp_op_reg(inst, X86_FDIV,
						fp_stack_index(gen, reg + JIT_REG_STACK_START), 1);
				}
				else
				{
					x86_fp_op_reg(inst, X86_FDIVR,
						fp_stack_index(gen, reg2 + JIT_REG_STACK_START), 1);
				}
			}
			else if((flags & _JIT_REGS_FLIP_ARGS) != 0)
			{
				if((flags & _JIT_REGS_REVERSE) == 0)
				{
					x86_fp_op_reg(inst, X86_FDIV,
						fp_stack_index(gen, reg + JIT_REG_STACK_START), 0);
				}
				else
				{
					x86_fp_op(inst, X86_FDIVR,
						fp_stack_index(gen, reg + JIT_REG_STACK_START));
				}
			}
			else
			{
				if((flags & _JIT_REGS_REVERSE) == 0)
				{
					x86_fp_op(inst, X86_FDIV,
						fp_stack_index(gen, reg2 + JIT_REG_STACK_START));
				}
				else
				{
					x86_fp_op_reg(inst, X86_FDIVR,
						fp_stack_index(gen, reg2 + JIT_REG_STACK_START), 0);
				}
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FREM:
case JIT_OP_DREM:
case JIT_OP_NFREM:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 0, _jit_regs_lookup("eax"));
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			unsigned char *label;
			label = inst;
			x86_fprem(inst);
			x86_fnstsw(inst);
			x86_alu_reg_imm(inst, X86_AND, X86_EAX, 0x0400);
			x86_branch(inst, X86_CC_NZ, label, 0);
			x86_fstp(inst, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FNEG:
case JIT_OP_DNEG:
case JIT_OP_NFNEG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_fchs(inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IAND:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_AND, reg, imm_value);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_AND, reg, X86_EBP, local_offset);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_AND, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IOR:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_OR, reg, imm_value);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_OR, reg, X86_EBP, local_offset);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IXOR:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_XOR, reg, imm_value);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_XOR, reg, X86_EBP, local_offset);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_XOR, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INOT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_not_reg(inst, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISHL:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_shift_reg_imm(inst, X86_SHL, reg, (imm_value & 0x1F));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_set_value2(gen, &regs, _jit_regs_lookup("ecx"), -1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_shift_reg(inst, X86_SHL, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISHR:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_shift_reg_imm(inst, X86_SAR, reg, (imm_value & 0x1F));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_set_value2(gen, &regs, _jit_regs_lookup("ecx"), -1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_shift_reg(inst, X86_SAR, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISHR_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_shift_reg_imm(inst, X86_SHR, reg, (imm_value & 0x1F));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_set_value2(gen, &regs, _jit_regs_lookup("ecx"), -1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_shift_reg(inst, X86_SHR, reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LAND:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			jit_int value1 = ((jit_int *)(imm_value))[0];
			jit_int value2 = ((jit_int *)(imm_value))[1];
			x86_alu_reg_imm(inst, X86_AND, reg, value1);
			x86_alu_reg_imm(inst, X86_AND, other_reg, value2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_AND, reg, X86_EBP, local_offset);
			x86_alu_reg_membase(inst, X86_AND, other_reg, X86_EBP, local_offset + 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value2(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_AND, reg, reg2);
			x86_alu_reg_reg(inst, X86_AND, other_reg, other_reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOR:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			jit_int value1 = ((jit_int *)(imm_value))[0];
			jit_int value2 = ((jit_int *)(imm_value))[1];
			x86_alu_reg_imm(inst, X86_OR, reg, value1);
			x86_alu_reg_imm(inst, X86_OR, other_reg, value2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_OR, reg, X86_EBP, local_offset);
			x86_alu_reg_membase(inst, X86_OR, other_reg, X86_EBP, local_offset + 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value2(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, reg2);
			x86_alu_reg_reg(inst, X86_OR, other_reg, other_reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LXOR:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			jit_int value1 = ((jit_int *)(imm_value))[0];
			jit_int value2 = ((jit_int *)(imm_value))[1];
			x86_alu_reg_imm(inst, X86_XOR, reg, value1);
			x86_alu_reg_imm(inst, X86_XOR, other_reg, value2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_XOR, reg, X86_EBP, local_offset);
			x86_alu_reg_membase(inst, X86_XOR, other_reg, X86_EBP, local_offset + 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COMMUTATIVE);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value2(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_XOR, reg, reg2);
			x86_alu_reg_reg(inst, X86_XOR, other_reg, other_reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LNOT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_not_reg(inst, reg);
			x86_not_reg(inst, other_reg);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR:
{
	unsigned char * inst;
	{
		_jit_regs_spill_all(gen);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			inst = output_branch(func, inst, 0xEB /* jmp */, insn);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_BR_IFALSE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, reg);
			inst = output_branch(func, inst, 0x74 /* eq */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ITRUE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, reg);
			inst = output_branch(func, inst, 0x75 /* ne */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IEQ:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, reg);
			inst = output_branch(func, inst, 0x74 /* eq */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x74 /* eq */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x74 /* eq */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x74 /* eq */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_INE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, reg);
			inst = output_branch(func, inst, 0x75 /* ne */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x75 /* ne */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x75 /* ne */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x75 /* ne */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x7C /* lt */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x7C /* lt */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x7C /* lt */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILT_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x72 /* lt_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x72 /* lt_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x72 /* lt_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x7E /* le */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x7E /* le */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x7E /* le */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILE_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x76 /* le_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x76 /* le_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x76 /* le_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x7F /* gt */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x7F /* gt */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x7F /* gt */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGT_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x77 /* gt_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x77 /* gt_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x77 /* gt_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x7D /* ge */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x7D /* ge */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x7D /* ge */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGE_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg, imm_value);
			inst = output_branch(func, inst, 0x73 /* ge_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg, X86_EBP, local_offset);
			inst = output_branch(func, inst, 0x73 /* ge_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg, reg2);
			inst = output_branch(func, inst, 0x73 /* ge_un */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_LFALSE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, other_reg);
			inst = output_branch(func, inst, 0x74 /* eq */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_LTRUE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg, other_reg);
			inst = output_branch(func, inst, 0x75 /* ne */, insn);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IEQ:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg2, reg2);
			inst = setcc_reg(inst, reg, X86_CC_EQ, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_EQ, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_EQ, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_EQ, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_OR, reg2, reg2);
			inst = setcc_reg(inst, reg, X86_CC_NE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_NE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_NE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_NE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_LT, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_LT, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_LT, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILT_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_LT, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_LT, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_LT, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_LE, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_LE, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_LE, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILE_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_LE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_LE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_LE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_GT, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_GT, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_GT, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGT_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_GT, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_GT, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_GT, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_GE, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_GE, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_GE, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGE_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_alu_reg_imm(inst, X86_CMP, reg2, imm_value);
			inst = setcc_reg(inst, reg, X86_CC_GE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		{
			x86_alu_reg_membase(inst, X86_CMP, reg2, X86_EBP, local_offset);
			inst = setcc_reg(inst, reg, X86_CC_GE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_CMP, reg2, reg3);
			inst = setcc_reg(inst, reg, X86_CC_GE, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FATAN:
case JIT_OP_DATAN:
case JIT_OP_NFATAN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_add_scratch(&regs, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_fld1(inst);
			x86_fpatan(inst);
			x86_fldz(inst);
			x86_fp_op_reg(inst, X86_FADD, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FCOS:
case JIT_OP_DCOS:
case JIT_OP_NFCOS:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_add_scratch(&regs, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_fcos(inst);
			x86_fldz(inst);
			x86_fp_op_reg(inst, X86_FADD, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FSIN:
case JIT_OP_DSIN:
case JIT_OP_NFSIN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_add_scratch(&regs, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_fsin(inst);
			x86_fldz(inst);
			x86_fp_op_reg(inst, X86_FADD, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FSQRT:
case JIT_OP_DSQRT:
case JIT_OP_NFSQRT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_fsqrt(inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IABS:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_set_scratch(gen, &regs, 0, _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_cdq(inst);
			x86_alu_reg_reg(inst, X86_XOR, reg, reg2);
			x86_alu_reg_reg(inst, X86_SUB, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LABS:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_mov_reg_reg(inst, reg2, other_reg, 4);
			x86_shift_reg_imm(inst, X86_SAR, reg2, 31);
			x86_alu_reg_reg(inst, X86_XOR, reg, reg2);
			x86_alu_reg_reg(inst, X86_XOR, other_reg, reg2);
			x86_alu_reg_reg(inst, X86_SUB, reg, reg2);
			x86_alu_reg_reg(inst, X86_SBB, other_reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FABS:
case JIT_OP_DABS:
case JIT_OP_NFABS:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_fabs(inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IMIN_UN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_SUB, reg2, reg3);
			x86_alu_reg_reg(inst, X86_SBB, reg, reg);
			x86_alu_reg_reg(inst, X86_AND, reg, reg2);
			x86_alu_reg_reg(inst, X86_ADD, reg, reg3);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISIGN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value1->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		{
			if(imm_value < 0)
			{
				x86_mov_reg_imm(inst, reg, -1);
			}
			else if(imm_value > 0)
			{
				x86_mov_reg_imm(inst, reg, 1);
			}
			else
			{
				x86_clear_reg(inst, reg);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_clear_reg(inst, reg);
			x86_test_reg_reg(inst, reg2, reg2);
			x86_set_reg(inst, X86_CC_NZ, reg, 0);
			x86_shift_reg_imm(inst, X86_SAR, reg2, 31);
			x86_alu_reg_reg(inst, X86_OR, reg, reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LSIGN:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	jit_nint imm_value;
	if(insn->value1->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		{
			jit_long value = *((jit_long *)(imm_value));
			if(value < 0)
			{
				x86_mov_reg_imm(inst, reg, -1);
			}
			else if(value > 0)
			{
				x86_mov_reg_imm(inst, reg, 1);
			}
			else
			{
				x86_clear_reg(inst, reg);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_clear_reg(inst, reg);
			x86_alu_reg_reg(inst, X86_OR, reg2, other_reg2);
			x86_set_reg(inst, X86_CC_NZ, reg, 0);
			x86_shift_reg_imm(inst, X86_SAR, other_reg2, 31);
			x86_alu_reg_reg(inst, X86_OR, reg, other_reg2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CHECK_NULL:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
	#if 0 && defined(JIT_USE_SIGNALS)
			/* if reg contains NULL this generates SEGV and the signal
			   handler will throw the exception  */
			x86_alu_reg_membase(inst, X86_CMP, reg, reg, 0);
	#else
			unsigned char *patch;
			x86_alu_reg_reg(inst, X86_OR, reg, reg);
			patch = inst;
			x86_branch8(inst, X86_CC_NE, 0, 0);
			inst = throw_builtin(inst, func, JIT_RESULT_NULL_REFERENCE);
			x86_patch(patch, inst);
	#endif
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CALL:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			jit_function_t func = (jit_function_t)(insn->dest);
			x86_call_code(inst, jit_function_to_closure(func));
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_TAIL:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			jit_function_t func = (jit_function_t)(insn->dest);
			x86_mov_reg_reg(inst, X86_ESP, X86_EBP, sizeof(void *));
			x86_pop_reg(inst, X86_EBP);
			x86_jump_code(inst, jit_function_to_closure(func));
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_INDIRECT:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			x86_call_reg(inst, X86_EAX);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_INDIRECT_TAIL:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			x86_mov_reg_reg(inst, X86_ESP, X86_EBP, sizeof(void *));
			x86_pop_reg(inst, X86_EBP);
			x86_jump_reg(inst, X86_EAX);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_VTABLE_PTR:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			x86_call_reg(inst, X86_EAX);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_VTABLE_PTR_TAIL:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			x86_mov_reg_reg(inst, X86_ESP, X86_EBP, sizeof(void *));
			x86_pop_reg(inst, X86_EBP);
			x86_jump_reg(inst, X86_EAX);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_EXTERNAL:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			x86_call_code(inst, (void *)(insn->dest));
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_EXTERNAL_TAIL:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			x86_mov_reg_reg(inst, X86_ESP, X86_EBP, sizeof(void *));
			x86_pop_reg(inst, X86_EBP);
			x86_jump_code(inst, (void *)(insn->dest));
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_RETURN:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			inst = jump_to_epilog(gen, inst, block);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_RETURN_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), -1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			inst = jump_to_epilog(gen, inst, block);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_RETURN_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_set_value1(gen, &regs, _jit_regs_lookup("eax"), _jit_regs_lookup("edx"));
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			inst = jump_to_epilog(gen, inst, block);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_RETURN_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_clobber_class(gen, &regs, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* clobber(freg) frees all registers on the fp stack */
			inst = jump_to_epilog(gen, inst, block);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_RETURN_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_clobber_class(gen, &regs, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* clobber(freg) frees all registers on the fp stack */
			inst = jump_to_epilog(gen, inst, block);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_RETURN_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_clobber_class(gen, &regs, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* clobber(freg) frees all registers on the fp stack */
			inst = jump_to_epilog(gen, inst, block);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_RETURN_SMALL_STRUCT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			switch(imm_value)
			{
			case 1:
				x86_widen_membase(inst, X86_EAX, reg, 0, 0, 0);
				break;
	
			case 2:
				x86_widen_membase(inst, X86_EAX, reg, 0, 0, 1);
				break;
	
			case 3:
				if(X86_EAX == reg)
				{
					x86_widen_membase(inst, X86_EDX, reg, 0, 0, 1);
					x86_widen_membase(inst, X86_EAX, reg, 2, 0, 0);
				}
				else
				{
					x86_widen_membase(inst, X86_EAX, reg, 2, 0, 0);
					x86_widen_membase(inst, X86_EDX, reg, 0, 0, 1);
				}
				x86_shift_reg_imm(inst, X86_SHL, X86_EAX, 16);
				x86_alu_reg_reg(inst, X86_OR, X86_EAX, X86_EDX);
				break;
	
			case 4:
				x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
				break;
	
			case 5:
				if(X86_EAX == reg)
				{
					x86_widen_membase(inst, X86_EDX, reg, 4, 0, 0);
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
				}
				else
				{
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
					x86_widen_membase(inst, X86_EDX, reg, 4, 0, 0);
				}
				break;
	
			case 6:
				if(X86_EAX == reg)
				{
					x86_widen_membase(inst, X86_EDX, reg, 4, 0, 1);
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
				}
				else
				{
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
					x86_widen_membase(inst, X86_EDX, reg, 4, 0, 1);
				}
				break;
	
			case 7:
				if(X86_EAX == reg)
				{
					x86_widen_membase(inst, X86_ECX, reg, 4, 0, 1);
					x86_widen_membase(inst, X86_EDX, reg, 6, 0, 0);
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
				}
				else if(X86_ECX == reg)
				{
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
					x86_widen_membase(inst, X86_EDX, reg, 6, 0, 0);
					x86_widen_membase(inst, X86_ECX, reg, 4, 0, 1);
				}
				else
				{
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
					x86_widen_membase(inst, X86_ECX, reg, 4, 0, 1);
					x86_widen_membase(inst, X86_EDX, reg, 6, 0, 0);
				}
				x86_shift_reg_imm(inst, X86_SHL, X86_EDX, 16);
				x86_alu_reg_reg(inst, X86_OR, X86_EDX, X86_ECX);
				break;
	
			case 8:
				if(X86_EAX == reg)
				{
					x86_mov_reg_membase(inst, X86_EDX, reg, 4, 4);
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
				}
				else
				{
					x86_mov_reg_membase(inst, X86_EAX, reg, 0, 4);
					x86_mov_reg_membase(inst, X86_EDX, reg, 4, 4);
				}
				break;
			}
	
			inst = jump_to_epilog(gen, inst, block);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_SETUP_FOR_NESTED:
{
	unsigned char * inst;
	{
		_jit_regs_spill_all(gen);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			jit_nint nest_reg = jit_value_get_nint_constant(insn->value1);
			if(nest_reg == -1)
			{
				x86_push_reg(inst, X86_EBP);
			}
			else
			{
				x86_mov_reg_reg(inst, _jit_reg_info[nest_reg].cpu_reg,
								X86_EBP, sizeof(void *));
			}
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_SETUP_FOR_SIBLING:
{
	unsigned char * inst;
	{
		_jit_regs_spill_all(gen);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			jit_value_t parent;
			jit_nint level = jit_value_get_nint_constant(insn->value1);
			jit_nint nest_reg = jit_value_get_nint_constant(insn->value2);
			int cpu_reg;
			if(nest_reg == -1)
			{
				cpu_reg = X86_EAX;
			}
			else
			{
				cpu_reg = _jit_reg_info[nest_reg].cpu_reg;
			}
			parent = func->builder->parent_frame;
			if(parent->in_register)
			{
				x86_mov_reg_reg(inst, cpu_reg,
								_jit_reg_info[parent->reg].cpu_reg,
								sizeof(void *));
			}
			else if(parent->in_global_register)
			{
				x86_mov_reg_reg(inst, cpu_reg,
								_jit_reg_info[parent->global_reg].cpu_reg,
								sizeof(void *));
			}
			else
			{
				_jit_gen_fix_value(parent);
				x86_mov_reg_membase(inst, cpu_reg, X86_EBP,
								    parent->frame_offset, sizeof(void *));
			}
			while(level > 0)
			{
				gen->ptr = inst;
				_jit_gen_check_space(gen, 16);
				x86_mov_reg_membase(inst, cpu_reg, cpu_reg, 0, sizeof(void *));
				--level;
			}
			if(nest_reg == -1)
			{
				x86_push_reg(inst, cpu_reg);
			}
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_IMPORT:
{
		{
			unsigned char *inst;
			int reg;
			jit_nint level = jit_value_get_nint_constant(insn->value2);
			_jit_gen_fix_value(insn->value1);
			reg = _jit_regs_load_value
				(gen, func->builder->parent_frame, 1, 0);
			inst = gen->ptr;
			_jit_gen_check_space(gen, 32 + level * 8);
			reg = _jit_reg_info[reg].cpu_reg;
			while(level > 0)
			{
				x86_mov_reg_membase(inst, reg, reg, 0, sizeof(void *));
				--level;
			}
			if(insn->value1->frame_offset != 0)
			{
				x86_alu_reg_imm(inst, X86_ADD, reg, insn->value1->frame_offset);
			}
			gen->ptr = inst;
		}
}
break;

case JIT_OP_THROW:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_push_reg(inst, reg);
			if(func->builder->setjmp_value != 0)
			{
				/* We have a "setjmp" block in the current function,
				   so we must record the location of the throw first */
				_jit_gen_fix_value(func->builder->setjmp_value);
				if(func->builder->position_independent)
				{
					x86_call_imm(inst, 0);
					x86_pop_membase(inst, X86_EBP,
							func->builder->setjmp_value->frame_offset
							+ jit_jmp_catch_pc_offset);
				}
				else
				{
					int pc = (int) inst;
					x86_mov_membase_imm(inst, X86_EBP,
							    func->builder->setjmp_value->frame_offset
							    + jit_jmp_catch_pc_offset, pc, 4);
				}
			}
			x86_call_code(inst, (void *)jit_exception_throw);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_RETHROW:
{
		{ /* Not used in native code back ends */ }
}
break;

case JIT_OP_LOAD_PC:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		{
			if(func->builder->position_independent)
			{
				x86_call_imm(inst, 0);
				x86_pop_reg(inst, reg);
			}
			else
			{
				int pc = (int) inst;
				x86_mov_reg_imm(inst, reg, pc);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_EXCEPTION_PC:
{
		{ /* Not used in native code back ends */ }
}
break;

case JIT_OP_ENTER_FINALLY:
{
		{ /* Nothing to do here: return address on the stack */ }
}
break;

case JIT_OP_LEAVE_FINALLY:
{
	unsigned char * inst;
	{
		_jit_regs_spill_all(gen);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			/* The "finally" return address is on the stack */
			x86_ret(inst);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_CALL_FINALLY:
{
	unsigned char * inst;
	{
		_jit_regs_spill_all(gen);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			jit_block_t block;
	
			block = jit_block_from_label(func, (jit_label_t)(insn->dest));
			if(!block)
			{
				return;
			}
	
			if(block->address)
			{
				x86_call_code(inst, block->address);
			}
			else
			{
				x86_call_imm(inst, block->fixup_list);
				block->fixup_list = (void *)(inst - 4);
			}
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_ENTER_FILTER:
{
		{
			/* TODO */
			TODO();
		}
}
break;

case JIT_OP_LEAVE_FILTER:
{
		{
			/* TODO */
			TODO();
		}
}
break;

case JIT_OP_CALL_FILTER:
{
		{
			/* TODO */
			TODO();
		}
}
break;

case JIT_OP_CALL_FILTER_RETURN:
{
		{
			/* TODO */
			TODO();
		}
}
break;

case JIT_OP_ADDRESS_OF_LABEL:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		{
			block = jit_block_from_label(func, (jit_label_t)(insn->value1));
			if(func->builder->position_independent)
			{
				/* TODO */
				TODO();
			}
			else
			{
				if(block->address)
				{
					x86_mov_reg_imm(inst, reg, block->address);
				}
				else
				{
					/* Output a placeholder and record on the block's fixup list */
					x86_mov_reg_imm(inst, reg, (int)(block->fixup_absolute_list));
					block->fixup_absolute_list = (void *)(inst - 4);
				}
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LOAD_SBYTE:
case JIT_OP_COPY_LOAD_UBYTE:
case JIT_OP_COPY_STORE_BYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(!insn->dest->is_constant && !insn->dest->in_register && !insn->dest->has_global_register&& (insn->flags & JIT_INSN_DEST_NEXT_USE) == 0 && insn->value1->is_constant)
	{
		_jit_gen_fix_value(insn->dest);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		imm_value = insn->value1->address;
		{
			x86_mov_membase_imm(inst, X86_EBP, local_offset, imm_value, 1);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(!insn->dest->is_constant && !insn->dest->in_register && !insn->dest->has_global_register&& (insn->flags & JIT_INSN_DEST_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST | _JIT_REGS_COPY);
		_jit_gen_fix_value(insn->dest);
		_jit_regs_init_value1(&regs, insn, 0, x86_breg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_mov_membase_reg(inst, X86_EBP, local_offset, reg, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LOAD_SHORT:
case JIT_OP_COPY_LOAD_USHORT:
case JIT_OP_COPY_STORE_SHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(!insn->dest->is_constant && !insn->dest->in_register && !insn->dest->has_global_register&& (insn->flags & JIT_INSN_DEST_NEXT_USE) == 0 && insn->value1->is_constant)
	{
		_jit_gen_fix_value(insn->dest);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		imm_value = insn->value1->address;
		{
			x86_mov_membase_imm(inst, X86_EBP, local_offset, imm_value, 2);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(!insn->dest->is_constant && !insn->dest->in_register && !insn->dest->has_global_register&& (insn->flags & JIT_INSN_DEST_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST | _JIT_REGS_COPY);
		_jit_gen_fix_value(insn->dest);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_mov_membase_reg(inst, X86_EBP, local_offset, reg, 2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(!insn->dest->is_constant && !insn->dest->in_register && !insn->dest->has_global_register&& (insn->flags & JIT_INSN_DEST_NEXT_USE) == 0 && insn->value1->is_constant)
	{
		_jit_gen_fix_value(insn->dest);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		imm_value = insn->value1->address;
		{
			x86_mov_membase_imm(inst, X86_EBP, local_offset, imm_value, 4);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_STRUCT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	jit_nint local_offset, local_offset2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_force_out(gen, insn->dest, 1);
		_jit_gen_fix_value(insn->dest);
		_jit_regs_force_out(gen, insn->value1, 0);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_clobber_class(gen, &regs, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		local_offset2 = insn->value1->frame_offset;
		{
			inst = memory_copy(gen, inst, X86_EBP, local_offset, X86_EBP, local_offset2,
					   jit_type_get_size(jit_value_get_type(insn->dest)));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ADDRESS_OF:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint local_offset;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_force_out(gen, insn->value1, 0);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			x86_lea_membase(inst, reg, X86_EBP, local_offset);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INCOMING_REG:
case JIT_OP_RETURN_REG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/*
			 * This rule does nothing itself. Also at this point
			 * the value is supposed to be already in the register
			 * so the "reg" pattern does not load it either. But
			 * it allows the allocator to check the liveness flags
			 * and free the register if the value is dead.
			 */
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value1->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value1->address;
		{
			x86_push_imm(inst, imm_value);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_gen_fix_value(insn->value1);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->value1->frame_offset;
		{
			x86_push_membase(inst, X86_EBP, local_offset);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_push_reg(inst, reg);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value1->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value1->address;
		{
			x86_push_imm(inst, ((jit_int *)(imm_value))[1]);
			x86_push_imm(inst, ((jit_int *)(imm_value))[0]);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_gen_fix_value(insn->value1);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->value1->frame_offset;
		{
			x86_push_membase(inst, X86_EBP, local_offset + 4);
			x86_push_membase(inst, X86_EBP, local_offset);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			x86_push_reg(inst, other_reg);
			x86_push_reg(inst, reg);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value1->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value1->address;
		{
			jit_int *ptr = (jit_int *)(imm_value);
			x86_push_imm(inst, ptr[0]);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_gen_fix_value(insn->value1);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->value1->frame_offset;
		{
			x86_push_membase(inst, X86_EBP, local_offset);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(jit_float32));
			x86_fst_membase(inst, X86_ESP, 0, 0, 1);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value1->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value1->address;
		{
			jit_int *ptr = (jit_int *)(imm_value);
			x86_push_imm(inst, ptr[1]);
			x86_push_imm(inst, ptr[0]);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_gen_fix_value(insn->value1);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->value1->frame_offset;
		{
			x86_push_membase(inst, X86_EBP, local_offset + 4);
			x86_push_membase(inst, X86_EBP, local_offset);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(jit_float64));
			x86_fst_membase(inst, X86_ESP, 0, 1, 1);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value1->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value1->address;
		{
			jit_int *ptr = (jit_int *)(imm_value);
			if(sizeof(jit_nfloat) != sizeof(jit_float64))
			{
				x86_push_imm(inst, ptr[2]);
			}
			x86_push_imm(inst, ptr[1]);
			x86_push_imm(inst, ptr[0]);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_gen_fix_value(insn->value1);
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->value1->frame_offset;
		{
			if(sizeof(jit_nfloat) != sizeof(jit_float64))
			{
				x86_push_membase(inst, X86_EBP, local_offset + 8);
			}
			x86_push_membase(inst, X86_EBP, local_offset + 4);
			x86_push_membase(inst, X86_EBP, local_offset);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_STACK);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			if(sizeof(jit_nfloat) != sizeof(jit_float64))
			{
				x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(jit_nfloat));
				x86_fst80_membase(inst, X86_ESP, 0);
			}
			else
			{
				x86_alu_reg_imm(inst, X86_SUB, X86_ESP, sizeof(jit_float64));
				x86_fst_membase(inst, X86_ESP, 0, 1, 1);
			}
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_STRUCT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_class(gen, &regs, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			jit_nuint size;
			size = (jit_nuint)jit_value_get_nint_constant(insn->value2);
			if((size % sizeof(void *)) == 0 && size <= 4 * sizeof(void *))
			{
				/* Handle small structures that are a multiple of the word size */
				while(size > 0)
				{
					size -= sizeof(void *);
					x86_push_membase(inst, reg, size);
				}
			}
			else
			{
				/* Handle arbitrary-sized structures */
				x86_alu_reg_imm(inst, X86_SUB, X86_ESP, ROUND_STACK(size));
				inst = memory_copy(gen, inst, X86_ESP, 0, reg, 0, size);
			}
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_POP_STACK:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, insn->value1->address);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_FLUSH_SMALL_STRUCT:
{
	unsigned char * inst;
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		{
			jit_nuint size;
			jit_nint offset;
			_jit_gen_fix_value(insn->value1);
			size = jit_type_get_size(jit_value_get_type(insn->value1));
			offset = insn->value1->frame_offset;
			inst = store_small_struct
				(inst, X86_EAX, X86_EDX, X86_EBP, offset, (jit_nint)size, 0);
		}
		gen->ptr = (unsigned char *)inst;
	}
}
break;

case JIT_OP_LOAD_RELATIVE_SBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_widen_membase(inst, reg, reg2, imm_value, 1, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_UBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_widen_membase(inst, reg, reg2, imm_value, 0, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_SHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_widen_membase(inst, reg, reg2, imm_value, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_USHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_widen_membase(inst, reg, reg2, imm_value, 0, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_mov_reg_membase(inst, reg, reg2, imm_value, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_dest_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			if(reg == reg2)
			{
				x86_mov_reg_membase(inst, other_reg, reg2, imm_value + 4, 4);
				x86_mov_reg_membase(inst, reg, reg2, imm_value, 4);
			}
			else
			{
				x86_mov_reg_membase(inst, reg, reg2, imm_value, 4);
				x86_mov_reg_membase(inst, other_reg, reg2, imm_value + 4, 4);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_fld_membase(inst, reg2, imm_value, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_fld_membase(inst, reg2, imm_value, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_constant && (sizeof(jit_nfloat) != sizeof(jit_float64)))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_fld80_membase(inst, reg2, imm_value);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_fld_membase(inst, reg2, imm_value, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_STRUCT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_force_out(gen, insn->dest, 1);
		_jit_gen_fix_value(insn->dest);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_class(gen, &regs, x86_reg);
		_jit_regs_begin(gen, &regs, 128);
		inst = (unsigned char *)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			inst = memory_copy(gen, inst, X86_EBP, local_offset, reg, imm_value,
					   jit_type_get_size(jit_value_get_type(insn->dest)));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_BYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2, imm_value3;
	if(insn->dest->is_constant && insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->dest->address;
		imm_value2 = insn->value1->address;
		imm_value3 = insn->value2->address;
		{
			x86_mov_mem_imm(inst, imm_value + imm_value3, imm_value2, 1);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->dest->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_value1(&regs, insn, 0, x86_breg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->dest->address;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value2 = insn->value2->address;
		{
			x86_mov_mem_reg(inst, imm_value + imm_value2, reg, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			x86_mov_membase_imm(inst, reg, imm_value2, imm_value, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_breg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_mov_membase_reg(inst, reg, imm_value, reg2, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_SHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2, imm_value3;
	if(insn->dest->is_constant && insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->dest->address;
		imm_value2 = insn->value1->address;
		imm_value3 = insn->value2->address;
		{
			x86_mov_mem_imm(inst, imm_value + imm_value3, imm_value2, 2);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->dest->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->dest->address;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value2 = insn->value2->address;
		{
			x86_mov_mem_reg(inst, imm_value + imm_value2, reg, 2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			x86_mov_membase_imm(inst, reg, imm_value2, imm_value, 2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_mov_membase_reg(inst, reg, imm_value, reg2, 2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2, imm_value3;
	if(insn->dest->is_constant && insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->dest->address;
		imm_value2 = insn->value1->address;
		imm_value3 = insn->value2->address;
		{
			x86_mov_mem_imm(inst, imm_value + imm_value3, imm_value2, 4);
		}
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->dest->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->dest->address;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value2 = insn->value2->address;
		{
			x86_mov_mem_reg(inst, imm_value + imm_value2, reg, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			x86_mov_membase_imm(inst, reg, imm_value2, imm_value, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_mov_membase_reg(inst, reg, imm_value, reg2, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	jit_nint imm_value, imm_value2;
	jit_nint local_offset;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			x86_mov_membase_imm(inst, reg, imm_value2, *(int *)(imm_value), 4);
			x86_mov_membase_imm(inst, reg, imm_value2 + 4, *(int *)(imm_value + 4), 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0 && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_mov_reg_membase(inst, reg2, X86_EBP, local_offset, 4);
			x86_mov_membase_reg(inst, reg, imm_value, reg2, 4);
			x86_mov_reg_membase(inst, reg2, X86_EBP, local_offset + 4, 4);
			x86_mov_membase_reg(inst, reg, imm_value + 4, reg2, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_mov_membase_reg(inst, reg, imm_value, reg2, 4);
			x86_mov_membase_reg(inst, reg, imm_value + 4, other_reg2, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			x86_mov_membase_imm(inst, reg, imm_value2, ((int *)(imm_value))[0], 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_fst_membase(inst, reg, imm_value, 0, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			x86_mov_membase_imm(inst, reg, imm_value2, ((int *)(imm_value))[0], 4);
			x86_mov_membase_imm(inst, reg, imm_value2 + 4, ((int *)(imm_value))[1], 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_fst_membase(inst, reg, imm_value, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			x86_mov_membase_imm(inst, reg, imm_value2, ((int *)(imm_value))[0], 4);
			x86_mov_membase_imm(inst, reg, imm_value2 + 4, ((int *)(imm_value))[1], 4);
			if(sizeof(jit_nfloat) != sizeof(jit_float64))
			{
				x86_mov_membase_imm(inst, reg, imm_value2 + 8, ((int *)(imm_value))[2], 4);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			if(sizeof(jit_nfloat) != sizeof(jit_float64))
			{
				x86_fst80_membase(inst, reg, imm_value);
			}
			else
			{
				x86_fst_membase(inst, reg, imm_value, 1, 1);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_STRUCT:
{
		{
			unsigned char *inst;
			int reg = _jit_regs_load_value
				(gen, insn->dest, 0,
				 (insn->flags & (JIT_INSN_DEST_NEXT_USE |
				 				 JIT_INSN_DEST_LIVE)));
			_jit_regs_spill_all(gen);
			_jit_gen_fix_value(insn->value1);
			inst = gen->ptr;
			_jit_gen_check_space(gen, 128);
			reg = _jit_reg_info[reg].cpu_reg;
			inst = memory_copy(gen, inst, reg, (int)(insn->value2->address),
							   X86_EBP, insn->value1->frame_offset,
							   jit_type_get_size(jit_value_get_type(insn->value1)));
			gen->ptr = inst;
		}
}
break;

case JIT_OP_ADD_RELATIVE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			if(insn->value2->address != 0)
			{
				x86_alu_reg_imm(inst, X86_ADD, reg, imm_value);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_SBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_widen_memindex(inst, reg, reg2, 0, reg3, 0, 1, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_UBYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_widen_memindex(inst, reg, reg2, 0, reg3, 0, 0, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_SHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_widen_memindex(inst, reg, reg2, 0, reg3, 1, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_USHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_widen_memindex(inst, reg, reg2, 0, reg3, 1, 0, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_mov_reg_memindex(inst, reg, reg2, 0, reg3, 2, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_lreg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_dest_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			if(reg == reg2 || reg == reg3)
			{
				x86_mov_reg_memindex(inst, other_reg, reg2, 4, reg3, 3, 4);
				x86_mov_reg_memindex(inst, reg, reg2, 0, reg3, 3, 4);
			}
			else
			{
				x86_mov_reg_memindex(inst, reg, reg2, 0, reg3, 3, 4);
				x86_mov_reg_memindex(inst, other_reg, reg2, 4, reg3, 3, 4);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_fld_memindex(inst, reg2, 0, reg3, 2, 0);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_fld_memindex(inst, reg2, 0, reg3, 3, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	if((sizeof(jit_nfloat) != sizeof(jit_float64)))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
	    		/* lea reg3, [reg3 + reg3 * 2]  */
			x86_lea_memindex(inst, reg3, reg3, 0, reg3, 1);
			/* fld [reg2 * 4] */
			x86_fld80_memindex(inst, reg2, 0, reg3, 2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, x86_freg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_fld_memindex(inst, reg2, 0, reg3, 3, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_BYTE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_breg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_mov_memindex_reg(inst, reg, 0, reg2, 0, reg3, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_SHORT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_mov_memindex_reg(inst, reg, 0, reg2, 1, reg3, 2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_INT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_mov_memindex_reg(inst, reg, 0, reg2, 2, reg3, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_LONG:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	int other_reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_mov_memindex_imm(inst, reg, 0, reg2, 3, *(int *)(imm_value), 4);
			x86_mov_memindex_imm(inst, reg, 4, reg2, 3, *(int *)(imm_value + 4), 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			x86_mov_reg_membase(inst, reg3, X86_EBP, local_offset, 4);
			x86_mov_memindex_reg(inst, reg, 0, reg2, 3, reg3, 4);
			x86_mov_reg_membase(inst, reg3, X86_EBP, local_offset + 4, 4);
			x86_mov_memindex_reg(inst, reg, 4, reg2, 3, reg3, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_lreg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg3 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			x86_mov_memindex_reg(inst, reg, 0, reg2, 3, reg3, 4);
			x86_mov_memindex_reg(inst, reg, 4, reg2, 3, other_reg3, 4);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_FLOAT32:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_fst_memindex(inst, reg, 0, reg2, 2, 0, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_FLOAT64:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_fst_memindex(inst, reg, 0, reg2, 3, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_NFLOAT:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	if((sizeof(jit_nfloat) != sizeof(jit_float64)))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
	    		/* lea reg2, [reg2 + reg2 * 2]  */
			x86_lea_memindex(inst, reg2, reg2, 0, reg2, 1);
			/* fst [reg2 * 4] */
			x86_fst80_memindex(inst, reg, 0, reg2, 2);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_STACK);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_freg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_fst_memindex(inst, reg, 0, reg2, 3, 1, 1);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_MEMCPY:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	if(insn->value2->is_constant && (insn->value2->address <= 0))
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value2->address;
		{ }
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->value2->is_constant && (insn->value2->address <= 32))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_add_scratch(&regs, x86_breg);
		_jit_regs_begin(gen, &regs, (32 + insn->value2->address * 4));
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			int disp;
			disp = 0;
			while(imm_value >= (disp + 4))
			{
				x86_mov_reg_membase(inst, reg3, reg2, disp, 4);
				x86_mov_membase_reg(inst, reg, disp, reg3, 4);
				disp += 4;
			}
			if(imm_value >= (disp + 2))
			{
				x86_mov_reg_membase(inst, reg3, reg2, disp, 2);
				x86_mov_membase_reg(inst, reg, disp, reg3, 2);
				disp += 2;
			}
			if(imm_value > disp)
			{
				x86_mov_reg_membase(inst, reg3, reg2, disp, 1);
				x86_mov_membase_reg(inst, reg, disp, reg3, 1);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("eax"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("ecx"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("edx"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("ebx"));
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_push_reg(inst, reg3);
			x86_push_reg(inst, reg2);
			x86_push_reg(inst, reg);
			x86_call_code(inst, jit_memcpy);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, 3 * sizeof(void *));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_MEMMOVE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	if(insn->value2->is_constant && (insn->value2->address <= 0))
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value2->address;
		{ }
		gen->ptr = (unsigned char *)inst;
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("eax"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("ecx"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("edx"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("ebx"));
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_push_reg(inst, reg3);
			x86_push_reg(inst, reg2);
			x86_push_reg(inst, reg);
			x86_call_code(inst, jit_memmove);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, 3 * sizeof(void *));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_MEMSET:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value, imm_value2;
	if(insn->value2->is_constant && (insn->value2->address <= 0))
	{
		_jit_gen_check_space(gen, 32);
		inst = (unsigned char *)(gen->ptr);
		imm_value = insn->value2->address;
		{ }
		gen->ptr = (unsigned char *)inst;
	}
	else if(insn->value1->is_constant && insn->value2->is_constant && (insn->value2->address <= 32))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, (32 + insn->value2->address * 4));
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			int disp;
			disp = 0;
			while(imm_value2 >= (disp + 4))
			{
				x86_mov_membase_imm(inst, reg, disp, imm_value * 0x01010101, 4);
				disp += 4;
			}
			if(imm_value2 >= (disp + 2))
			{
				x86_mov_membase_imm(inst, reg, disp, imm_value * 0x0101, 2);
				disp += 2;
			}
			if(insn->value2->address > disp)
			{
				x86_mov_membase_imm(inst, reg, disp, imm_value, 1);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address < 4))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_breg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			x86_mov_membase_reg(inst, reg, 0, reg2, 1);
			if(imm_value > 1)
			{
				x86_mov_membase_reg(inst, reg, 1, reg2, 1);
				if(imm_value > 2)
				{
					x86_mov_membase_reg(inst, reg, 2, reg2, 1);
				}
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address <= 32 && (insn->value2->address % 2) == 0))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_reg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, (32 + insn->value2->address * 4));
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			int disp;
			x86_mov_reg_reg(inst, reg3, reg2, 4);
			x86_shift_reg_imm(inst, X86_SHL, reg2, 8);
			x86_alu_reg_reg(inst, X86_OR, reg2, reg3);
			x86_mov_reg_reg(inst, reg3, reg2, 4);
			x86_shift_reg_imm(inst, X86_SHL, reg2, 16);
			x86_alu_reg_reg(inst, X86_OR, reg2, reg3);
			disp = 0;
			while(imm_value >= (disp + 4))
			{
				x86_mov_membase_reg(inst, reg, disp, reg2, 4);
				disp += 4;
			}
			if(imm_value > disp)
			{
				x86_mov_membase_reg(inst, reg, disp, reg2, 2);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address <= 32 && (insn->value2->address % 2) != 0))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, x86_breg);
		_jit_regs_add_scratch(&regs, x86_reg);
		_jit_regs_begin(gen, &regs, (32 + insn->value2->address * 4));
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			int disp;
			x86_mov_reg_reg(inst, reg3, reg2, 4);
			x86_shift_reg_imm(inst, X86_SHL, reg2, 8);
			x86_alu_reg_reg(inst, X86_OR, reg2, reg3);
			x86_mov_reg_reg(inst, reg3, reg2, 4);
			x86_shift_reg_imm(inst, X86_SHL, reg2, 16);
			x86_alu_reg_reg(inst, X86_OR, reg2, reg3);
			disp = 0;
			while(imm_value >= (disp + 4))
			{
				x86_mov_membase_reg(inst, reg, disp, reg2, 4);
				disp += 4;
			}
			if(imm_value >= (disp + 2))
			{
				x86_mov_membase_reg(inst, reg, disp, reg2, 2);
				disp += 2;
			}
			if(imm_value > disp)
			{
				x86_mov_membase_reg(inst, reg, disp, reg2, 1);
			}
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_init_value2(&regs, insn, 0, x86_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("eax"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("ecx"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("edx"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("ebx"));
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			x86_push_reg(inst, reg3);
			x86_push_reg(inst, reg2);
			x86_push_reg(inst, reg);
			x86_call_code(inst, jit_memset);
			x86_alu_reg_imm(inst, X86_ADD, X86_ESP, 3 * sizeof(void *));
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ALLOCA:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_init_value1(&regs, insn, 0, x86_reg);
		_jit_regs_begin(gen, &regs, 32);
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			x86_alu_reg_reg(inst, X86_SUB, X86_ESP, reg);
			x86_mov_reg_reg(inst, reg, X86_ESP, 4);
			gen->stack_changed = 1;
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_JUMP_TABLE:
{
	unsigned char * inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value, imm_value2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY | _JIT_REGS_BRANCH);
		_jit_regs_init_dest(&regs, insn, 0, x86_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_begin(gen, &regs, (32 + sizeof(void *) * insn->value2->address));
		inst = (unsigned char *)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			unsigned char *patch_jump_table;
			unsigned char *patch_fall_through;
			int index;
			jit_label_t *labels;
			jit_nint num_labels;
			jit_block_t block;
	
			labels = (jit_label_t *) imm_value;
			num_labels = imm_value2;
	
			x86_alu_reg_imm(inst, X86_CMP, reg, num_labels);
			patch_fall_through = inst;
			x86_branch32(inst, X86_CC_AE, 0, 0);
	
			if(func->builder->position_independent)
			{
				/* TODO */
				TODO();
			}
			else
			{
				patch_jump_table = inst;
				x86_jump_memindex(inst, X86_NOBASEREG, 0, reg, 2);
				while(((jit_nint) inst & (sizeof(void*) - 1)) != 0)
				{
					x86_nop(inst);
				}
	
				// displacement goes after opcode. ModR/M, and SIB bytes
				*((void **)(patch_jump_table + 3)) = inst;
			}
	
			for(index = 0; index < num_labels; index++)
			{
				block = jit_block_from_label(func, labels[index]);
				if(!block)
				{
					return;
				}
	
				if(func->builder->position_independent)
				{
					/* TODO */
					TODO();
				}
				else
				{
					if(block->address)
					{
						x86_imm_emit32(inst, block->address);
					}
					else
					{
						/* Output a placeholder and record on the block's fixup list */
						x86_imm_emit32(inst, (int)(block->fixup_absolute_list));
						block->fixup_absolute_list = (void *)(inst - 4);
					}
				}
			}
	
			x86_patch(patch_fall_through, inst);
		}
		gen->ptr = (unsigned char *)inst;
		_jit_regs_commit(gen, &regs);
	}
}
break;

#elif defined(JIT_INCLUDE_SUPPORTED)

case JIT_OP_TRUNC_SBYTE:
case JIT_OP_TRUNC_UBYTE:
case JIT_OP_TRUNC_SHORT:
case JIT_OP_TRUNC_USHORT:
case JIT_OP_CHECK_SBYTE:
case JIT_OP_CHECK_UBYTE:
case JIT_OP_CHECK_SHORT:
case JIT_OP_CHECK_USHORT:
case JIT_OP_CHECK_INT:
case JIT_OP_CHECK_UINT:
case JIT_OP_LOW_WORD:
case JIT_OP_EXPAND_INT:
case JIT_OP_EXPAND_UINT:
case JIT_OP_FLOAT32_TO_INT:
case JIT_OP_FLOAT64_TO_INT:
case JIT_OP_NFLOAT_TO_INT:
case JIT_OP_FLOAT32_TO_LONG:
case JIT_OP_FLOAT64_TO_LONG:
case JIT_OP_NFLOAT_TO_LONG:
case JIT_OP_INT_TO_FLOAT32:
case JIT_OP_INT_TO_FLOAT64:
case JIT_OP_INT_TO_NFLOAT:
case JIT_OP_UINT_TO_FLOAT32:
case JIT_OP_UINT_TO_FLOAT64:
case JIT_OP_UINT_TO_NFLOAT:
case JIT_OP_LONG_TO_FLOAT32:
case JIT_OP_LONG_TO_FLOAT64:
case JIT_OP_LONG_TO_NFLOAT:
case JIT_OP_ULONG_TO_FLOAT32:
case JIT_OP_ULONG_TO_FLOAT64:
case JIT_OP_ULONG_TO_NFLOAT:
case JIT_OP_FLOAT64_TO_FLOAT32:
case JIT_OP_NFLOAT_TO_FLOAT32:
case JIT_OP_NFLOAT_TO_FLOAT64:
case JIT_OP_FLOAT32_TO_NFLOAT:
case JIT_OP_FLOAT32_TO_FLOAT64:
case JIT_OP_FLOAT64_TO_NFLOAT:
case JIT_OP_IADD:
case JIT_OP_ISUB:
case JIT_OP_IMUL:
case JIT_OP_IDIV:
case JIT_OP_IDIV_UN:
case JIT_OP_IREM:
case JIT_OP_IREM_UN:
case JIT_OP_INEG:
case JIT_OP_LADD:
case JIT_OP_LSUB:
case JIT_OP_LNEG:
case JIT_OP_FADD:
case JIT_OP_DADD:
case JIT_OP_NFADD:
case JIT_OP_FSUB:
case JIT_OP_DSUB:
case JIT_OP_NFSUB:
case JIT_OP_FMUL:
case JIT_OP_DMUL:
case JIT_OP_NFMUL:
case JIT_OP_FDIV:
case JIT_OP_DDIV:
case JIT_OP_NFDIV:
case JIT_OP_FREM:
case JIT_OP_DREM:
case JIT_OP_NFREM:
case JIT_OP_FNEG:
case JIT_OP_DNEG:
case JIT_OP_NFNEG:
case JIT_OP_IAND:
case JIT_OP_IOR:
case JIT_OP_IXOR:
case JIT_OP_INOT:
case JIT_OP_ISHL:
case JIT_OP_ISHR:
case JIT_OP_ISHR_UN:
case JIT_OP_LAND:
case JIT_OP_LOR:
case JIT_OP_LXOR:
case JIT_OP_LNOT:
case JIT_OP_BR:
case JIT_OP_BR_IFALSE:
case JIT_OP_BR_ITRUE:
case JIT_OP_BR_IEQ:
case JIT_OP_BR_INE:
case JIT_OP_BR_ILT:
case JIT_OP_BR_ILT_UN:
case JIT_OP_BR_ILE:
case JIT_OP_BR_ILE_UN:
case JIT_OP_BR_IGT:
case JIT_OP_BR_IGT_UN:
case JIT_OP_BR_IGE:
case JIT_OP_BR_IGE_UN:
case JIT_OP_BR_LFALSE:
case JIT_OP_BR_LTRUE:
case JIT_OP_IEQ:
case JIT_OP_INE:
case JIT_OP_ILT:
case JIT_OP_ILT_UN:
case JIT_OP_ILE:
case JIT_OP_ILE_UN:
case JIT_OP_IGT:
case JIT_OP_IGT_UN:
case JIT_OP_IGE:
case JIT_OP_IGE_UN:
case JIT_OP_FATAN:
case JIT_OP_DATAN:
case JIT_OP_NFATAN:
case JIT_OP_FCOS:
case JIT_OP_DCOS:
case JIT_OP_NFCOS:
case JIT_OP_FSIN:
case JIT_OP_DSIN:
case JIT_OP_NFSIN:
case JIT_OP_FSQRT:
case JIT_OP_DSQRT:
case JIT_OP_NFSQRT:
case JIT_OP_IABS:
case JIT_OP_LABS:
case JIT_OP_FABS:
case JIT_OP_DABS:
case JIT_OP_NFABS:
case JIT_OP_IMIN_UN:
case JIT_OP_ISIGN:
case JIT_OP_LSIGN:
case JIT_OP_CHECK_NULL:
case JIT_OP_CALL:
case JIT_OP_CALL_TAIL:
case JIT_OP_CALL_INDIRECT:
case JIT_OP_CALL_INDIRECT_TAIL:
case JIT_OP_CALL_VTABLE_PTR:
case JIT_OP_CALL_VTABLE_PTR_TAIL:
case JIT_OP_CALL_EXTERNAL:
case JIT_OP_CALL_EXTERNAL_TAIL:
case JIT_OP_RETURN:
case JIT_OP_RETURN_INT:
case JIT_OP_RETURN_LONG:
case JIT_OP_RETURN_FLOAT32:
case JIT_OP_RETURN_FLOAT64:
case JIT_OP_RETURN_NFLOAT:
case JIT_OP_RETURN_SMALL_STRUCT:
case JIT_OP_SETUP_FOR_NESTED:
case JIT_OP_SETUP_FOR_SIBLING:
case JIT_OP_IMPORT:
case JIT_OP_THROW:
case JIT_OP_RETHROW:
case JIT_OP_LOAD_PC:
case JIT_OP_LOAD_EXCEPTION_PC:
case JIT_OP_ENTER_FINALLY:
case JIT_OP_LEAVE_FINALLY:
case JIT_OP_CALL_FINALLY:
case JIT_OP_ENTER_FILTER:
case JIT_OP_LEAVE_FILTER:
case JIT_OP_CALL_FILTER:
case JIT_OP_CALL_FILTER_RETURN:
case JIT_OP_ADDRESS_OF_LABEL:
case JIT_OP_COPY_LOAD_SBYTE:
case JIT_OP_COPY_LOAD_UBYTE:
case JIT_OP_COPY_STORE_BYTE:
case JIT_OP_COPY_LOAD_SHORT:
case JIT_OP_COPY_LOAD_USHORT:
case JIT_OP_COPY_STORE_SHORT:
case JIT_OP_COPY_INT:
case JIT_OP_COPY_LONG:
case JIT_OP_COPY_FLOAT32:
case JIT_OP_COPY_FLOAT64:
case JIT_OP_COPY_NFLOAT:
case JIT_OP_COPY_STRUCT:
case JIT_OP_ADDRESS_OF:
case JIT_OP_INCOMING_REG:
case JIT_OP_RETURN_REG:
case JIT_OP_PUSH_INT:
case JIT_OP_PUSH_LONG:
case JIT_OP_PUSH_FLOAT32:
case JIT_OP_PUSH_FLOAT64:
case JIT_OP_PUSH_NFLOAT:
case JIT_OP_PUSH_STRUCT:
case JIT_OP_POP_STACK:
case JIT_OP_FLUSH_SMALL_STRUCT:
case JIT_OP_LOAD_RELATIVE_SBYTE:
case JIT_OP_LOAD_RELATIVE_UBYTE:
case JIT_OP_LOAD_RELATIVE_SHORT:
case JIT_OP_LOAD_RELATIVE_USHORT:
case JIT_OP_LOAD_RELATIVE_INT:
case JIT_OP_LOAD_RELATIVE_LONG:
case JIT_OP_LOAD_RELATIVE_FLOAT32:
case JIT_OP_LOAD_RELATIVE_FLOAT64:
case JIT_OP_LOAD_RELATIVE_NFLOAT:
case JIT_OP_LOAD_RELATIVE_STRUCT:
case JIT_OP_STORE_RELATIVE_BYTE:
case JIT_OP_STORE_RELATIVE_SHORT:
case JIT_OP_STORE_RELATIVE_INT:
case JIT_OP_STORE_RELATIVE_LONG:
case JIT_OP_STORE_RELATIVE_FLOAT32:
case JIT_OP_STORE_RELATIVE_FLOAT64:
case JIT_OP_STORE_RELATIVE_NFLOAT:
case JIT_OP_STORE_RELATIVE_STRUCT:
case JIT_OP_ADD_RELATIVE:
case JIT_OP_LOAD_ELEMENT_SBYTE:
case JIT_OP_LOAD_ELEMENT_UBYTE:
case JIT_OP_LOAD_ELEMENT_SHORT:
case JIT_OP_LOAD_ELEMENT_USHORT:
case JIT_OP_LOAD_ELEMENT_INT:
case JIT_OP_LOAD_ELEMENT_LONG:
case JIT_OP_LOAD_ELEMENT_FLOAT32:
case JIT_OP_LOAD_ELEMENT_FLOAT64:
case JIT_OP_LOAD_ELEMENT_NFLOAT:
case JIT_OP_STORE_ELEMENT_BYTE:
case JIT_OP_STORE_ELEMENT_SHORT:
case JIT_OP_STORE_ELEMENT_INT:
case JIT_OP_STORE_ELEMENT_LONG:
case JIT_OP_STORE_ELEMENT_FLOAT32:
case JIT_OP_STORE_ELEMENT_FLOAT64:
case JIT_OP_STORE_ELEMENT_NFLOAT:
case JIT_OP_MEMCPY:
case JIT_OP_MEMMOVE:
case JIT_OP_MEMSET:
case JIT_OP_ALLOCA:
case JIT_OP_JUMP_TABLE:
	return 1;

#endif
