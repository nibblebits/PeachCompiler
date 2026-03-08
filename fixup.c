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

struct fixup_system* fixup_sys_new()
{
    struct fixup_system* system = calloc(1, sizeof(struct fixup_system));
    system->fixups = vector_create(sizeof(struct fixup));
    return system;
}

struct fixup_config* fixup_config(struct fixup* fixup)
{
    return &fixup->config;
}

void fixup_free(struct fixup* fixup)
{
    fixup->config.end(fixup);
    free(fixup);
}

void fixup_start_iteration(struct fixup_system* system)
{
    vector_set_peek_pointer(system->fixups, 0);
}

struct fixup* fixup_next(struct fixup_system* system)
{
    return vector_peek_ptr(system->fixups);
}

void fixup_sys_fixups_free(struct fixup_system* system)
{
    fixup_start_iteration(system);
    struct fixup* fixup = fixup_next(system);
    while(fixup)
    {
        fixup_free(fixup);
        fixup = fixup_next(system);
    }
}

void fixup_sys_free(struct fixup_system* system)
{
    fixup_sys_fixups_free(system);
    vector_free(system->fixups);
    free(system);
}

int fixup_sys_unresolved_fixups_count(struct fixup_system* system)
{
    size_t c = 0;
    fixup_start_iteration(system);
    struct fixup* fixup = fixup_next(system);
    while(fixup)
    {
        if (fixup->flags & FIXUP_FLAG_RESOLVED)
        {
            fixup = fixup_next(system);
            continue;
        }
        c++;
        fixup = fixup_next(system);
    }

    return c;
}

struct fixup* fixup_register(struct fixup_system* system, struct fixup_config* config)
{
    struct fixup* fixup = calloc(1, sizeof(struct fixup));
    memcpy(&fixup->config, config, sizeof(struct fixup_config));
    fixup->system = system;
    vector_push(system->fixups, fixup);
    return fixup;
}

bool fixup_resolve(struct fixup* fixup)
{
    if (fixup_config(fixup)->fix(fixup))
    {
        fixup->flags |= FIXUP_FLAG_RESOLVED;
        return true;
    }

    return false;
}

void* fixup_private(struct fixup* fixup)
{
    return fixup_config(fixup)->private;
}

bool fixups_resolve(struct fixup_system* system)
{
    fixup_start_iteration(system);
    struct fixup* fixup = fixup_next(system);
    while(fixup)
    {
        if (fixup->flags & FIXUP_FLAG_RESOLVED)
        {
            continue;
        }
        fixup_resolve(fixup);
        fixup = fixup_next(system);
    }

    return fixup_sys_unresolved_fixups_count(system) == 0;
}