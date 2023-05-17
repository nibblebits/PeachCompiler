#include "compiler.h"
#include "helpers/vector.h"

static struct compile_process* validator_current_compile_process;
static struct node* current_function;

void validation_new_scope(int flags)
{
    resolver_default_new_scope(validator_current_compile_process->resolver, flags);
}

void validation_end_scope()
{
    resolver_default_finish_scope(validator_current_compile_process->resolver);
}

struct node* validation_next_tree_node()
{
    return vector_peek_ptr(validator_current_compile_process->node_tree_vec);
}

void validate_initialize(struct compile_process* process)
{
    validator_current_compile_process = process;
    vector_set_peek_pointer(process->node_tree_vec, 0);
    symresolver_new_table(process);
}

void validate_destruct(struct compile_process* process)
{
    symresolver_end_table(process);
    vector_set_peek_pointer(process->node_tree_vec, 0);
}

int validate_tree()
{
    return VALIDATION_ALL_OK;
}

int validate(struct compile_process* process)
{
    int res = 0;
    validate_initialize(process);
    res = validate_tree(process);
    validate_destruct(process);
    return res;
}