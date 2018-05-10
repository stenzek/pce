/*
 * jit-cfg.c - Control Flow Graph routines for the JIT.
 *
 * Copyright (C) 2006  Southern Storm Software, Pty Ltd.
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

#include "jit-internal.h"
#include "jit-cfg.h"

static void
init_node(_jit_node_t node, jit_block_t block)
{
	node->block = block;
	if(block)
	{
		jit_block_set_meta(block, _JIT_BLOCK_CFG_NODE, node, 0);
	}
	node->flags = 0;
	node->succs = 0;
	node->num_succs = 0;
	node->preds = 0;
	node->num_preds = 0;

	_jit_bitset_init(&node->live_in);
	_jit_bitset_init(&node->live_out);
	_jit_bitset_init(&node->live_use);
	_jit_bitset_init(&node->live_def);

	node->dfn = -1;
}

static void
init_edge(_jit_edge_t edge)
{
	edge->src = 0;
	edge->dst = 0;
	edge->flags = 0;
}

static void
init_value_entry(_jit_value_entry_t value)
{
	value->value = 0;
}

static _jit_node_t
create_node()
{
	_jit_node_t node;
	node = jit_new(struct _jit_node);
	if(node)
	{
		init_node(node, 0);
	}
	return node;
}

_jit_cfg_t
create_cfg(jit_function_t func)
{
	_jit_cfg_t cfg;

	cfg = jit_new(struct _jit_cfg);
	if(!cfg)
	{
		return 0;
	}

	cfg->entry = create_node();
	if(!cfg->entry)
	{
		jit_free(cfg);
		return 0;
	}

	cfg->exit = create_node();
	if(!cfg->exit)
	{
		jit_free(cfg->entry);
		jit_free(cfg);
		return 0;
	}

	cfg->func = func;
	cfg->nodes = 0;
	cfg->num_nodes = 0;
	cfg->edges = 0;
	cfg->num_edges = 0;
	cfg->post_order = 0;
	cfg->values = 0;
	cfg->num_values = 0;
	cfg->max_values = 0;

	return cfg;
}

static int
build_nodes(_jit_cfg_t cfg, jit_function_t func)
{
	int count;
	jit_block_t block;

	count = 0;
	block = 0;
	while((block = jit_block_next(func, block)) != 0)
	{
		++count;
	}

	cfg->num_nodes = count;
	cfg->nodes = jit_malloc(count * sizeof(struct _jit_node));
	if(!cfg->nodes)
	{
		return 0;
	}

	count = 0;
	block = 0;
	while((block = jit_block_next(func, block)) != 0)
	{
		init_node(&cfg->nodes[count++], block);
	}

	return 1;
}

static _jit_node_t
get_next_node(_jit_cfg_t cfg, _jit_node_t node)
{
	int index = (node - cfg->nodes) + 1;
	if(index < cfg->num_nodes)
	{
		return cfg->nodes + index;
	}
	else
	{
		return cfg->exit;
	}
}

static _jit_node_t
get_label_node(_jit_cfg_t cfg, jit_label_t label)
{
	jit_block_t block;
	block = jit_block_from_label(cfg->func, label);
	if(!block)
	{
		return 0;
	}
	return jit_block_get_meta(block, _JIT_BLOCK_CFG_NODE);
}

static _jit_node_t
get_catcher_node(_jit_cfg_t cfg)
{
	jit_label_t label;
	label = cfg->func->builder->catcher_label;
	if(label == jit_label_undefined)
	{
		return cfg->exit;
	}
	return get_label_node(cfg, label);
}

static void
enum_edge(_jit_cfg_t cfg, _jit_node_t src, _jit_node_t dst, int flags, int create)
{
	if(!cfg || !src || !dst)
	{
		return;
	}

	if(create)
	{
		cfg->edges[cfg->num_edges].src = src;
		cfg->edges[cfg->num_edges].dst = dst;
		cfg->edges[cfg->num_edges].flags = flags;
		src->succs[src->num_succs] = &cfg->edges[cfg->num_edges];
		dst->preds[dst->num_preds] = &cfg->edges[cfg->num_edges];
	}

	++(cfg->num_edges);
	++(src->num_succs);
	++(dst->num_preds);
}

static void
enum_node_edges(_jit_cfg_t cfg, _jit_node_t node, int create)
{
	jit_insn_t insn;
	jit_label_t label;
	jit_label_t *labels;
	int index, num_labels;

	/* TODO: Handle catch, finally, filter blocks and calls. */

	insn = _jit_block_get_last(node->block);
	if(!insn)
	{
		/* empty block: create a fall-through edge */
		enum_edge(cfg, node, get_next_node(cfg, node), 0, create);
	}
	else if(insn->opcode == JIT_OP_BR)
	{
		label = (jit_label_t) insn->dest;
		enum_edge(cfg, node, get_label_node(cfg, label), 0, create);
	}
	else if(insn->opcode >= JIT_OP_BR_IFALSE && insn->opcode <= JIT_OP_BR_NFGE_INV)
	{
		label = (jit_label_t) insn->dest;
		enum_edge(cfg, node, get_label_node(cfg, label), 0, create);
		enum_edge(cfg, node, get_next_node(cfg, node), 0, create);
	}
	else if(insn->opcode >= JIT_OP_RETURN && insn->opcode <= JIT_OP_RETURN_SMALL_STRUCT)
	{
		enum_edge(cfg, node, cfg->exit, 0, create);
	}
	else if(insn->opcode == JIT_OP_THROW)
	{
		enum_edge(cfg, node, get_catcher_node(cfg), 0, create);
	}
	else if(insn->opcode == JIT_OP_RETHROW)
	{
		enum_edge(cfg, node, cfg->exit, 0, create);
	}
	else if(insn->opcode == JIT_OP_JUMP_TABLE)
	{
		labels = (jit_label_t *) insn->value1->address;
		num_labels = (int) insn->value2->address;
		for(index = 0; index < num_labels; index++)
		{
			enum_edge(cfg, node, get_label_node(cfg, labels[index]), 0, create);
		}
		enum_edge(cfg, node, get_next_node(cfg, node), 0, create);
	}
	else
	{
		/* otherwise create a fall-through edge */
		enum_edge(cfg, node, get_next_node(cfg, node), 0, create);
	}
}

static void
enum_all_edges(_jit_cfg_t cfg, jit_function_t func, int create)
{
	int index;

	if(cfg->num_nodes == 0)
	{
		enum_edge(cfg, cfg->entry, cfg->exit, 0, create);
	}
	else
	{
		enum_edge(cfg, cfg->entry, &cfg->nodes[0], 0, create);
		for(index = 0; index < cfg->num_nodes; index++)
		{
			enum_node_edges(cfg, &cfg->nodes[index], create);
		}
	}
}

static int
create_edges(_jit_cfg_t cfg, jit_function_t func)
{
	int index;

	if(cfg->num_edges == 0)
	{
		return 1;
	}

	cfg->edges = jit_malloc(cfg->num_edges * sizeof(struct _jit_edge));
	if(!cfg->edges)
	{
		return 0;
	}
	for(index = 0; index < cfg->num_edges; index++)
	{
		init_edge(&cfg->edges[index]);
	}
	for(index = 0; index < cfg->num_nodes; index++)
	{
		if(cfg->nodes[index].num_succs > 0)
		{
			cfg->nodes[index].succs = jit_calloc(cfg->nodes[index].num_succs,
							     sizeof(_jit_edge_t));
			if(!cfg->nodes[index].succs)
			{
				return 0;
			}
			cfg->nodes[index].num_succs = 0;
		}
		if(cfg->nodes[index].num_preds > 0)
		{
			cfg->nodes[index].preds = jit_calloc(cfg->nodes[index].num_preds,
							     sizeof(_jit_edge_t));
			if(!cfg->nodes[index].preds)
			{
				return 0;
			}
			cfg->nodes[index].num_preds = 0;
		}
	}

	cfg->num_edges = 0;
	return 1;
}

static int
build_edges(_jit_cfg_t cfg, jit_function_t func)
{
	enum_all_edges(cfg, func, 0);
	if(!create_edges(cfg, func))
	{
		return 0;
	}
	enum_all_edges(cfg, func, 1);
	return 1;
}

static int
compute_depth_first_order(_jit_cfg_t cfg)
{
	struct stack_entry
	{
		_jit_node_t node;
		int index;
	} *stack;
	_jit_node_t node;
	_jit_node_t succ;
	int post_order_num;
	int sp;
	int index;

	if(cfg->post_order)
	{
		return 1;
	}

	stack = jit_malloc((cfg->num_nodes + 1) * sizeof(struct stack_entry));
	if(!stack)
	{
		return 0;
	}

	cfg->post_order = jit_calloc(cfg->num_nodes, sizeof(_jit_node_t));
	if(!cfg->post_order)
	{
		jit_free(stack);
		return 0;
	}

	post_order_num = 0;

	stack[0].node = cfg->entry;
	stack[0].index = 0;
	sp = 1;

	while(sp)
	{
		node = stack[sp - 1].node;
		index = stack[sp - 1].index;

		succ = node->succs[index]->dst;
		if(succ != cfg->exit && (succ->flags & _JIT_NODE_VISITED) == 0)
		{
			succ->flags |= _JIT_NODE_VISITED;
			if(succ->num_succs > 0)
			{
				stack[sp].node = succ;
				stack[sp].index = 0;
				++sp;
			}
			else
			{
				cfg->post_order[post_order_num++] = succ;
			}
		}
		else
		{
			if(index < node->num_succs)
			{
				stack[sp - 1].index = index + 1;
			}
			else
			{
				if(node != cfg->entry)
				{
					cfg->post_order[post_order_num++] = node;
				}
				--sp;
			}
		}
	}

	jit_free(stack);
	return 1;
}

static jit_value_t
get_dest(jit_insn_t insn)
{
	if(insn->opcode == JIT_OP_NOP
	   || (insn->flags & JIT_INSN_DEST_OTHER_FLAGS) != 0
	   || (insn->dest && insn->dest->is_constant))
	{
		return 0;
	}
	return insn->dest;
}

static jit_value_t
get_value1(jit_insn_t insn)
{
	if(insn->opcode == JIT_OP_NOP
	   || (insn->flags & JIT_INSN_VALUE1_OTHER_FLAGS) != 0
	   || (insn->value1 && insn->value1->is_constant))
	{
		return 0;
	}
	return insn->value1;
}

static jit_value_t
get_value2(jit_insn_t insn)
{
	if(insn->opcode == JIT_OP_NOP
	   || (insn->flags & JIT_INSN_VALUE2_OTHER_FLAGS) != 0
	   || (insn->value2 && insn->value2->is_constant))
	{
		return 0;
	}
	return insn->value2;
}

static int
create_value_entry(_jit_cfg_t cfg, jit_value_t value)
{
	_jit_value_entry_t values;
	int max_values;

	if(value->index >= 0)
	{
		return 1;
	}

	if(cfg->num_values == cfg->max_values)
	{
		if(cfg->max_values == 0)
		{
			max_values = 20;
			values = jit_malloc(max_values * sizeof(struct _jit_value_entry));
		}
		else
		{
			max_values = cfg->max_values * 2;
			values = jit_realloc(cfg->values, max_values * sizeof(struct _jit_value_entry));
		}
		if(!values)
		{
			return 0;
		}
		cfg->values = values;
		cfg->max_values = max_values;
	}

	value->index = cfg->num_values++;
	init_value_entry(&cfg->values[value->index]);

	return 1;
}

static int
use_value(_jit_cfg_t cfg, _jit_node_t node, jit_value_t value)
{
	if(value->index < 0)
	{
		return 1;
	}
	if(_jit_bitset_is_allocated(&node->live_def)
	   && _jit_bitset_test_bit(&node->live_def, value->index))
	{
		return 1;
	}
	if(!_jit_bitset_is_allocated(&node->live_use)
	   && !_jit_bitset_allocate(&node->live_use, cfg->num_values))
	{
		return 0;
	}
	_jit_bitset_set_bit(&node->live_use, value->index);
	return 1;
}

static int
def_value(_jit_cfg_t cfg, _jit_node_t node, jit_value_t value)
{
	if(!_jit_bitset_is_allocated(&node->live_def)
	   && !_jit_bitset_allocate(&node->live_def, cfg->num_values))
	{
		return 0;
	}
	_jit_bitset_set_bit(&node->live_def, value->index);
	return 1;
}

static int
create_value_entries(_jit_cfg_t cfg)
{
	int index;
	_jit_node_t node;
	jit_insn_iter_t iter;
	jit_insn_t insn;
	jit_value_t dest;
	jit_value_t value1;
	jit_value_t value2;

	for(index = 0; index < cfg->num_nodes; index++)
	{
		node = &cfg->nodes[index];
		jit_insn_iter_init(&iter, node->block);
		while((insn = jit_insn_iter_next(&iter)) != 0)
		{
			dest = get_dest(insn);
			value1 = get_value1(insn);
			value2 = get_value1(insn);

			if(dest && !create_value_entry(cfg, dest))
			{
				return 0;
			}
			if(value1 && !create_value_entry(cfg, value1))
			{
				return 0;
			}
			if(value2 && !create_value_entry(cfg, value2))
			{
				return 0;
			}
		}
	}

	return 1;
}

static int
compute_local_live_sets(_jit_cfg_t cfg)
{
	int index;
	_jit_node_t node;
	jit_insn_iter_t iter;
	jit_insn_t insn;
	jit_value_t dest;
	jit_value_t value1;
	jit_value_t value2;

	for(index = 0; index < cfg->num_nodes; index++)
	{
		node = &cfg->nodes[index];
		jit_insn_iter_init(&iter, node->block);
		while((insn = jit_insn_iter_next(&iter)) != 0)
		{
			dest = get_dest(insn);
			value1 = get_value1(insn);
			value2 = get_value2(insn);

			if(value1 && !use_value(cfg, node, value1))
			{
				return 0;
			}
			if(value2 && !use_value(cfg, node, value2))
			{
				return 0;
			}
			if(dest)
			{
				if((insn->flags & JIT_INSN_DEST_IS_VALUE) != 0)
				{
					if(!use_value(cfg, node, dest))
					{
						return 0;
					}
				}
				else
				{
					if(!def_value(cfg, node, dest))
					{
						return 0;
					}
				}
			}
		}
	}

	return 1;
}

static int
compute_global_live_sets(_jit_cfg_t cfg)
{
	int change;
	int index, succ_index;
	_jit_node_t node;
	_jit_node_t succ;
	_jit_bitset_t bitset;

	if(!_jit_bitset_allocate(&bitset, cfg->num_values))
	{
		return 0;
	}

	do
	{
		change = 0;
		for(index = 0; index < cfg->num_nodes; index++)
		{
			node = cfg->post_order[index];
			if(!node)
			{
				continue;
			}

			_jit_bitset_clear(&bitset);
			for(succ_index = 0; succ_index < node->num_succs; succ_index++)
			{
				succ = node->succs[succ_index]->dst;
				if(_jit_bitset_is_allocated(&succ->live_in))
				{
					_jit_bitset_add(&bitset, &succ->live_in);
				}
			}
			if(!_jit_bitset_is_allocated(&node->live_out)
			   && !_jit_bitset_allocate(&node->live_out, cfg->num_values))
			{
				_jit_bitset_free(&bitset);
				return 0;
			}
			if(_jit_bitset_copy(&node->live_out, &bitset))
			{
				change = 1;
			}

			_jit_bitset_sub(&bitset, &node->live_def);
			_jit_bitset_add(&bitset, &node->live_use);
			if(!_jit_bitset_is_allocated(&node->live_in)
			   && !_jit_bitset_allocate(&node->live_in, cfg->num_values))
			{
				_jit_bitset_free(&bitset);
				return 0;
			}
			if(_jit_bitset_copy(&node->live_in, &bitset))
			{
				change = 1;
			}
		}
	}
	while(change);

	_jit_bitset_free(&bitset);
	return 1;
}

void
_jit_cfg_free(_jit_cfg_t cfg)
{
	int index;

	if(cfg->nodes)
	{
		for(index = 0; index < cfg->num_nodes; index++)
		{
			if(cfg->nodes[index].succs)
			{
				jit_free(cfg->nodes[index].succs);
			}
			if(cfg->nodes[index].preds)
			{
				jit_free(cfg->nodes[index].preds);
			}
		}
		jit_free(cfg->nodes);
	}
	if(cfg->edges)
	{
		jit_free(cfg->edges);
	}
	if(cfg->post_order)
	{
		jit_free(cfg->post_order);
	}
	if(cfg->values)
	{
		jit_free(cfg->values);
	}
	jit_free(cfg->entry);
	jit_free(cfg->exit);
	jit_free(cfg);
}

_jit_cfg_t
_jit_cfg_build(jit_function_t func)
{
	_jit_cfg_t cfg;

	cfg = create_cfg(func);
	if(!cfg)
	{
		return 0;
	}
	if(!build_nodes(cfg, func) || !build_edges(cfg, func))
	{
		_jit_cfg_free(cfg);
		return 0;
	}
	if(!compute_depth_first_order(cfg))
	{
		_jit_cfg_free(cfg);
		return 0;
	}

	return cfg;
}

int
_jit_cfg_compute_liveness(_jit_cfg_t cfg)
{
	return (create_value_entries(cfg)
		&& compute_local_live_sets(cfg)
		&& compute_global_live_sets(cfg));
}
