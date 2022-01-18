#include "compiler.h"

bool datatype_is_struct_or_union_for_name(const char* name)
{
    return S_EQ(name,"union") || S_EQ(name, "struct");
}