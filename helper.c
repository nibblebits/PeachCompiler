#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>
size_t variable_size(struct node* var_node)
{
    assert(var_node->type == NODE_TYPE_VARIABLE);
    return datatype_size(&var_node->var.type);
}

size_t variable_size_for_list(struct node* var_list_node)
{
    assert(var_list_node->type == NODE_TYPE_VARIABLE_LIST);
    size_t size = 0;
    vector_set_peek_pointer(var_list_node->var_list.list, 0);
    struct node* var_node = vector_peek_ptr(var_list_node->var_list.list);
    while(var_node)
    {
        size += variable_size(var_node);
        var_node = vector_peek_ptr(var_list_node->var_list.list);
    }

    return size;
}