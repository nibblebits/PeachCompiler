/*
 * PeachCompiler C Compiler Project
 * Copyright (C) 2026 Daniel McCarthy <daniel@dragonzap.com>
 * This file is part of the PeachCompiler.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * For full source code, documentation, and structured learning,
 * see the official compiler development video course by Daniel McCarthy here:
 * dragonzap.com/course/creating-a-c-compiler-from-scratch
 *
 * Learn to build your own C compiler from scratch with that video course over 39 hours of video content available.
 */

#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

struct array_brackets* array_brackets_new()
{
    struct array_brackets* brackets = calloc(1, sizeof(struct array_brackets));
    brackets->n_brackets = vector_create(sizeof(struct node*));
    return brackets;
}

void array_brackets_free(struct array_brackets* brackets)
{
    free(brackets);
}

void array_brackets_add(struct array_brackets* brackets, struct node* bracket_node)
{
    assert(bracket_node->type == NODE_TYPE_BRACKET);
    vector_push(brackets->n_brackets, &bracket_node);
}

struct vector* array_brackets_node_vector(struct array_brackets* brackets)
{
    return brackets->n_brackets;
}

size_t array_brackets_calculate_size_from_index(struct datatype* dtype, struct array_brackets* brackets, int index)
{
    struct vector* array_vec = array_brackets_node_vector(brackets);
    size_t size = dtype->size;
    if (index >= vector_count(array_vec))
    {
        // char* abc;
        // return abc[0]; return abc[1];
        return size;
    }

    vector_set_peek_pointer(array_vec, index);
    struct node* array_bracket_node = vector_peek_ptr(array_vec);
    if (!array_bracket_node)
    {
        return 0;
    }

    while(array_bracket_node)
    {
        assert(array_bracket_node->bracket.inner->type == NODE_TYPE_NUMBER);
        int number = array_bracket_node->bracket.inner->llnum;
        size *= number;
        array_bracket_node = vector_peek_ptr(array_vec);
    }

    return size;
}

size_t array_brackets_count(struct datatype* dtype)
{
    return vector_count(dtype->array.brackets->n_brackets);
}

size_t array_brackets_calculate_size(struct datatype* dtype, struct array_brackets* brackets)
{
    return array_brackets_calculate_size_from_index(dtype, brackets, 0);
}

int array_total_indexes(struct datatype* dtype)
{
    assert(dtype->flags & DATATYPE_FLAG_IS_ARRAY);
    struct array_brackets* brackets = dtype->array.brackets;
    return vector_count(brackets->n_brackets);
}
