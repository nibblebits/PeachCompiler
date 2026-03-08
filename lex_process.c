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
#include <stdlib.h>

struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* functions, void* private)
{
    struct lex_process* process = calloc(1, sizeof(struct lex_process));
    process->function = functions;
    process->token_vec = vector_create(sizeof(struct token));
    process->compiler = compiler;
    process->private = private;
    process->pos.line = 1;
    process->pos.col = 1;
    return process;
}

void lex_process_free(struct lex_process* process)
{
    vector_free(process->token_vec);
    free(process);
}

void* lex_process_private(struct lex_process* process)
{
    return process->private;
}

struct vector* lex_process_tokens(struct lex_process* process)
{
    return process->token_vec;
}