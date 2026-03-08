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

bool datatype_is_struct_or_union(struct datatype* dtype)
{
    return dtype->type == DATA_TYPE_STRUCT || dtype->type == DATA_TYPE_UNION;
}

bool datatype_is_struct_or_union_for_name(const char* name)
{
    return S_EQ(name,"union") || S_EQ(name, "struct");
}

size_t datatype_size_for_array_access(struct datatype* dtype)
{
    if (datatype_is_struct_or_union(dtype) && dtype->flags & DATATYPE_FLAG_IS_POINTER && 
        dtype->pointer_depth == 1)
    {
        // struct abc* abc; abc[0];
        return dtype->size;
    }

    return datatype_size(dtype);
}

bool datatype_is_void_no_ptr(struct datatype* dtype)
{
    return S_EQ(dtype->type_str, "void") && !(dtype->flags & DATATYPE_FLAG_IS_POINTER);
}

void datatype_set_void(struct datatype* dtype)
{
    dtype->type = DATA_TYPE_VOID;
    dtype->type_str = "void";
    dtype->size = 0;
}
size_t datatype_element_size(struct datatype* dtype)
{
    if (dtype->flags & DATATYPE_FLAG_IS_POINTER)
    {
        return DATA_SIZE_DWORD;
    }

    return dtype->size;
}

size_t datatype_size_no_ptr(struct datatype* dtype)
{
    if (dtype->flags & DATATYPE_FLAG_IS_ARRAY)
    {
        return dtype->array.size;
    }

    return dtype->size;
}

size_t datatype_size(struct datatype* dtype)
{
    if (dtype->flags & DATATYPE_FLAG_IS_POINTER && dtype->pointer_depth > 0)
    {
        return DATA_SIZE_DWORD;
    }

    if (dtype->flags & DATATYPE_FLAG_IS_ARRAY)
    {
        return dtype->array.size;
    }

    return dtype->size;
}

bool datatype_is_primitive(struct datatype* dtype)
{
    return !datatype_is_struct_or_union(dtype);
}

bool datatype_is_struct_or_union_non_pointer(struct datatype* dtype)
{
    return dtype->type != DATA_TYPE_UNKNOWN && !datatype_is_primitive(dtype) && !(dtype->flags & DATATYPE_FLAG_IS_POINTER);  
}
