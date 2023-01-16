#include "compiler.h"


void preprocessor_stddef_include(struct preprocessor* preprocessor, struct preprocessor_included_file* file);
void preprocessor_stdarg_internal_include(struct preprocessor* preprocessor, struct preprocessor_included_file* file);

PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION preprocessor_static_include_handler_for(const char* filename)
{
    if (S_EQ(filename, "stddef-internal.h"))
    {
        return preprocessor_stddef_include;
    }
    else if(S_EQ(filename, "stdarg-internal.h"))
    {
        return preprocessor_stdarg_internal_include;
    }

    return NULL;
}