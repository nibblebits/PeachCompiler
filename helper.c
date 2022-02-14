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

int padding(int val, int to)
{
    if (to <= 0)
    {
        return 0;
    }

    if ((val % to) == 0)
    {
        return 0;
    }

    return to - (val % to) % to;
}

int align_value(int val, int to)
{
    if (val % to)
    {
        val += padding(val, to);
    }
    return val;
}

int align_value_treat_positive(int val, int to)
{
    assert(to >= 0);
    if (val < 0)
    {
        to = -to;
    }
    return align_value(val, to);
}

int compute_sum_padding(struct vector* vec)
{
    int padding = 0;
    int last_type = -1;
    bool mixed_types = false;
    vector_set_peek_pointer(vec, 0);
    struct node* cur_node = vector_peek_ptr(vec);
    struct node* last_node = NULL;
    while(cur_node)
    {
        if (cur_node->type != NODE_TYPE_VARIABLE)
        {
            cur_node = vector_peek_ptr(vec);
            continue;
        }

        padding += cur_node->var.padding;
        last_type = cur_node->var.type.type;
        last_node = cur_node;
        cur_node = vector_peek_ptr(vec);
    }

    return padding;

}