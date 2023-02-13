#include "compiler.h"
#include "helpers/vector.h"

// va_start(list, final_argument_in_function)
void native_va_start(struct generator* generator, struct native_function* func, struct vector* arguments)
{
    struct compile_process* compiler = generator->compiler;
    if (vector_count(arguments) != 2)
    {
        compiler_error(compiler, "va_start expects two arguments %i was provided", vector_count(arguments));
    }

    struct node* list_arg = vector_peek_ptr(arguments);
    struct node* stack_arg = vector_peek_ptr(arguments);
    if (stack_arg->type != NODE_TYPE_IDENTIFIER)
    {
        compiler_error(compiler, "Expecting a valid stack argument for va_start");
    }
    generator->asm_push("; va_start on variable %s", stack_arg->sval);
    vector_set_peek_pointer(arguments, 0);
    generator->gen_exp(generator, stack_arg, EXPRESSION_GET_ADDRESS);
    
    struct resolver_result* result = resolver_follow(compiler->resolver, list_arg);
    assert(resolver_result_ok(result));
    struct resolver_entity* list_arg_entity = resolver_result_entity_root(result);
    struct generator_entity_address address_out;
    generator->entity_address(generator, list_arg_entity, &address_out);
    generator->asm_push("mov dword [%s], ebx", address_out.address);
    generator->asm_push("; va_start end for variable %s", stack_arg->sval);

    struct datatype void_datatype;
    datatype_set_void(&void_datatype);
    generator->ret(&void_datatype, "0");
}
void preprocessor_stdarg_internal_include(struct preprocessor* preprocessor, struct preprocessor_included_file* file)
{
    #warning "Create VALIST"
    native_create_function(preprocessor->compiler, "va_start", 
        &(struct native_function_callbacks){.call=native_va_start});
}