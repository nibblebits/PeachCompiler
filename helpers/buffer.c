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

#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

struct buffer* buffer_create()
{
    struct buffer* buf = calloc(sizeof(struct buffer), 1);
    buf->data = calloc(BUFFER_REALLOC_AMOUNT, 1);
    buf->len = 0;
    buf->msize = BUFFER_REALLOC_AMOUNT;
    return buf;
}

void buffer_extend(struct buffer* buffer, size_t size)
{
    buffer->data = realloc(buffer->data, buffer->msize+size);
    buffer->msize+=size;
}

void buffer_need(struct buffer* buffer, size_t size)
{
    if (buffer->msize <= (buffer->len+size))
    {
        size += BUFFER_REALLOC_AMOUNT;
        buffer_extend(buffer, size);
    }
}


void buffer_printf(struct buffer* buffer, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int index = buffer->len;
    // Temporary, this is a limitation we are guessing the size is no more than 2048
    int len = 2048;
    buffer_extend(buffer, len);
    int actual_len = vsnprintf(&buffer->data[index], len, fmt, args);
    buffer->len += actual_len;
    va_end(args);
}

void buffer_printf_no_terminator(struct buffer* buffer, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int index = buffer->len;
    // Temporary, this is a limitation we are guessing the size is no more than 2048
    int len = 2048;
    buffer_extend(buffer, len);
    int actual_len = vsnprintf(&buffer->data[index], len, fmt, args);
    buffer->len += actual_len-1;
    va_end(args);
}

void buffer_write(struct buffer* buffer, char c)
{
    buffer_need(buffer, sizeof(char));

    buffer->data[buffer->len] = c;
    buffer->len++;
}

void* buffer_ptr(struct buffer* buffer)
{
    return buffer->data;
}

char buffer_read(struct buffer* buffer)
{
    if (buffer->rindex >= buffer->len)
    {
        return -1;
    }
    char c = buffer->data[buffer->rindex];
    buffer->rindex++;
    return c;
}

char buffer_peek(struct buffer* buffer)
{
    if (buffer->rindex >= buffer->len)
    {
        return -1;
    }
    char c = buffer->data[buffer->rindex];
    return c;
}

void buffer_free(struct buffer* buffer)
{
    free(buffer->data);
    free(buffer);
}