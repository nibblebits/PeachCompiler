#ifndef STDDEF_H
#define STDDEF_H

#include "stddef-internal.h"

#define offsetof(TYPE, MEMBER) &((TYPE*)0x00)->MEMBER
#endif