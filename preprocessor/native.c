#include "compiler.h"
#include <stdlib.h>

int preprocessor_line_macro_evaluate(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments)
{
    struct preprocessor* preprocessor = definition->preprocessor;
    struct compile_process* compiler = preprocessor->compiler;

    if (arguments)
    {
        compiler_error(compiler, "__LINE__ macro expects no arguments");
    }   

    struct token* previous_token = preprocessor_previous_token(compiler);
    return previous_token->pos.line;
}

struct vector* preprocessor_line_macro_value(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments)
{
    struct preprocessor* preprocessor = definition->preprocessor;
    struct compile_process* compiler = preprocessor->compiler;

    if (arguments)
    {
        compiler_error(compiler, "__LINE__ macro expects no arguments");
    }   
    struct token* previous_token = preprocessor_previous_token(compiler);
    return preprocessor_build_value_vector_for_integer(previous_token->pos.line);
}

void preprocessor_create_definitions(struct preprocessor* preprocessor)
{
    preprocessor_definition_create_native("__LINE__", preprocessor_line_macro_evaluate, preprocessor_line_macro_value, preprocessor);   
}

struct symbol* native_create_function(struct compile_process* compiler, const char* name,
 struct native_function_callbacks* callbacks)
{
    struct native_function* func = calloc(1, sizeof(struct native_function));
    memcpy(&func->callbacks, callbacks, sizeof(func->callbacks));
    func->name = name;
    return symresolver_register_symbol(compiler, name, SYMBOL_TYPE_NATIVE_FUNCTION, func);
}

struct native_function* native_function_get(struct compile_process* compiler, const char* name)
{
    struct symbol* sym = symresolver_get_symbol_for_native_function(compiler, name);
    if (!sym)
    {
        return NULL;
    }

    return sym->data;
}
