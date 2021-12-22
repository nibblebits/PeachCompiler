#include "compiler.h"
#include "helpers/vector.h"
#include <stdlib.h>

struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* functions, void* private)
{
    struct lex_process* process = calloc(1, sizeof(struct lex_process));
    process->function = functions;
    process->token_vec = vector_create(sizeof(struct token));
    process->compiler = compiler;
    process->private = private;
    process->pos.line = 1;
    process->pos.col = 1;
    return process;
}

void lex_process_free(struct lex_process* process)
{
    vector_free(process->token_vec);
    free(process);
}

void* lex_process_private(struct lex_process* process)
{
    return process->private;
}

struct vector* lex_process_tokens(struct lex_process* process)
{
    return process->token_vec;
}