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
#include <stdlib.h>

int preprocessor_line_macro_evaluate(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments)
{
    struct preprocessor* preprocessor = definition->preprocessor;
    struct compile_process* compiler = preprocessor->compiler;

    if (arguments)
    {
        compiler_error(compiler, "__LINE__ macro expects no arguments");
    }   

    struct token* previous_token = preprocessor_previous_token(compiler);
    return previous_token->pos.line;
}

struct vector* preprocessor_line_macro_value(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments)
{
    struct preprocessor* preprocessor = definition->preprocessor;
    struct compile_process* compiler = preprocessor->compiler;

    if (arguments)
    {
        compiler_error(compiler, "__LINE__ macro expects no arguments");
    }   
    struct token* previous_token = preprocessor_previous_token(compiler);
    return preprocessor_build_value_vector_for_integer(previous_token->pos.line);
}

void preprocessor_create_definitions(struct preprocessor* preprocessor)
{
    preprocessor_definition_create_native("__LINE__", preprocessor_line_macro_evaluate, preprocessor_line_macro_value, preprocessor);   
}

struct symbol* native_create_function(struct compile_process* compiler, const char* name,
 struct native_function_callbacks* callbacks)
{
    struct native_function* func = calloc(1, sizeof(struct native_function));
    memcpy(&func->callbacks, callbacks, sizeof(func->callbacks));
    func->name = name;
    return symresolver_register_symbol(compiler, name, SYMBOL_TYPE_NATIVE_FUNCTION, func);
}

struct native_function* native_function_get(struct compile_process* compiler, const char* name)
{
    struct symbol* sym = symresolver_get_symbol_for_native_function(compiler, name);
    if (!sym)
    {
        return NULL;
    }

    return sym->data;
}
