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
#include "helpers/vector.h"
#include "compiler.h"
int main(int argc, char** argv)
{
    const char* input_file = "./test.c";
    const char* output_file = "./test";
    const char* option = "exec";

    if (argc > 1)
    {
        input_file = argv[1];
    }

    if (argc > 2)
    {
        output_file = argv[2];
    }

    if (argc > 3)
    {
        option = argv[3];
    }
    int compile_flags = COMPILE_PROCESS_EXECUTE_NASM;
    if (S_EQ(option, "object"))
    {
        compile_flags |= COMPILE_PROCESS_EXPORT_AS_OBJECT;
    }
    int res = compile_file(input_file, output_file, compile_flags);
    if (res == COMPILER_FILE_COMPILED_OK)
    {
        printf("everything compiled file\n");
    }
    else if(res == COMPILER_FAILED_WITH_ERRORS)
    {
        printf("Compile failed\n");
    }
    else
    {
        printf("Unknown response for compile time\n");
    }

    if (compile_flags & COMPILE_PROCESS_EXECUTE_NASM)
    {
        char nasm_output_file[40];
        char nasm_cmd[512];
        sprintf(nasm_output_file, "%s.o", output_file);
        if (compile_flags & COMPILE_PROCESS_EXPORT_AS_OBJECT)
        {
            sprintf(nasm_cmd, "nasm -f elf32 %s -o %s", output_file, nasm_output_file);
        }
        else
        {
            sprintf(nasm_cmd, "nasm -f elf32 %s -o %s && gcc -m32 %s -o %s", output_file, nasm_output_file, nasm_output_file, output_file);
        }

        printf("%s", nasm_cmd);
        int res = system(nasm_cmd);
        if (res < 0)
        {
            printf("Issue assemblign the assembly file with NASM and linking with gcc");
            return res;
        }

    }
    return 0;
}