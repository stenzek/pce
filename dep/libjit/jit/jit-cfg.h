/*
 * jit-cfg.h - Control Flow Graph routines for the JIT.
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

#ifndef	_JIT_CFG_H
#define	_JIT_CFG_H

#include <config.h>
#include "jit-bitset.h"

#define _JIT_BLOCK_CFG_NODE 10010

#define _JIT_NODE_VISITED 1

typedef struct _jit_cfg *_jit_cfg_t;
typedef struct _jit_node *_jit_node_t;
typedef struct _jit_edge *_jit_edge_t;
typedef struct _jit_value_entry *_jit_value_entry_t;

/*
 * Control flow graph.
 */
struct _jit_cfg
{
	jit_function_t		func;

	/* Special nodes */
	_jit_node_t		entry;
	_jit_node_t		exit;

	/* Array of nodes */
	_jit_node_t		nodes;
	int			num_nodes;

	/* Array of edges */
	_jit_edge_t		edges;
	int			num_edges;

	/* depth first search post order. */
	_jit_node_t		*post_order;

	/* values */
	_jit_value_entry_t	values;
	int			num_values;
	int			max_values;
};

/*
 * Control flow graph node.
 */
struct _jit_node
{
	jit_block_t		block;
	int			flags;

	/* edges to successor nodes */
	_jit_edge_t		*succs;
	int			num_succs;

	/* edges to predecessor nodes */
	_jit_edge_t		*preds;
	int			num_preds;

	/* liveness analysis data */
	_jit_bitset_t		live_in;
	_jit_bitset_t		live_out;
	_jit_bitset_t		live_use;
	_jit_bitset_t		live_def;

	/* depth first search number */
	int			dfn;
};

/*
 * Control flow graph edge.
 */
// struct _jit_edge
// {
// 	_jit_node_t		src;
// 	_jit_node_t		dst;
// 	int			flags;
// };

/*
 * Value entry that contains information for data flow analysis
 * and register allocation.
 */
struct _jit_value_entry
{
	jit_value_t		value;
	short			reg;
	short			other_reg;
};

#endif
