#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

struct array_brackets* array_brackets_new()
{
    struct array_brackets* brackets = calloc(1, sizeof(struct array_brackets));
    brackets->n_brackets = vector_create(sizeof(struct node*));
    return brackets;
}

void array_brackets_free(struct array_brackets* brackets)
{
    free(brackets);
}

void array_brackets_add(struct array_brackets* brackets, struct node* bracket_node)
{
    assert(bracket_node->type == NODE_TYPE_BRACKET);
    vector_push(brackets->n_brackets, &bracket_node);
}

struct vector* array_brackets_node_vector(struct array_brackets* brackets)
{
    return brackets->n_brackets;
}

size_t array_brackets_calculate_size_from_index(struct datatype* dtype, struct array_brackets* brackets, int index)
{
    #warning "Not yet implemented calculating size of array brackets"
    return 0;
}

size_t array_brackets_calculate_size(struct datatype* dtype, struct array_brackets* brackets)
{
    return array_brackets_calculate_size_from_index(dtype, brackets, 0);
}

int array_total_indexes(struct datatype* dtype)
{
    assert(dtype->flags & DATATYPE_FLAG_IS_ARRAY);
    struct array_brackets* brackets = dtype->array.brackets;
    return vector_count(brackets->n_brackets);
}
