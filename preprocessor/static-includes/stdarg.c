#include "compiler.h"

void native_test_function(struct generator* generator, struct node* orinating_function, struct native_function* func, struct vector* arguments)
{
    generator->asm_push("; TEST FUNCTION ACTIVATED!");
}

void preprocessor_stdarg_internal_include(struct preprocessor* preprocessor, struct preprocessor_included_file* file)
{
    #warning "Create VALIST"
    native_create_function(preprocessor->compiler, "test", 
        &(struct native_function_callbacks){.call=native_test_function});
}