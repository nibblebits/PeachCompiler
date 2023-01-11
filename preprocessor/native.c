#include "compiler.h"

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