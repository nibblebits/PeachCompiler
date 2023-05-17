#include "compiler.h"

void validate_initialize(struct compile_process* process)
{
    // todo
}

void validate_destruct(struct compile_process* process)
{
    // todo
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