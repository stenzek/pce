/* Automatically generated from ../../jit/jit-rules-arm.ins - DO NOT EDIT */
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
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 24);
			arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 24);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_TRUNC_UBYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_alu_reg_imm8(inst, ARM_AND, reg, reg, 0xFF);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_TRUNC_SHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 16);
			arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 16);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_TRUNC_USHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 16);
			arm_shift_reg_imm8(inst, ARM_SHR, reg, reg, 16);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INT_TO_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint local_offset;
	if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_add_scratch(&regs, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			//Load int from a local variable stored in memory
			arm_load_membase_float(inst, reg2, ARM_FP, local_offset, 0);
			arm_convert_float_signed_integer_double(inst, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			//The int value is in a register
			arm_mov_float_reg(inst, reg3, reg2);
			arm_convert_float_signed_integer_double(inst, reg, reg3)
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_NFLOAT_TO_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_convert_float_single_double(inst, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_NFLOAT_TO_FLOAT64:
case JIT_OP_FLOAT64_TO_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* Nothing to do: float64 and nfloat are the same thing on ARM linux. Just copy the value */
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_FLOAT32_TO_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_convert_float_double_single(inst, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IADD:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_alu_reg_imm8(inst, ARM_ADD, reg, reg, imm_value);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_ADD, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISUB:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_alu_reg_imm8(inst, ARM_SUB, reg, reg, imm_value);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_SUB, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IMUL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Handle special cases of immediate multiplies */
			switch(imm_value)
			{
				case 0:
				{
					arm_mov_reg_imm8(inst, reg, 0);
				}
				break;
	
				case 1: break;
	
				case 2:
				{
					arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 1);
				}
				break;
	
				case 4:
				{
					arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 2);
				}
				break;
	
				case 8:
				{
					arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 3);
				}
				break;
	
				case 16:
				{
					arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 4);
				}
				break;
	
				case 32:
				{
					arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 5);
				}
				break;
	
				case 64:
				{
					arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 6);
				}
				break;
	
				case 128:
				{
					arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, 7);
				}
				break;
	
				default:
				{
					arm_mov_reg_imm8(inst, ARM_WORK, imm_value);
					arm_mul_reg_reg(inst, reg, reg, ARM_WORK);
				}
				break;
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			if(reg != reg2)
			{
				arm_mul_reg_reg(inst, reg, reg, reg2);
			}
			else
			{
				/* Cannot use the same register for both arguments */
				arm_mov_reg_reg(inst, ARM_WORK, reg2);
				arm_mul_reg_reg(inst, reg, reg, ARM_WORK);
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IDIV:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address == 0)
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			throw_builtin(&inst, func, ARM_CC_AL, JIT_RESULT_DIVISION_BY_ZERO);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
	else if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255 && (insn->value2->address == 1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Division by 1. Return the value itself */
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255 && ((insn->value2->address > 0) && (((jit_nuint)insn->value2->address) & (((jit_nuint)insn->value2->address) - 1)) == 0))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Handle special cases of small immediate divides: divisions by positive powers of two */
			/* NB: (n & (n-1)) == 0 if and only if n is a power of 2 */
			
			/* Move the dividend in the work register, setting the codes (in order to know if it's positive or negative) */
			arm_alu_cc_reg(inst, ARM_MOV, ARM_WORK, reg);
	
			/* If the dividend is negative, make it positive (0-x = -x)*/
			arm_alu_reg_imm8_cond(inst, ARM_RSB, reg, ARM_WORK, 0, ARM_CC_MI);
	
			switch(imm_value)
			{
				//Integer divide by shifting
				case 2:
				{
					arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 1);
				}
				break;
	
				case 4:
				{
					arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 2);
				}
				break;
	
				case 8:
				{
					arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 3);
				}
				break;
	
				case 16:
				{
					arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 4);
				}
				break;
	
				case 32:
				{
					arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 5);
				}
				break;
	
				case 64:
				{
					arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 6);
				}
				break;
	
				case 128:
				{
					arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, 7);
				}
				break;
			}
			
			/* If the dividend was negative, make it negative again (0-x = -x)*/
			arm_alu_reg_imm8_cond(inst, ARM_RSB, reg, reg, 0, ARM_CC_MI);
			
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address == -1))
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Dividing by -1 simply negates */
			/*TODO: if the value to be divided by -1 is jit_min_int, 
			  an exception (JIT_RESULT_ARITHMETIC) should probably be thrown */
			arm_alu_reg(inst, ARM_MVN, reg, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r1"));
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			/* Every other immediate division:
			   ARM does not have an integer division operation. It's emulated via software. */
			
			//Put the dividend in the right position
			if (reg != ARM_R0)
				arm_mov_reg_reg(inst, ARM_R0, reg);
			
			//Put the divisor in the right position
			mov_reg_imm(gen, &inst, ARM_R1, imm_value);
			
			//Perform the division by calling a function from the runtime ABI
			extern int __aeabi_idiv(int numerator, int denominator);
			arm_call(inst, __aeabi_idiv);
			
			if(reg != ARM_R0)
			{
				//Move the result back where it is expected to be
				arm_mov_reg_reg(inst, reg, ARM_R0);
			}
			
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r1"));
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			/* Every division taking data from two registers:
			   ARM does not have an integer division operation. It's emulated via software. */
			
			int dividend = reg;
			int divisor = reg2;
			int scratch = reg3;
			
			//Put the dividend in the right position
			if (dividend != ARM_R0)
			{
				if (divisor==ARM_R0)
				{
					//Prevent the divisor from being overwritten
					if(dividend != ARM_R1)
					{
						//The place where the divisor should be is free. Move it there
						arm_mov_reg_reg(inst, ARM_R1, divisor);
						divisor=1;
					}
					else
					{
						/* The dividend is where the divisor should be.
						   We must use a scratch register to swap them */
						arm_mov_reg_reg(inst, scratch, divisor);
						divisor=scratch;
					}
					
				}
				
				arm_mov_reg_reg(inst, ARM_R0, dividend);
			}
			
			if (divisor != ARM_R1)
			{
				//Put the divisor in the right position
				arm_mov_reg_reg(inst, ARM_R1, divisor);
			}
			
			//Perform the division by calling a function from the runtime ABI
			extern int __aeabi_idiv(int numerator, int denominator);
			arm_call(inst, __aeabi_idiv);
			
			//Move the result back where it is expected to be
			if(reg != ARM_R0)
			{
				arm_mov_reg_reg(inst, reg, ARM_R0);
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INEG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* -x is the same as (0 - x) */
			arm_alu_reg_imm8(inst, ARM_RSB, reg, reg, 0);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LADD:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value2(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			arm_alu_cc_reg_reg(inst, ARM_ADD, reg, reg, reg2);
			arm_alu_reg_reg(inst, ARM_ADC, other_reg, other_reg, other_reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LSUB:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value2(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			arm_alu_cc_reg_reg(inst, ARM_SUB, reg, reg, reg2);
			arm_alu_reg_reg(inst, ARM_SBC, other_reg, other_reg, other_reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LNEG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			arm_alu_reg(inst, ARM_MVN, reg, reg);
			arm_alu_reg(inst, ARM_MVN, other_reg, other_reg);
			arm_alu_cc_reg_imm8(inst, ARM_ADD, reg, reg, 1);
			arm_alu_reg_imm8(inst, ARM_ADC, other_reg, other_reg, 0);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_FADD:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_FADD, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_FADD:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_ADF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_FSUB:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_FSUB, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_FSUB:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_SUF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_FMUL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_FMUL, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_FMUL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_MUF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_FDIV:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_FDIV, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_FDIV:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg_32(inst, ARM_DVF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_FNEG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_alu_freg_32(inst, ARM_MNF, reg, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_FNEG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_alu_freg_32(inst, ARM_MNF, reg, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_DADD:
case JIT_OP_NFADD:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_FADD, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_DADD:
case JIT_OP_NFADD:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_ADF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_DSUB:
case JIT_OP_NFSUB:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_FSUB, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_DSUB:
case JIT_OP_NFSUB:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_SUF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_DMUL:
case JIT_OP_NFMUL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_FMUL, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_DMUL:
case JIT_OP_NFMUL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_MUF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_DDIV:
case JIT_OP_NFDIV:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_FDIV, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_DDIV:
case JIT_OP_NFDIV:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_freg_freg(inst, ARM_DVF, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_DNEG:
case JIT_OP_NFNEG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_alu_freg(inst, ARM_MNF, reg, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_DNEG:
case JIT_OP_NFNEG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_alu_freg(inst, ARM_MNF, reg, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

case JIT_OP_IAND:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_alu_reg_imm8(inst, ARM_AND, reg, reg, imm_value);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_AND, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IOR:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_alu_reg_imm8(inst, ARM_ORR, reg, reg, imm_value);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_ORR, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IXOR:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_alu_reg_imm8(inst, ARM_EOR, reg, reg, imm_value);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_EOR, reg, reg, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INOT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* MVN == "move not" */
			arm_alu_reg(inst, ARM_MVN, reg, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISHL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_shift_reg_imm8(inst, ARM_SHL, reg, reg, (imm_value & 0x1F));
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_imm8(inst, ARM_AND, ARM_WORK, reg2, 0x1F);
			arm_shift_reg_reg(inst, ARM_SHL, reg, reg, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISHR:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_shift_reg_imm8(inst, ARM_SAR, reg, reg, (imm_value & 0x1F));
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_imm8(inst, ARM_AND, ARM_WORK, reg2, 0x1F);
			arm_shift_reg_reg(inst, ARM_SAR, reg, reg, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ISHR_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_shift_reg_imm8(inst, ARM_SHR, reg, reg, (imm_value & 0x1F));
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_alu_reg_imm8(inst, ARM_AND, ARM_WORK, reg2, 0x1F);
			arm_shift_reg_reg(inst, ARM_SHR, reg, reg, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LAND:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value2(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_AND, reg, reg, reg2);
			arm_alu_reg_reg(inst, ARM_AND, other_reg, other_reg, other_reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOR:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value2(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_ORR, reg, reg, reg2);
			arm_alu_reg_reg(inst, ARM_ORR, other_reg, other_reg, other_reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LXOR:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg, other_reg2;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value2(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		{
			arm_alu_reg_reg(inst, ARM_EOR, reg, reg, reg2);
			arm_alu_reg_reg(inst, ARM_EOR, other_reg, other_reg, other_reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LNOT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			arm_alu_reg(inst, ARM_MVN, reg, reg);
			arm_alu_reg(inst, ARM_MVN, other_reg, other_reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR:
{
	arm_inst_buf inst;
	{
		_jit_regs_spill_all(gen);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			/* ARM_CC_AL == "always branch" */
			output_branch(func, &inst, ARM_CC_AL, insn);
	
			/* Flush the constant pool now, to minimize the probability that
			   it is accidentally flushed in the middle of a loop body */
			jit_gen_save_inst_ptr(gen, inst);
			flush_constants(gen, 1);
			jit_gen_load_inst_ptr(gen, inst);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_BR_IFALSE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, 0);
			output_branch(func, &inst, ARM_CC_EQ, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ITRUE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, 0);
			output_branch(func, &inst, ARM_CC_NE, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IEQ:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_EQ, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_EQ, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_INE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_NE, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_NE, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_LT, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_LT, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILT_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_LT_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_LT_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_LE, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_LE, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_ILE_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_LE_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_LE_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_GT, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_GT, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGT_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_GT_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_GT_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_GE, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_GE, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_BR_IGE_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			output_branch(func, &inst, ARM_CC_GE_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			output_branch(func, &inst, ARM_CC_GE_UN, insn);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ICMP:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE);
			arm_alu_reg_cond(inst, ARM_MVN, reg, reg, ARM_CC_LT);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE);
			arm_alu_reg_cond(inst, ARM_MVN, reg, reg, ARM_CC_LT);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ICMP_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE_UN);
			arm_alu_reg_cond(inst, ARM_MVN, reg, reg, ARM_CC_LT_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE_UN);
			arm_alu_reg_cond(inst, ARM_MVN, reg, reg, ARM_CC_LT_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IEQ:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_EQ);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_NE);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_EQ);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_NE);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_NE);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_EQ);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_NE);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_EQ);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LT);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GE);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LT);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GE);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILT_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LT_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GE_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LT_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GE_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LE);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GT);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LE);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GT);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ILE_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LE_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GT_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_LE_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_GT_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGT_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GT_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LE_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GE);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LT);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GE);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LT);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_IGE_UN:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	if(insn->value2->is_nint_constant && insn->value2->address >= 0 && insn->value2->address <= 255)
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, imm_value);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GE_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LT_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
			arm_test_reg_reg(inst, ARM_CMP, reg, reg2);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 1, ARM_CC_GE_UN);
			arm_alu_reg_imm8_cond(inst, ARM_MOV, reg, 0, 0, ARM_CC_LT_UN);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CHECK_NULL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_test_reg_imm8(inst, ARM_CMP, reg, 0);
			throw_builtin(&inst, func, ARM_CC_EQ, JIT_RESULT_NULL_REFERENCE);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_CALL:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			jit_function_t func = (jit_function_t)(insn->dest);
			arm_call(inst, jit_function_to_closure(func));
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_CALL_TAIL:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			jit_function_t func = (jit_function_t)(insn->dest);
			arm_pop_frame_tail(inst, 0);
			arm_jump(inst, jit_function_to_closure(func));
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_CALL_INDIRECT:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			arm_mov_reg_reg((inst), ARM_LINK, ARM_PC);
			arm_mov_reg_reg((inst), ARM_PC, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_CALL_VTABLE_PTR:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			arm_mov_reg_reg((inst), ARM_LINK, ARM_PC);
			arm_mov_reg_reg((inst), ARM_PC, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_CALL_EXTERNAL:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			arm_call(inst, (void *)(insn->dest));
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_RETURN:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_RETURN_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			int cpu_reg = reg;
			if(cpu_reg != ARM_R0)
			{
				arm_mov_reg_reg(inst, ARM_R0, cpu_reg);
			}
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_RETURN_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value1->is_constant)
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->value1->address;
		{
			mov_reg_imm(gen, &inst, ARM_R0, ((jit_int *)(imm_value))[0]);
			mov_reg_imm(gen, &inst, ARM_R1, ((jit_int *)(imm_value))[1]);
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0)
	{
		_jit_gen_fix_value(insn->value1);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		local_offset = insn->value1->frame_offset;
		{
			arm_load_membase(inst, ARM_R0, ARM_FP, local_offset);
			arm_load_membase(inst, ARM_R1, ARM_FP, local_offset + 4);
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			if(reg != 0)
			{
				arm_mov_reg_reg(inst, ARM_R0, reg);
				arm_mov_reg_reg(inst, ARM_R1, other_reg);
			}
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_RETURN_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_mov_reg_float(inst, ARM_R0, reg);
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_RETURN_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			if(reg != 0)
			{
				arm_alu_freg_32(inst, ARM_MVF, ARM_F0, reg);
			}
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_RETURN_FLOAT32:
{
		{
			arm_inst_buf inst;
			_jit_regs_spill_all(gen);
			_jit_gen_fix_value(insn->value1);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_R0, ((int *)(insn->value1->address))[0]);
			}
			else
			{
				arm_load_membase(inst, ARM_R0, ARM_FP, insn->value1->frame_offset);
			}
			jump_to_epilog(gen, &inst, block);
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_RETURN_FLOAT64:
case JIT_OP_RETURN_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r1"));
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_mov_reg_reg_double(inst,ARM_R0,ARM_R1, reg);
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_FPA

case JIT_OP_RETURN_FLOAT64:
case JIT_OP_RETURN_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			if(reg != 0)
			{
				arm_alu_freg(inst, ARM_MVF, ARM_F0, reg);
			}
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FPA */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_RETURN_FLOAT64:
case JIT_OP_RETURN_NFLOAT:
{
		{
			arm_inst_buf inst;
			_jit_regs_spill_all(gen);
			_jit_gen_fix_value(insn->value1);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_R0, ((int *)(insn->value1->address))[0]);
				mov_reg_imm
					(gen, &inst, ARM_R1, ((int *)(insn->value1->address))[1]);
			}
			else
			{
				arm_load_membase(inst, ARM_R0, ARM_FP, insn->value1->frame_offset);
				arm_load_membase(inst, ARM_R1, ARM_FP,
								 insn->value1->frame_offset + 4);
			}
			jump_to_epilog(gen, &inst, block);
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

case JIT_OP_RETURN_SMALL_STRUCT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r1"));
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			//reg: address of the struct to be returned
			//imm_value: size of the struct to be returned
			
			//Prevent the accidental overwriting of the address
			int temp_reg = reg;
			if(temp_reg < 3)
			{
				arm_mov_reg_reg(inst, ARM_WORK, temp_reg);
				temp_reg = ARM_WORK;
			}
			
			//Copy the struct to the return register in a way that's appropriate to its size
			switch(imm_value)
			{
			case 1:
				arm_load_membase_byte(inst, ARM_R0, temp_reg, 0);
				break;
	
			case 2:
				arm_load_membase_ushort(inst, ARM_R0, temp_reg, 0);
				break;
	
			case 3:
				arm_load_membase_ushort(inst, ARM_R0, temp_reg, 0);
				arm_load_membase_byte(inst, ARM_R1, temp_reg, 2);
				arm_shift_reg_imm8(inst, ARM_SHL, ARM_R1, ARM_R1, 16);
				arm_alu_reg_reg(inst, ARM_ORR, ARM_R0, ARM_R0, ARM_R1);
				break;
	
			case 4:
				arm_load_membase(inst, ARM_R0, temp_reg, 0);
				break;
	
			/*TODO: is this the right way to return a struct > 4 bytes?
			* Or should it be returned by address? Look at the Procedure Call Standard!
			*/
			
			case 5:
				arm_load_membase(inst, ARM_R0, temp_reg, 0);
				arm_load_membase_byte(inst, ARM_R1, temp_reg, 4);
				break;
	
			case 6:
				arm_load_membase(inst, ARM_R0, temp_reg, 0);
				arm_load_membase_ushort(inst, ARM_R1, temp_reg, 4);
				break;
	
			case 7:
				arm_load_membase(inst, ARM_R0, temp_reg, 0);
				arm_load_membase_ushort(inst, ARM_R1, temp_reg, 4);
				arm_load_membase_byte(inst, ARM_R2, temp_reg, 6);
				arm_shift_reg_imm8(inst, ARM_SHL, ARM_R2, ARM_R2, 16);
				arm_alu_reg_reg(inst, ARM_ORR, ARM_R1, ARM_R1, ARM_R2);
				break;
	
			case 8:
				arm_load_membase(inst, ARM_R0, temp_reg, 0);
				arm_load_membase(inst, ARM_R1, temp_reg, 4);
				break;
			}
	
			jump_to_epilog(gen, &inst, block);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_SETUP_FOR_NESTED:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			jit_nint nest_reg = jit_value_get_nint_constant(insn->value1);
			if(nest_reg == -1)
			{
				arm_push_reg(inst, ARM_FP);
			}
			else
			{
				arm_mov_reg_reg(inst, _jit_reg_info[nest_reg].cpu_reg, ARM_FP);
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_SETUP_FOR_SIBLING:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			jit_nint level = jit_value_get_nint_constant(insn->value1);
			jit_nint nest_reg = jit_value_get_nint_constant(insn->value2);
			int cpu_reg;
			if(nest_reg == -1)
			{
				cpu_reg = ARM_R0;
			}
			else
			{
				cpu_reg = _jit_reg_info[nest_reg].cpu_reg;
			}
			arm_load_membase(inst, cpu_reg, ARM_FP, JIT_APPLY_PARENT_FRAME_OFFSET);
			while(level > 0)
			{
				arm_load_membase(inst, cpu_reg, cpu_reg,
								 JIT_APPLY_PARENT_FRAME_OFFSET);
				--level;
			}
			if(nest_reg == -1)
			{
				arm_push_reg(inst, cpu_reg);
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_IMPORT:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			/* TODO */
			TODO();
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_THROW:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_BRANCH);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_clobber_all(gen, &regs);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
	
			arm_push_reg(inst, reg);
			if(func->builder->setjmp_value != 0)
			{
				/* We have a "setjmp" block in the current function,
				   so we must record the location of the throw first */
				jit_nint pc_offset;
			
				_jit_gen_fix_value(func->builder->setjmp_value);
				
				pc_offset = func->builder->setjmp_value->frame_offset +
								jit_jmp_catch_pc_offset;
								
				if(func->builder->position_independent)
				{
					arm_call_imm(inst, 0);
					arm_pop_membase(inst, ARM_FP, pc_offset);
				}
				else
				{
					int pc = (int) (unsigned char *) arm_inst_get_posn(inst);
					arm_mov_membase_imm(inst, ARM_FP, pc_offset, pc, 4, ARM_WORK);
				}
			}
			arm_call(inst, (void *)jit_exception_throw);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_PC:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		{
		if(func->builder->position_independent)
		{
			arm_call_imm(inst, 0);
			arm_pop_reg(inst, reg);
		}
		else
		{
			int pc = inst.current;
			mov_reg_imm(gen, &inst, reg, pc);
		}
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ENTER_FINALLY:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{ /* 
		 * The return address is in the link register
		 * We must save it on the stack in case it will be overwritten by the content
		 * of the "finally" block.
		 * In order to respect the ABI of the ARM architecture, that prescribes an 8-byte
		 * alignment for the stack at a public interface, we save the value twice, 
		 * in order to move the current SP by 8 bytes 
		 * (we could have just saved the value once and then moved the SP by 4 bytes)
		 */
		arm_push_reg(inst, ARM_LINK);
		arm_push_reg(inst, ARM_LINK);
	}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_LEAVE_FINALLY:
{
	arm_inst_buf inst;
	{
		_jit_regs_spill_all(gen);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
		/* The "finally" return address is on the stack (twice, just for padding)*/
			arm_pop_reg(inst, ARM_LINK);
			arm_pop_reg(inst, ARM_LINK);
			arm_return(inst);
	}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_CALL_FINALLY:
{
	arm_inst_buf inst;
	{
		_jit_regs_spill_all(gen);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
		jit_block_t block;
		int offset;
		block = jit_block_from_label(func, (jit_label_t)(insn->dest));
		if(!block)
		{
			return;
		}
		if(arm_inst_get_posn(inst) >= arm_inst_get_limit(inst))
		{
			/* The buffer has overflowed, so don't worry about fixups */
			return;
		}
		if(block->address)
		{
			/* We already know the address of the block */
			arm_call(inst, block->address);
		}
		else
		{
			/* Output a placeholder and record on the block's fixup list */
			if(block->fixup_list)
			{
				offset = (int)(((unsigned char *)arm_inst_get_posn(inst)) -
				((unsigned char *)(block->fixup_list)));
			}
			else
			{
				offset = 0;
			}
			arm_call_imm(inst, offset);
			block->fixup_list = (void *)(arm_inst_get_posn(inst) - 1);
		}
	}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_ADDRESS_OF_LABEL:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
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
				mov_reg_imm(gen, &inst, reg, block->address);
			}
			else
			{
				/* Output a placeholder and record on the block's fixup list */
					mov_reg_imm(gen, &inst, reg, (int)(block->fixup_absolute_list));
					block->fixup_absolute_list = (void *)(inst.current - 1);
			}
		}
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LOAD_SBYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LOAD_UBYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LOAD_SHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LOAD_USHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value;
	jit_nint local_offset;
	if(!insn->dest->is_constant && !insn->dest->in_register && !insn->dest->has_global_register&& (insn->flags & JIT_INSN_DEST_NEXT_USE) == 0 && insn->value1->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST | _JIT_REGS_COPY);
		_jit_gen_fix_value(insn->dest);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		imm_value = insn->value1->address;
		reg = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			arm_mov_membase_imm(inst, ARM_FP, local_offset, imm_value, 4, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#ifdef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_COPY_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FLOAT_REGS */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_COPY_FLOAT32:
{
		{
			arm_inst_buf inst;
			_jit_regs_force_out(gen, insn->value1, 0);
			_jit_regs_force_out(gen, insn->dest, 1);
			_jit_gen_fix_value(insn->value1);
			_jit_gen_fix_value(insn->dest);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[0]);
			}
			else
			{
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset);
			}
			arm_store_membase(inst, ARM_WORK, ARM_FP, insn->dest->frame_offset);
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

#ifdef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_COPY_FLOAT64:
case JIT_OP_COPY_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_COPY);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FLOAT_REGS */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_COPY_FLOAT64:
case JIT_OP_COPY_NFLOAT:
{
		{
			arm_inst_buf inst;
			_jit_regs_force_out(gen, insn->value1, 0);
			_jit_regs_force_out(gen, insn->dest, 1);
			_jit_gen_fix_value(insn->value1);
			_jit_gen_fix_value(insn->dest);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[0]);
				arm_store_membase(inst, ARM_WORK, ARM_FP,
								  insn->dest->frame_offset);
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[1]);
				arm_store_membase(inst, ARM_WORK, ARM_FP,
								  insn->dest->frame_offset + 4);
			}
			else
			{
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset);
				arm_store_membase(inst, ARM_WORK, ARM_FP,
								  insn->dest->frame_offset);
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset + 4);
				arm_store_membase(inst, ARM_WORK, ARM_FP,
								  insn->dest->frame_offset + 4);
			}
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

case JIT_OP_COPY_STRUCT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	jit_nint local_offset, local_offset2;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_force_out(gen, insn->dest, 1);
		_jit_gen_fix_value(insn->dest);
		_jit_regs_force_out(gen, insn->value1, 0);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		local_offset2 = insn->value1->frame_offset;
		reg = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			inst = memory_copy(gen, inst, ARM_FP, local_offset, ARM_FP, local_offset2,
					jit_type_get_size(jit_value_get_type(insn->dest)), reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_COPY_STORE_BYTE:
{
		{
			arm_inst_buf inst;
			int reg;
			_jit_regs_force_out(gen, insn->dest, 1);
			_jit_gen_fix_value(insn->dest);
			reg = _jit_regs_load_value
				(gen, insn->value1, 0,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)));
			jit_gen_load_inst_ptr(gen, inst);
			arm_store_membase_byte(inst, _jit_reg_info[reg].cpu_reg,
								   ARM_FP, insn->dest->frame_offset);
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

case JIT_OP_COPY_STORE_SHORT:
{
		{
			arm_inst_buf inst;
			int reg;
			_jit_regs_force_out(gen, insn->dest, 1);
			_jit_gen_fix_value(insn->dest);
			reg = _jit_regs_load_value
				(gen, insn->value1, 1,
				 (insn->flags & (JIT_INSN_VALUE1_NEXT_USE |
				 				 JIT_INSN_VALUE1_LIVE)));
			jit_gen_load_inst_ptr(gen, inst);
			arm_store_membase_short(inst, _jit_reg_info[reg].cpu_reg,
								    ARM_FP, insn->dest->frame_offset);
			jit_gen_save_inst_ptr(gen, inst);
			//_jit_regs_free_reg(gen, reg, 1); //TODO: check if it's needed
		}
}
break;

case JIT_OP_ADDRESS_OF:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	jit_nint local_offset;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_force_out(gen, insn->value1, 0);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		{
			if(local_offset > 0)
			{
				arm_alu_reg_imm(inst, ARM_ADD, reg, ARM_FP, local_offset);
			}
			else if(local_offset < 0)
			{
				arm_alu_reg_imm(inst, ARM_SUB, reg, ARM_FP, -local_offset);
			}
			else
			{
				arm_mov_reg_reg(inst, reg, ARM_FP);
			}
	
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_INCOMING_REG:
case JIT_OP_RETURN_REG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
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
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_push_reg(inst, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_PUSH_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			arm_push_reg(inst, other_reg);
			arm_push_reg(inst, reg);
			gen->stack_changed=1;
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#ifdef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_PUSH_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_push_reg_float32(inst, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FLOAT_REGS */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_PUSH_FLOAT32:
{
		{
			arm_inst_buf inst;
			_jit_regs_force_out(gen, insn->value1, 0);
			_jit_gen_fix_value(insn->value1);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[0]);
			}
			else
			{
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset);
			}
			arm_push_reg(inst, ARM_WORK);
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

#ifdef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_PUSH_FLOAT64:
case JIT_OP_PUSH_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_push_reg_float64(inst, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FLOAT_REGS */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_PUSH_FLOAT64:
case JIT_OP_PUSH_NFLOAT:
{
		{
			arm_inst_buf inst;
			_jit_regs_force_out(gen, insn->value1, 0);
			_jit_gen_fix_value(insn->value1);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[1]);
				arm_push_reg(inst, ARM_WORK);
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[0]);
				arm_push_reg(inst, ARM_WORK);
			}
			else
			{
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset + 4);
				arm_push_reg(inst, ARM_WORK);
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset);
				arm_push_reg(inst, ARM_WORK);
			}
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

case JIT_OP_PUSH_STRUCT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			/* TODO */
			TODO();
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_POP_STACK:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			arm_alu_reg_imm(inst, ARM_ADD, ARM_SP, ARM_SP, insn->value1->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_FLUSH_SMALL_STRUCT:
{
	arm_inst_buf inst;
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		{
			jit_nuint size;
			jit_nint offset;
			_jit_gen_fix_value(insn->value1);
			size = jit_type_get_size(jit_value_get_type(insn->value1));
			offset = insn->value1->frame_offset;
			switch(size)
			{
				case 1:
				{
					arm_store_membase_byte(inst, ARM_R0, ARM_FP, offset);
				}
				break;
	
				case 2:
				{
					arm_store_membase_short(inst, ARM_R0, ARM_FP, offset);
				}
				break;
	
				case 3:
				{
					arm_mov_reg_reg(inst, ARM_R1, ARM_R0);
					arm_store_membase_short(inst, ARM_R0, ARM_FP, offset);
					arm_shift_reg_imm8(inst, ARM_SHR, ARM_R0, ARM_R1, 16);
					arm_store_membase_byte(inst, ARM_R0, ARM_FP, offset + 2);
				}
				break;
	
				case 4:
				{
					arm_store_membase(inst, ARM_R0, ARM_FP, offset);
				}
				break;
	
				case 5:
				{
					arm_store_membase(inst, ARM_R0, ARM_FP, offset);
					arm_store_membase_byte(inst, ARM_R1, ARM_FP, offset + 4);
				}
				break;
	
				case 6:
				{
					arm_store_membase(inst, ARM_R0, ARM_FP, offset);
					arm_store_membase_short(inst, ARM_R1, ARM_FP, offset + 4);
				}
				break;
	
				case 7:
				{
					arm_store_membase(inst, ARM_R0, ARM_FP, offset);
					arm_mov_reg_reg(inst, ARM_R2, ARM_R1);
					arm_store_membase_short(inst, ARM_R1, ARM_FP, offset + 4);
					arm_shift_reg_imm8(inst, ARM_SHR, ARM_R1, ARM_R2, 16);
					arm_store_membase_byte(inst, ARM_R1, ARM_FP, offset + 6);
				}
				break;
	
				case 8:
				{
					arm_store_membase(inst, ARM_R0, ARM_FP, offset);
					arm_store_membase(inst, ARM_R1, ARM_FP, offset + 4);
				}
				break;
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_SET_PARAM_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	jit_nint imm_value, imm_value2;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			arm_mov_membase_imm(inst, ARM_SP, imm_value2, imm_value, 4, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
	}
	else
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_mov_membase_reg(inst, ARM_SP, imm_value, reg, 4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_SET_PARAM_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		{
			arm_store_membase(inst, reg, ARM_SP, insn->value2->address);
			arm_store_membase(inst, other_reg, ARM_SP, insn->value2->address + 4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#ifdef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_SET_PARAM_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_store_membase_float32(inst, reg, ARM_SP, insn->value2->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FLOAT_REGS */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_SET_PARAM_FLOAT32:
{
		{
			arm_inst_buf inst;
			_jit_regs_force_out(gen, insn->value1, 0);
			_jit_gen_fix_value(insn->value1);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[0]);
				arm_store_membase
					(inst, ARM_WORK, ARM_SP, insn->value2->address);
			}
			else
			{
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset);
				arm_store_membase
					(inst, ARM_WORK, ARM_SP, insn->value2->address);
			}
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

#ifdef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_SET_PARAM_FLOAT64:
case JIT_OP_SET_PARAM_NFLOAT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_store_membase_float64(inst, reg, ARM_SP, insn->value2->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_FLOAT_REGS */

#ifndef JIT_ARM_HAS_FLOAT_REGS

case JIT_OP_SET_PARAM_FLOAT64:
case JIT_OP_SET_PARAM_NFLOAT:
{
		{
			arm_inst_buf inst;
			_jit_regs_force_out(gen, insn->value1, 0);
			_jit_gen_fix_value(insn->value1);
			jit_gen_load_inst_ptr(gen, inst);
			if(insn->value1->is_constant)
			{
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[0]);
				arm_store_membase
					(inst, ARM_WORK, ARM_SP, insn->value2->address);
				mov_reg_imm
					(gen, &inst, ARM_WORK, ((int *)(insn->value1->address))[1]);
				arm_store_membase
					(inst, ARM_WORK, ARM_SP, insn->value2->address + 4);
			}
			else
			{
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset);
				arm_store_membase
					(inst, ARM_WORK, ARM_SP, insn->value2->address);
				arm_load_membase(inst, ARM_WORK, ARM_FP,
								 insn->value1->frame_offset + 4);
				arm_store_membase
					(inst, ARM_WORK, ARM_SP, insn->value2->address + 4);
			}
			jit_gen_save_inst_ptr(gen, inst);
		}
}
break;

#endif /* !JIT_ARM_HAS_FLOAT_REGS */

case JIT_OP_SET_PARAM_STRUCT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		/* Handle arbitrary-sized structures */
		jit_nint offset = jit_value_get_nint_constant(insn->dest);
		inst = memory_copy(gen, inst, ARM_SP, offset, reg, 0, imm_value, reg2);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_SBYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_load_membase_sbyte(inst, reg, reg, insn->value2->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_UBYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_load_membase_byte(inst, reg, reg, insn->value2->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_SHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_load_membase_short(inst, reg, reg, insn->value2->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_USHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_load_membase_ushort(inst, reg, reg, insn->value2->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			arm_load_membase(inst, reg, reg, insn->value2->address);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_RELATIVE_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_dest_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			if(reg == reg2)
			{
				arm_mov_reg_membase(inst, other_reg, reg2, imm_value + 4, 4);
				arm_mov_reg_membase(inst, reg, reg2, imm_value, 4);
			}
			else
			{
				arm_mov_reg_membase(inst, reg, reg2, imm_value, 4);
				arm_mov_reg_membase(inst, other_reg, reg2, imm_value + 4, 4);
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_LOAD_RELATIVE_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg32);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
		arm_fld_membase(inst, reg, reg2, imm_value, 0);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_LOAD_RELATIVE_FLOAT64:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
		arm_fld_membase(inst, reg, reg2, imm_value, 1);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

case JIT_OP_LOAD_RELATIVE_NFLOAT:
{
		{
			/* TODO */
			TODO();
			abort();
		}
}
break;

case JIT_OP_LOAD_RELATIVE_STRUCT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value;
	jit_nint local_offset;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_force_out(gen, insn->dest, 1);
		_jit_gen_fix_value(insn->dest);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		local_offset = insn->dest->frame_offset;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		inst = memory_copy(gen, inst, ARM_FP, local_offset, reg, imm_value, jit_type_get_size(jit_value_get_type(insn->dest)), reg2);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_BYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2, imm_value3;
	if(insn->dest->is_constant && insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->dest->address;
		imm_value2 = insn->value1->address;
		imm_value3 = insn->value2->address;
		reg = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		arm_mov_mem_imm(inst, imm_value + imm_value3, imm_value2, 1, reg);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->dest->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->dest->address;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value2 = insn->value2->address;
		{
		arm_mov_mem_reg(inst, imm_value + imm_value2, reg, 1);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
		arm_mov_membase_imm(inst, reg, imm_value2, imm_value, 1, ARM_WORK);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
		arm_mov_membase_reg(inst, reg, imm_value, reg2, 1);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_SHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2, imm_value3;
	if(insn->dest->is_constant && insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->dest->address;
		imm_value2 = insn->value1->address;
		imm_value3 = insn->value2->address;
		reg = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		arm_mov_mem_imm(inst, imm_value + imm_value3, imm_value2, 2, reg);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->dest->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->dest->address;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value2 = insn->value2->address;
		{
		arm_mov_mem_reg(inst, imm_value + imm_value2, reg, 2);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
		arm_mov_membase_imm(inst, reg, imm_value2, imm_value, 2, ARM_WORK);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
		arm_mov_membase_reg(inst, reg, imm_value, reg2, 2);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2, imm_value3;
	if(insn->dest->is_constant && insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->dest->address;
		imm_value2 = insn->value1->address;
		imm_value3 = insn->value2->address;
		reg = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			arm_mov_mem_imm(inst, imm_value + imm_value3, imm_value2, 4, reg);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->dest->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->dest->address;
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value2 = insn->value2->address;
		{
			arm_mov_mem_reg(inst, imm_value + imm_value2, reg, 4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			arm_mov_membase_imm(inst, reg, imm_value2, imm_value, 4, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_mov_membase_reg(inst, reg, imm_value, reg2, 4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_RELATIVE_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	int other_reg2;
	jit_nint imm_value, imm_value2;
	jit_nint local_offset;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			arm_mov_membase_imm(inst, reg, imm_value2, *(int *)(imm_value), 4, ARM_WORK);
			arm_mov_membase_imm(inst, reg, imm_value2 + 4, *(int *)(imm_value + 4), 4, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value1->is_constant && !insn->value1->in_register && !insn->value1->has_global_register&& (insn->flags & JIT_INSN_VALUE1_NEXT_USE) == 0 && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_gen_fix_value(insn->value1);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		local_offset = insn->value1->frame_offset;
		imm_value = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			arm_mov_reg_membase(inst, reg2, ARM_FP, local_offset, 4);
			arm_mov_membase_reg(inst, reg, imm_value, reg2, 4);
			arm_mov_reg_membase(inst, reg2, ARM_FP, local_offset + 4, 4);
			arm_mov_membase_reg(inst, reg, imm_value + 4, reg2, 4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_lreg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		other_reg2 = _jit_reg_info[_jit_regs_get_value1_other(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_mov_membase_reg(inst, reg, imm_value, reg2, 4);
			arm_mov_membase_reg(inst, reg, imm_value + 4, other_reg2, 4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_STORE_RELATIVE_FLOAT32:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
			arm_mov_membase_imm(inst, reg, imm_value2, ((int *)(imm_value))[0], 4, ARM_WORK);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg32);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_store_membase_float32(inst, reg2, reg, imm_value);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

#ifdef JIT_ARM_HAS_VFP

case JIT_OP_STORE_RELATIVE_FLOAT64:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2;
	jit_nint imm_value, imm_value2;
	if(insn->value1->is_constant && insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		reg2 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			arm_mov_membase_imm(inst, reg, imm_value2, ((int *)(imm_value))[0], 4, reg2);
			arm_mov_membase_imm(inst, reg, imm_value2 + 4, ((int *)(imm_value))[1], 4, reg2);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_freg64);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			arm_store_membase_float64(inst, reg2, reg, imm_value);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#endif /* JIT_ARM_HAS_VFP */

case JIT_OP_STORE_RELATIVE_NFLOAT:
{
		{
			/* TODO */
			TODO();
			abort();
		}
}
break;

case JIT_OP_STORE_RELATIVE_STRUCT:
{
		{
		arm_inst_buf inst;
		int reg = _jit_regs_load_value(gen, insn->dest, 0,
		  (insn->flags & (JIT_INSN_DEST_NEXT_USE |
		  JIT_INSN_DEST_LIVE)));
		_jit_regs_spill_all(gen);
		_jit_gen_fix_value(insn->value1);
		jit_gen_load_inst_ptr(gen, inst);
		_jit_gen_check_space(gen, 128);
		reg = _jit_reg_info[reg].cpu_reg;
		inst = memory_copy(gen, inst, reg, (int)(insn->value2->address),
				ARM_FP, insn->value1->frame_offset,
				jit_type_get_size(jit_value_get_type(insn->value1)), -1);
		jit_gen_save_inst_ptr(gen, inst);
	}
}
break;

case JIT_OP_ADD_RELATIVE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
			if(insn->value2->address != 0)
			{
				arm_alu_reg_imm(inst, ARM_ADD, reg, reg, insn->value2->address);
			}
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_UBYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
		arm_widen_memindex(inst, reg, reg2, 0, reg3, 0, 0, 0);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_USHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
		arm_widen_memindex(inst, reg, reg2, 0, reg3, 1, 0, 1);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		{
		/* The last parameter is unimportant: it's not used, 
		since the displacement (4th param) is 0 */
		arm_mov_reg_memindex(inst, reg, reg2, 0, reg3, 2, 4, 0);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4, reg5;
	int other_reg;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_lreg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		other_reg = _jit_reg_info[_jit_regs_get_dest_other(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		reg5 = _jit_reg_info[_jit_regs_get_scratch(&regs, 1)].cpu_reg;
		{
		//reg=destination long register (1-st word, LSB)
		//other_reg=destination long register (2-nd word, MSB)
		//reg2=base register
		//reg3=index register
		//reg4=scratch register for arm_mov_reg_memindex
		//reg5=scratch register for overwriting prevention
		
		assert(reg2 != reg3);
		
		int basereg=reg2;
		int indexreg=reg3;
		
		//Write the 1-st word
		if(reg == basereg)
		{
			//Prevent base reg from being overwritten
			arm_mov_reg_reg(inst, reg5, basereg);
			basereg=reg5;
		}
		else if (reg == indexreg)
		{	
			//Prevent index reg from being overwritten
			arm_mov_reg_reg(inst, reg5, indexreg);
			indexreg=reg5;
		}
		arm_mov_reg_memindex(inst, reg, basereg, 0, indexreg, 3, 4, reg4);
		
		//Write the 2-nd word
		arm_mov_reg_memindex(inst, other_reg, basereg, 4, indexreg, 3, 4, reg4);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_LOAD_ELEMENT_FLOAT64:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_FREE_DEST);
		_jit_regs_init_dest(&regs, insn, 0, arm_freg64);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		arm_fld_memindex(inst, reg, reg2, 0, reg3, 3, 1, reg4);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_BYTE:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		arm_mov_memindex_reg(inst, reg, 0, reg2, 0, reg3, 1, reg4);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_SHORT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		arm_mov_memindex_reg(inst, reg, 0, reg2, 1, reg3, 2, reg4);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_INT:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		arm_mov_memindex_reg(inst, reg, 0, reg2, 2, reg3, 4, reg4);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_LONG:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	int other_reg3;
	jit_nint imm_value;
	jit_nint local_offset;
	if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
			TODO();
			abort();
			//x86_mov_memindex_imm(inst, reg, 0, reg2, 3, *(int *)(imm_value), 4);
			//x86_mov_memindex_imm(inst, reg, 4, reg2, 3, *(int *)(imm_value + 4), 4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(!insn->value2->is_constant && !insn->value2->in_register && !insn->value2->has_global_register&& (insn->flags & JIT_INSN_VALUE2_NEXT_USE) == 0)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_gen_fix_value(insn->value2);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		local_offset = insn->value2->frame_offset;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 1)].cpu_reg;
		{
			arm_mov_reg_membase(inst, reg3, ARM_FP, local_offset, 4);
			arm_mov_memindex_reg(inst, reg, 0, reg2, 3, reg3, 4, reg4);
			arm_mov_reg_membase(inst, reg3, ARM_FP, local_offset + 4, 4);
			arm_mov_memindex_reg(inst, reg, 4, reg2, 3, reg3, 4, reg4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_lreg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		other_reg3 = _jit_reg_info[_jit_regs_get_value2_other(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			arm_mov_memindex_reg(inst, reg, 0, reg2, 3, reg3, 4, reg4);
			arm_mov_memindex_reg(inst, reg, 4, reg2, 3, other_reg3, 4, reg4);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_STORE_ELEMENT_FLOAT64:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_freg64);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		arm_fst_memindex(inst, reg3, reg, 0, reg2, 3, 1, reg4);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_ALLOCA:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg;
	{
		_jit_regs_init(gen, &regs, 0);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		{
		arm_alu_reg_reg(inst, ARM_SUB, ARM_SP, ARM_SP, reg);
		arm_mov_reg_reg(inst, reg, ARM_SP);
		gen->stack_changed = 1;
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_MEMCPY:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	jit_nint imm_value;
	if(insn->value2->is_constant && (insn->value2->address <= 0))
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->value2->address;
		{ }
		jit_gen_save_inst_ptr(gen, inst);
	}
	else if(insn->value2->is_constant)
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r1"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r2"));
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			/* 
			 * Call jit_memcpy(dest,src,size).
			 * reg=dest, reg2=src, imm_value=size
			 */
			int dest=reg;
			int src=reg2;
			
			if (dest != ARM_R0) {
				if(src==ARM_R0)
				{
					//Prevent overwriting useful data
					arm_mov_reg_reg(inst, reg3, src);
					src=reg3;
				}
				arm_mov_reg_reg((inst), ARM_R0, dest);
			}
			if (src != ARM_R1) {
				//Move the "src" from wherever it is to where it should be
				arm_mov_reg_reg(inst, ARM_R1, src);
			}
			mov_reg_imm(gen, &(inst), ARM_R2, imm_value);
		    
			//Call the function
			arm_call(inst, jit_memcpy);
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r1"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r2"));
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
			/* 
			* Call jit_memcpy(dest,src,size).
			* reg=dest, reg2=src, reg3=size
			*/
			if (reg != ARM_R0) {
				if(reg2==ARM_R0)
				{
					//Prevent overwriting useful data
					arm_mov_reg_reg(inst, reg4, reg2);
				}
				arm_mov_reg_reg((inst), ARM_R0, reg);
			}
			if (reg2 != ARM_R1) {
				if (reg2==ARM_R0)
				{
					//Recover previously saved data
					arm_mov_reg_reg(inst, ARM_R1, reg4);
				}
				else
				{
					arm_mov_reg_reg((inst), ARM_R1, reg2);
				}
			}
			if (reg3 != ARM_R2) {
				arm_mov_reg_reg((inst), ARM_R2, reg3);
			}
		    
			//Call the function
			arm_call(inst, jit_memcpy);
	
		}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

case JIT_OP_MEMSET:
{
	arm_inst_buf inst;
	_jit_regs_t regs;
	int reg, reg2, reg3, reg4;
	jit_nint imm_value, imm_value2;
	if(insn->value2->is_constant && (insn->value2->address <= 0))
	{
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		imm_value = insn->value2->address;
		{ }
		jit_gen_save_inst_ptr(gen, inst);
	}
	else if(insn->value1->is_constant && insn->value2->is_constant && (insn->value2->address <= 32))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		imm_value = insn->value1->address;
		imm_value2 = insn->value2->address;
		{
		// reg = pointer to the initial memory location
		// imm_value = value to be written in memory
		// imm_value2 = length in bytes
		int disp;
		disp = 0;
		while(imm_value2 >= (disp + 4))
		{
			//NB: if 0<A<255, then A*0x01010101 = a 32-bit value where each of its four bytes is A.
			arm_mov_membase_imm(inst, reg, disp, imm_value * 0x01010101, 4, ARM_WORK);
			disp += 4;
		}
		if(imm_value2 >= (disp + 2))
		{
			arm_mov_membase_imm(inst, reg, disp, imm_value * 0x0101, 2, ARM_WORK);
			disp += 2;
		}
		if(insn->value2->address > disp)
		{
			arm_mov_membase_imm(inst, reg, disp, imm_value, 1, ARM_WORK);
		}
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address < 4))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		{
		TODO();
		abort();
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address <= 32 && (insn->value2->address % 2) == 0))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		// reg = pointer to the initial memory location
		// reg2 = value to be written in memory
		// imm_value = length in bytes
		// reg3 = scratch register
		int disp;
		arm_mov_reg_reg(inst, reg3, reg2);
		arm_shift_reg_imm8(inst, ARM_SHL, reg2, reg2, 8);
		arm_alu_reg_reg(inst, ARM_ORR, reg2, reg2, reg3);
		arm_mov_reg_reg(inst, reg3, reg2);
		arm_shift_reg_imm8(inst, ARM_SHL, reg2, reg2, 16);
		arm_alu_reg_reg(inst, ARM_ORR, reg2, reg2, reg3);
		disp = 0;
		while(imm_value >= (disp + 4))
		{
			arm_mov_membase_reg(inst, reg, disp, reg2, 4);
			disp += 4;
		}
		if(imm_value > disp)
		{
			arm_mov_membase_reg(inst, reg, disp, reg2, 2);
		}
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else if(insn->value2->is_constant && (insn->value2->address <= 32 && (insn->value2->address % 2) != 0))
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, _JIT_REGS_EARLY_CLOBBER, arm_reg);
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		imm_value = insn->value2->address;
		reg3 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		TODO();
		abort();
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
	else
	{
		_jit_regs_init(gen, &regs, _JIT_REGS_TERNARY);
		_jit_regs_init_dest(&regs, insn, 0, arm_reg);
		_jit_regs_init_value1(&regs, insn, 0, arm_reg);
		_jit_regs_init_value2(&regs, insn, 0, arm_reg);
		_jit_regs_clobber(&regs, _jit_regs_lookup("r0"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r1"));
		_jit_regs_clobber(&regs, _jit_regs_lookup("r2"));
		_jit_regs_add_scratch(&regs, arm_reg);
		_jit_regs_assign(gen, &regs);
		_jit_regs_gen(gen, &regs);
		jit_gen_load_inst_ptr(gen, inst);
		inst = (arm_inst_buf)(gen->ptr);
		reg = _jit_reg_info[_jit_regs_get_dest(&regs)].cpu_reg;
		reg2 = _jit_reg_info[_jit_regs_get_value1(&regs)].cpu_reg;
		reg3 = _jit_reg_info[_jit_regs_get_value2(&regs)].cpu_reg;
		reg4 = _jit_reg_info[_jit_regs_get_scratch(&regs, 0)].cpu_reg;
		{
		// reg = pointer to the initial memory location
		// reg2 = value to be written in memory
		// reg3 = length in bytes
		// reg4 = scratch register
		
		int pointer=reg;
		int value=reg2;
		int length=reg3;
		int scratch=reg4;
		
	
		/* Move the outgoing parameters in the right registers (if they are not already where they should be */
		if (pointer != ARM_R0) {
			if(value == ARM_R0)
			{
				//Prevent the value from being overwritten
				arm_mov_reg_reg((inst), scratch, ARM_R0);
				value=scratch;
			}
			else if(length==ARM_R0)
			{
				//Prevent the length from being overwritten
				arm_mov_reg_reg((inst), scratch, ARM_R0);
				length=scratch;
			}
			
			arm_mov_reg_reg((inst), ARM_R0, pointer);
			
			//The register that contained the pointer is now free
			scratch=pointer;
		}
		
		if (value != ARM_R1)
		{
			if (length == ARM_R1)
			{
				//The length is stored in R1. Prevent it from being overwritten
				arm_mov_reg_reg(inst, scratch, length);
				length=scratch;
			}
			
			//Set param 2
			arm_mov_reg_reg((inst), ARM_R1, value);
			
			//The register that contained the value is now free
			scratch=value;
		}
		
		if(length != ARM_R1)
		{
			//Param 3 still isn't in place: move it!
			arm_mov_reg_reg(inst, ARM_R2, length);
		}
		
		arm_call(inst, jit_memset);
	}
		jit_gen_save_inst_ptr(gen, inst);
		_jit_regs_commit(gen, &regs);
	}
}
break;

#elif defined(JIT_INCLUDE_SUPPORTED)

case JIT_OP_TRUNC_SBYTE:
case JIT_OP_TRUNC_UBYTE:
case JIT_OP_TRUNC_SHORT:
case JIT_OP_TRUNC_USHORT:
case JIT_OP_INT_TO_NFLOAT:
case JIT_OP_NFLOAT_TO_FLOAT32:
case JIT_OP_NFLOAT_TO_FLOAT64:
case JIT_OP_FLOAT64_TO_NFLOAT:
case JIT_OP_FLOAT32_TO_NFLOAT:
case JIT_OP_IADD:
case JIT_OP_ISUB:
case JIT_OP_IMUL:
case JIT_OP_IDIV:
case JIT_OP_INEG:
case JIT_OP_LADD:
case JIT_OP_LSUB:
case JIT_OP_LNEG:
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_FADD:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_FADD:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_FSUB:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_FSUB:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_FMUL:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_FMUL:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_FDIV:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_FDIV:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_FNEG:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_FNEG:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_DADD:
case JIT_OP_NFADD:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_DADD:
case JIT_OP_NFADD:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_DSUB:
case JIT_OP_NFSUB:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_DSUB:
case JIT_OP_NFSUB:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_DMUL:
case JIT_OP_NFMUL:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_DMUL:
case JIT_OP_NFMUL:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_DDIV:
case JIT_OP_NFDIV:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_DDIV:
case JIT_OP_NFDIV:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_DNEG:
case JIT_OP_NFNEG:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_DNEG:
case JIT_OP_NFNEG:
#endif
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
case JIT_OP_ICMP:
case JIT_OP_ICMP_UN:
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
case JIT_OP_CHECK_NULL:
case JIT_OP_CALL:
case JIT_OP_CALL_TAIL:
case JIT_OP_CALL_INDIRECT:
case JIT_OP_CALL_VTABLE_PTR:
case JIT_OP_CALL_EXTERNAL:
case JIT_OP_RETURN:
case JIT_OP_RETURN_INT:
case JIT_OP_RETURN_LONG:
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_RETURN_FLOAT32:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_RETURN_FLOAT32:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_RETURN_FLOAT32:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_RETURN_FLOAT64:
case JIT_OP_RETURN_NFLOAT:
#endif
#ifdef JIT_ARM_HAS_FPA
case JIT_OP_RETURN_FLOAT64:
case JIT_OP_RETURN_NFLOAT:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_RETURN_FLOAT64:
case JIT_OP_RETURN_NFLOAT:
#endif
case JIT_OP_RETURN_SMALL_STRUCT:
case JIT_OP_SETUP_FOR_NESTED:
case JIT_OP_SETUP_FOR_SIBLING:
case JIT_OP_IMPORT:
case JIT_OP_THROW:
case JIT_OP_LOAD_PC:
case JIT_OP_ENTER_FINALLY:
case JIT_OP_LEAVE_FINALLY:
case JIT_OP_CALL_FINALLY:
case JIT_OP_ADDRESS_OF_LABEL:
case JIT_OP_COPY_LOAD_SBYTE:
case JIT_OP_COPY_LOAD_UBYTE:
case JIT_OP_COPY_LOAD_SHORT:
case JIT_OP_COPY_LOAD_USHORT:
case JIT_OP_COPY_INT:
case JIT_OP_COPY_LONG:
#ifdef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_COPY_FLOAT32:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_COPY_FLOAT32:
#endif
#ifdef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_COPY_FLOAT64:
case JIT_OP_COPY_NFLOAT:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_COPY_FLOAT64:
case JIT_OP_COPY_NFLOAT:
#endif
case JIT_OP_COPY_STRUCT:
case JIT_OP_COPY_STORE_BYTE:
case JIT_OP_COPY_STORE_SHORT:
case JIT_OP_ADDRESS_OF:
case JIT_OP_INCOMING_REG:
case JIT_OP_RETURN_REG:
case JIT_OP_PUSH_INT:
case JIT_OP_PUSH_LONG:
#ifdef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_PUSH_FLOAT32:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_PUSH_FLOAT32:
#endif
#ifdef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_PUSH_FLOAT64:
case JIT_OP_PUSH_NFLOAT:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_PUSH_FLOAT64:
case JIT_OP_PUSH_NFLOAT:
#endif
case JIT_OP_PUSH_STRUCT:
case JIT_OP_POP_STACK:
case JIT_OP_FLUSH_SMALL_STRUCT:
case JIT_OP_SET_PARAM_INT:
case JIT_OP_SET_PARAM_LONG:
#ifdef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_SET_PARAM_FLOAT32:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_SET_PARAM_FLOAT32:
#endif
#ifdef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_SET_PARAM_FLOAT64:
case JIT_OP_SET_PARAM_NFLOAT:
#endif
#ifndef JIT_ARM_HAS_FLOAT_REGS
case JIT_OP_SET_PARAM_FLOAT64:
case JIT_OP_SET_PARAM_NFLOAT:
#endif
case JIT_OP_SET_PARAM_STRUCT:
case JIT_OP_LOAD_RELATIVE_SBYTE:
case JIT_OP_LOAD_RELATIVE_UBYTE:
case JIT_OP_LOAD_RELATIVE_SHORT:
case JIT_OP_LOAD_RELATIVE_USHORT:
case JIT_OP_LOAD_RELATIVE_INT:
case JIT_OP_LOAD_RELATIVE_LONG:
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_LOAD_RELATIVE_FLOAT32:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_LOAD_RELATIVE_FLOAT64:
#endif
case JIT_OP_LOAD_RELATIVE_NFLOAT:
case JIT_OP_LOAD_RELATIVE_STRUCT:
case JIT_OP_STORE_RELATIVE_BYTE:
case JIT_OP_STORE_RELATIVE_SHORT:
case JIT_OP_STORE_RELATIVE_INT:
case JIT_OP_STORE_RELATIVE_LONG:
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_STORE_RELATIVE_FLOAT32:
#endif
#ifdef JIT_ARM_HAS_VFP
case JIT_OP_STORE_RELATIVE_FLOAT64:
#endif
case JIT_OP_STORE_RELATIVE_NFLOAT:
case JIT_OP_STORE_RELATIVE_STRUCT:
case JIT_OP_ADD_RELATIVE:
case JIT_OP_LOAD_ELEMENT_UBYTE:
case JIT_OP_LOAD_ELEMENT_USHORT:
case JIT_OP_LOAD_ELEMENT_INT:
case JIT_OP_LOAD_ELEMENT_LONG:
case JIT_OP_LOAD_ELEMENT_FLOAT64:
case JIT_OP_STORE_ELEMENT_BYTE:
case JIT_OP_STORE_ELEMENT_SHORT:
case JIT_OP_STORE_ELEMENT_INT:
case JIT_OP_STORE_ELEMENT_LONG:
case JIT_OP_STORE_ELEMENT_FLOAT64:
case JIT_OP_ALLOCA:
case JIT_OP_MEMCPY:
case JIT_OP_MEMSET:
	return 1;

#endif
