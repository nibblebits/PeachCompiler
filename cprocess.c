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

#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "helpers/vector.h"

const char* default_include_dirs[] = {"./pc_includes", "../pc_includes", "/usr/include/peach-includes", "/usr/include"};

const char* compiler_include_dir_begin(struct compile_process* process)
{
    vector_set_peek_pointer(process->include_dirs, 0);
    const char* dir = vector_peek_ptr(process->include_dirs);
    return dir;
}

const char* compiler_include_dir_next(struct compile_process* process)
{
    const char* dir =vector_peek_ptr(process->include_dirs);
    return dir;
}

void compiler_setup_default_include_directories(struct vector* include_vec)
{
    size_t total = sizeof(default_include_dirs) / sizeof(const char*);
    for (int i = 0; i < total; i++)
    {
        vector_push(include_vec, &default_include_dirs[i]);
    }
}

struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags, struct compile_process* parent_process)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return NULL;
    }

    FILE *out_file = NULL;
    if (filename_out)
    {
        out_file = fopen(filename_out, "w");
        if (!out_file)
        {
            return NULL;
        }
    }

    struct compile_process* process = calloc(1, sizeof(struct compile_process));
    process->token_vec = vector_create(sizeof(struct token));
    process->token_vec_original = vector_create(sizeof(struct token));
    process->node_vec = vector_create(sizeof(struct node*));
    process->node_tree_vec = vector_create(sizeof(struct node*));
    
    process->flags = flags;
    process->cfile.fp = file;
    process->ofile = out_file;
    process->generator = codegenerator_new(process);
    process->resolver = resolver_default_new_process(process);

    symresolver_initialize(process);
    symresolver_new_table(process);
    
    if (parent_process)
    {
        process->preprocessor = parent_process->preprocessor;
        process->include_dirs = parent_process->include_dirs;
    }
    else
    {
        process->preprocessor = preprocessor_create(process);
        process->include_dirs = vector_create(sizeof(const char*));
        // Load the default include directories
        compiler_setup_default_include_directories(process->include_dirs);
    }
    
    char* path = malloc(PATH_MAX);
    realpath(filename, path);
    process->cfile.abs_path = path;
    node_set_vector(process->node_vec, process->node_tree_vec);
    return process;
}

char compile_process_next_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler;
    compiler->pos.col += 1;
    char c = getc(compiler->cfile.fp);
    if (c == '\n')
    {
        compiler->pos.line +=1 ;
        compiler->pos.col = 1;
    }

    return c;
}

char compile_process_peek_char(struct lex_process* lex_process)
{
    struct compile_process* compiler = lex_process->compiler;
    char c = getc(compiler->cfile.fp);
    ungetc(c, compiler->cfile.fp);
    return c;
}

void compile_process_push_char(struct lex_process* lex_process, char c)
{
    struct compile_process* compiler = lex_process->compiler;
    ungetc(c, compiler->cfile.fp);
}