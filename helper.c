#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>
size_t variable_size(struct node *var_node)
{
    assert(var_node->type == NODE_TYPE_VARIABLE);
    return datatype_size(&var_node->var.type);
}

struct datatype* datatype_thats_a_pointer(struct datatype* d1, struct datatype* d2)
{
    if (d1->flags & DATATYPE_FLAG_IS_POINTER)
    {
        return d1;
    }

    if (d2->flags & DATATYPE_FLAG_IS_POINTER)
    {
        return d2;
    }

    return NULL;
}

bool is_logical_operator(const char* op)
{
    return S_EQ(op, "&&") || S_EQ(op, "||");
}

bool is_logical_node(struct node* node)
{
    return node->type == NODE_TYPE_EXPRESSION && is_logical_operator(node->exp.op);
}

struct datatype* datatype_pointer_reduce(struct datatype* datatype, int by)
{
    struct datatype* new_datatype = calloc(1, sizeof(struct datatype));
    memcpy(new_datatype, datatype, sizeof(struct datatype));
    new_datatype->pointer_depth -= by;
    if (new_datatype->pointer_depth <= 0)
    {
        new_datatype->flags &= ~DATATYPE_FLAG_IS_POINTER;
        new_datatype->pointer_depth = 0;
    }
    return new_datatype;
}


size_t variable_size_for_list(struct node *var_list_node)
{
    assert(var_list_node->type == NODE_TYPE_VARIABLE_LIST);
    size_t size = 0;
    vector_set_peek_pointer(var_list_node->var_list.list, 0);
    struct node *var_node = vector_peek_ptr(var_list_node->var_list.list);
    while (var_node)
    {
        size += variable_size(var_node);
        var_node = vector_peek_ptr(var_list_node->var_list.list);
    }

    return size;
}

struct node *variable_struct_or_union_body_node(struct node *node)
{
    if (!node_is_struct_or_union_variable(node))
    {
        return NULL;
    }

    if (node->var.type.type == DATA_TYPE_STRUCT)
    {
        return node->var.type.struct_node->_struct.body_n;
    }

    // return the union body.
    if (node->var.type.type == DATA_TYPE_UNION)
    {
        return node->var.type.union_node->_union.body_n;
    }
    return NULL;
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

int compute_sum_padding(struct vector *vec)
{
    int padding = 0;
    int last_type = -1;
    bool mixed_types = false;
    vector_set_peek_pointer(vec, 0);
    struct node *cur_node = vector_peek_ptr(vec);
    struct node *last_node = NULL;
    while (cur_node)
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

int array_multiplier(struct datatype *dtype, int index, int index_value)
{
    if (!(dtype->flags & DATATYPE_FLAG_IS_ARRAY))
    {
        return index_value;
    }

    vector_set_peek_pointer(dtype->array.brackets->n_brackets, index + 1);
    int size_sum = index_value;
    struct node *bracket_node = vector_peek_ptr(dtype->array.brackets->n_brackets);
    while (bracket_node)
    {
        assert(bracket_node->bracket.inner->type == NODE_TYPE_NUMBER);
        int declared_index = bracket_node->bracket.inner->llnum;
        int size_value = declared_index;
        size_sum *= size_value;
        bracket_node = vector_peek_ptr(dtype->array.brackets->n_brackets);
    }

    return size_sum;
}

int array_offset(struct datatype *dtype, int index, int index_value)
{
    if (!(dtype->flags & DATATYPE_FLAG_IS_ARRAY) ||
        (index == vector_count(dtype->array.brackets->n_brackets) - 1))
    {
        return index_value * datatype_element_size(dtype);
    }

    return array_multiplier(dtype, index, index_value) * datatype_element_size(dtype);
}


struct node* body_largest_variable_node(struct node* body_node)
{
    if (!body_node)
    {
        return NULL;
    }

    if(body_node->type != NODE_TYPE_BODY)
    {
        return NULL;
    }

    return body_node->body.largest_var_node;
}

struct node* variable_struct_or_union_largest_variable_node(struct node* var_node)
{
    return body_largest_variable_node(variable_struct_or_union_body_node(var_node));
}

int struct_offset(struct compile_process* compile_proc, const char* struct_name, const char* var_name, struct node** var_node_out, int last_pos, int flags)
{
    struct symbol* struct_sym = symresolver_get_symbol(compile_proc, struct_name);
    assert(struct_sym && struct_sym->type == SYMBOL_TYPE_NODE);
    struct node* node = struct_sym->data;
    assert(node_is_struct_or_union(node));

    struct vector* struct_vars_vec = node->_struct.body_n->body.statements;
    vector_set_peek_pointer(struct_vars_vec, 0);
    if (flags & STRUCT_ACCESS_BACKWARDS)
    {
        vector_set_peek_pointer_end(struct_vars_vec);
        vector_set_flag(struct_vars_vec, VECTOR_FLAG_PEEK_DECREMENT);
    }

    struct node* var_node_cur = variable_node(vector_peek_ptr(struct_vars_vec));
    struct node* var_node_last = NULL;
    int position = last_pos;
    *var_node_out = NULL;
    while(var_node_cur)
    {
        *var_node_out = var_node_cur;
        if (var_node_last)
        {
            position += variable_size(var_node_last);
            if (variable_node_is_primitive(var_node_cur))
            {
                position = align_value_treat_positive(position, var_node_cur->var.type.size);
            }
            else
            {
                position = align_value_treat_positive(position, variable_struct_or_union_largest_variable_node(var_node_cur)->var.type.size);
            }
        }

        // Have we found the variable? then we are done
        if (S_EQ(var_node_cur->var.name, var_name))
        {
            break;
        }

        var_node_last = var_node_cur;
        var_node_cur = variable_node(vector_peek_ptr(struct_vars_vec));
    }

    vector_unset_flag(struct_vars_vec, VECTOR_FLAG_PEEK_DECREMENT);
    return position;
}

bool is_access_operator(const char* op)
{
    return S_EQ(op, "->") || S_EQ(op, ".");
}
bool is_access_node(struct node* node)
{
    return node->type == NODE_TYPE_EXPRESSION && is_access_operator(node->exp.op);
}

bool is_access_node_with_op(struct node* node, const char* op)
{
    return is_access_node(node) && S_EQ(node->exp.op, op);
}

bool is_array_operator(const char* op)
{
    return S_EQ(op, "[]");
}

bool is_array_node(struct node* node)
{
    return node->type == NODE_TYPE_EXPRESSION && is_array_operator(node->exp.op);
}

bool is_parentheses_operator(const char* op)
{
    return S_EQ(op, "()");
}

bool is_parentheses_node(struct node* node)
{
    return node->type == NODE_TYPE_EXPRESSION && is_parentheses_operator(node->exp.op);
}

bool is_argument_operator(const char* op)
{
    return S_EQ(op, ",");
}

bool is_argument_node(struct node* node)
{
    return node->type == NODE_TYPE_EXPRESSION && is_argument_operator(node->exp.op);
}

bool is_unary_operator(const char* op)
{
    return S_EQ(op, "-") || S_EQ(op, "!") || S_EQ(op, "~") || S_EQ(op, "*") || S_EQ(op, "&") || S_EQ(op, "++") || S_EQ(op, "--");
}

bool op_is_indirection(const char* op)
{
    return S_EQ(op, "*");
}

bool op_is_address(const char* op)
{
    return S_EQ(op, "&");
}

bool file_exists(const char* filename)
{
    FILE* f = fopen(filename, "r");
    if (!f)
    {
        return false;
    }

    fclose(f);
    return true;
}

struct datatype datatype_for_numeric()
{
    struct datatype dtype = {};
    dtype.flags |= DATATYPE_FLAG_IS_LITERAL;
    dtype.type = DATA_TYPE_INTEGER;
    dtype.type_str = "int";
    dtype.size = DATA_SIZE_DWORD;
    return dtype;
}

struct datatype datatype_for_string()
{
    struct datatype dtype = {};
    dtype.type = DATA_TYPE_INTEGER;
    dtype.type_str = "char";
    dtype.flags |= DATATYPE_FLAG_IS_POINTER | DATATYPE_FLAG_IS_LITERAL;
    dtype.pointer_depth = 1;
    dtype.size = DATA_SIZE_DWORD;
    return dtype;
}

bool is_parentheses(const char* op)
{
    return (S_EQ(op, "("));
}

bool is_left_operanded_unary_operator(const char* op)
{
    return S_EQ(op, "++") || S_EQ(op, "--");
}

bool unary_operand_compatible(struct token* token)
{
    return is_access_operator(token->sval) ||
            is_array_operator(token->sval) ||
            is_parentheses(token->sval);
}
void datatype_decrement_pointer(struct datatype* dtype)
{
    dtype->pointer_depth--;
    if (dtype->pointer_depth <= 0)
    {
        dtype->flags &= ~DATATYPE_FLAG_IS_POINTER;
    }

}

long arithmetic(struct compile_process* compiler, long left_operand, long right_operand, const char* op, bool* success)
{
    *success = true;
    int result = 0;
    if (S_EQ(op, "*"))
    {
        result = left_operand * right_operand;
    }
    else if(S_EQ(op, "/"))
    {
        result = left_operand / right_operand;
    }
    else if(S_EQ(op, "+"))
    {
        result = left_operand + right_operand;
    }
    else if(S_EQ(op, "-"))
    {
        result = left_operand - right_operand;
    }
    else if(S_EQ(op, "=="))
    {
        result = left_operand == right_operand;
    }
    else if(S_EQ(op, "!="))
    {
        result = left_operand != right_operand;
    }
    else if(S_EQ(op, ">"))
    {
        result = left_operand > right_operand;
    }
    else if(S_EQ(op, "<"))
    {
        result = left_operand < right_operand;
    }
    else if(S_EQ(op, ">="))
    {
        result = left_operand >= right_operand;
    }
    else if(S_EQ(op, "<="))
    {
        result = left_operand <= right_operand;
    }
    else if(S_EQ(op, "<<"))
    {
        result = left_operand << right_operand;
    }
    else if(S_EQ(op, ">>"))
    {
        result = left_operand >> right_operand;
    }
    else if(S_EQ(op, "&&"))
    {
        result = left_operand && right_operand;
    }
    else if(S_EQ(op, "||"))
    {
        result = left_operand || right_operand;
    }
    else
    {
        *success = false;
    }

    return result;
}