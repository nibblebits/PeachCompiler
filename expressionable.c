#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

/**
 * Format: {operator1, operator2, operator3, NULL}
 */

struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS] = {
    {.operators = {"++", "--", "()", "[]", "(", "[", ".", "->", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"*", "/", "%", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"+", "-", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"<<", ">>", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"<", "<=", ">", ">=", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"==", "!=", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"&", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"^", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"|", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"&&", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"||", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators = {"?", ":", NULL}, .associtivity = ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators = {"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", NULL}, .associtivity = ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators = {",", NULL}, .associtivity = ASSOCIATIVITY_LEFT_TO_RIGHT}};

void expressionable_ignore_nl(struct expressionable *expressionable, struct token *next_token);
void expressionable_parse(struct expressionable *expressionable);

void expressionable_error(struct expressionable *expressionable, const char *str)
{
    assert(0 == 1 && str);
}

int expressionable_parse_single(struct expressionable *expressionable);

struct expressionable_callbacks *expressionable_callbacks(struct expressionable *expressionable)
{
    return &expressionable->config.callbacks;
}
void *expressionable_node_pop(struct expressionable *expressionable)
{
    void *last_node = vector_back_ptr(expressionable->node_vec_out);
    vector_pop(expressionable->node_vec_out);
    return last_node;
}

struct token *expressionable_token_next(struct expressionable *expressionable)
{
    struct token *next_token = vector_peek_no_increment(expressionable->token_vec);
    expressionable_ignore_nl(expressionable, next_token);
    return vector_peek(expressionable->token_vec);
}

void expressionable_node_push(struct expressionable *expressionable, void *node_ptr)
{
    vector_push(expressionable->node_vec_out, &node_ptr);
}

void *expressionable_node_peek_or_null(struct expressionable *expressionable)
{
    return vector_back_ptr_or_null(expressionable->node_vec_out);
}

void expressionable_ignore_nl(struct expressionable *expressionable, struct token *next_token)
{
    while (next_token && token_is_symbol(next_token, '\\'))
    {
        // Skip the "\"
        vector_peek(expressionable->token_vec);
        // Skip the new line token
        struct token *nl_token = vector_peek(expressionable->token_vec);
        assert(nl_token->type == TOKEN_TYPE_NEWLINE);
        next_token = vector_peek_no_increment(expressionable->token_vec);
    }
}
struct token *expressionable_peek_next(struct expressionable *expressionable)
{
    struct token *next_token = vector_peek_no_increment(expressionable->token_vec);
    expressionable_ignore_nl(expressionable, next_token);
    return vector_peek_no_increment(expressionable->token_vec);
}

bool expressionable_token_next_is_operator(struct expressionable *expressionable, const char *op)
{
    struct token *token = expressionable_peek_next(expressionable);
    return token_is_operator(token, op);
}

void expressionable_init(struct expressionable* expressionable, struct vector* token_vector, struct vector* node_vector, struct expressionable_config* config, int flags)
{
    memset(expressionable, 0, sizeof(struct expressionable));
    memcpy(&expressionable->config, config, sizeof(struct expressionable_config));

    expressionable->token_vec = token_vector;
    expressionable->node_vec_out = node_vector;
    expressionable->flags = flags;
}

struct expressionable* expressionable_create(struct expressionable_config* config, struct vector* token_vector, struct vector* node_vector, int flags)
{
    assert(vector_element_size(token_vector) == sizeof(struct token));
    struct expressionable* expressionable = calloc(1, sizeof(struct expressionable));
    expressionable_init(expressionable, token_vector, node_vector, config, flags);
    return expressionable;
} 
int expressionable_parse_number(struct expressionable *expressionable)
{
    void *node_ptr = expressionable_callbacks(expressionable)->handle_number_callback(expressionable);
    if (!node_ptr)
    {
        return -1;
    }

    expressionable_node_push(expressionable, node_ptr);
    return 0;
}
int expressionable_parse_identifier(struct expressionable *expressionable)
{
    void *node_ptr = expressionable_callbacks(expressionable)->handle_identifier_callback(expressionable);
    if (!node_ptr)
    {
        return -1;
    }

    expressionable_node_push(expressionable, node_ptr);
    return 0;
}

int expressionable_parser_get_precedence_for_operator(const char *op, struct expressionable_op_precedence_group **group_out)
{
    *group_out = NULL;
    for (int i = 0; i < TOTAL_OPERATOR_GROUPS; i++)
    {
        for (int b = 0; op_precedence[i].operators[b]; b++)
        {
            const char *_op = op_precedence[i].operators[b];
            if (S_EQ(op, _op))
            {
                *group_out = &op_precedence[i];
                return i;
            }
        }
    }

    return -1;
}

bool expressionable_parser_left_op_has_priority(const char *op_left, const char *op_right)
{
    struct expressionable_op_precedence_group *group_left = NULL;
    struct expressionable_op_precedence_group *group_right = NULL;

    if (S_EQ(op_left, op_right))
    {
        return false;
    }

    int precedence_left = expressionable_parser_get_precedence_for_operator(op_left, &group_left);
    int precedence_right = expressionable_parser_get_precedence_for_operator(op_right, &group_right);
    if (group_left->associtivity == ASSOCIATIVITY_RIGHT_TO_LEFT)
    {
        return false;
    }

    return precedence_left <= precedence_right;
}

void expressionable_parser_node_shift_children_left(struct expressionable *expressionable, void *node)
{
    int node_type = expressionable_callbacks(expressionable)->get_node_type(expressionable, node);
    assert(node_type == EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION);

    void *left_node = expressionable_callbacks(expressionable)->get_left_node(expressionable, node);
    void *right_node = expressionable_callbacks(expressionable)->get_right_node(expressionable, node);
    int right_node_type = expressionable_callbacks(expressionable)->get_node_type(expressionable, right_node);
    assert(right_node_type == EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION);

    const char *right_op = expressionable_callbacks(expressionable)->get_node_operator(expressionable, right_node);
    void *new_exp_left_node = left_node;
    void *new_exp_right_node = expressionable_callbacks(expressionable)->get_left_node(expressionable, right_node);
    const char *node_op = expressionable_callbacks(expressionable)->get_node_operator(expressionable, node);
    expressionable_callbacks(expressionable)->make_expression_node(expressionable, new_exp_left_node, new_exp_right_node, node_op);

    void *new_left_operand = expressionable_node_pop(expressionable);
    void *new_right_operand = expressionable_callbacks(expressionable)->get_right_node(expressionable, right_node);
    expressionable_callbacks(expressionable)->set_exp_node(expressionable, node, new_left_operand, new_right_operand, right_op);
}

void expressionable_parser_reorder_expression(struct expressionable *expressionable, void **node_out)
{
    void *node = *node_out;
    int node_type = expressionable_callbacks(expressionable)->get_node_type(expressionable, node);

    if (node_type != EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION)
    {
        return;
    }

    void *left_node = expressionable_callbacks(expressionable)->get_left_node(expressionable, node);
    void *right_node = expressionable_callbacks(expressionable)->get_right_node(expressionable, node);

    int left_node_type = expressionable_callbacks(expressionable)->get_node_type(expressionable, left_node);
    int right_node_type = expressionable_callbacks(expressionable)->get_node_type(expressionable, right_node);
    assert(left_node_type >= 0);
    assert(right_node_type >= 0);

    if (left_node_type != EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION && right_node &&
        right_node_type != EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION)
    {
        return;
    }

    if (left_node_type != EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION && right_node &&
        right_node_type == EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION)
    {
        const char *right_op = expressionable_callbacks(expressionable)->get_node_operator(expressionable, right_node);
        const char *main_op = expressionable_callbacks(expressionable)->get_node_operator(expressionable, node);

        if (expressionable_parser_left_op_has_priority(main_op, right_op))
        {
            expressionable_parser_node_shift_children_left(expressionable, node);

            void **address_of_left = expressionable_callbacks(expressionable)->get_left_node_address(expressionable, node);
            void **address_of_right = expressionable_callbacks(expressionable)->get_right_node_address(expressionable, node);
            expressionable_parser_reorder_expression(expressionable, address_of_left);
            expressionable_parser_reorder_expression(expressionable, address_of_right);
        }
    }
}

bool expressionable_generic_type_is_value_expressionable(int type)
{
    return type == EXPRESSIONABLE_GENERIC_TYPE_NUMBER ||
           type == EXPRESSIONABLE_GENERIC_TYPE_IDENTIFIER ||
           type == EXPRESSIONABLE_GENERIC_TYPE_UNARY ||
           type == EXPRESSIONABLE_GENERIC_TYPE_PARENTHESES ||
           type == EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION;
}

void expressionable_expect_op(struct expressionable *expressionable, const char *op)
{
    struct token *next_token = expressionable_token_next(expressionable);
    if (next_token == NULL || !token_is_operator(next_token, op))
    {
        expressionable_error(expressionable, "Expecting a certain operator but something else was provided\n");
    }
}

void expressionable_expect_sym(struct expressionable *expressionable, char c)
{
    struct token *next_token = expressionable_token_next(expressionable);
    if (next_token && !token_is_symbol(next_token, c))
    {
        expressionable_error(expressionable, "Expecting a different symbol than was provided");
    }
}

void expressionable_deal_with_additional_expression(struct expressionable *expressionable)
{
    if (is_operator_token(expressionable_peek_next(expressionable)))
    {
        expressionable_parse(expressionable);
    }
}
void expressionable_parse_parentheses(struct expressionable *expressionable)
{
    void *left_node = NULL;
    void *tmp_node = expressionable_node_peek_or_null(expressionable);
    int tmp_type = tmp_node ? expressionable_callbacks(expressionable)->get_node_type(expressionable, tmp_node) : -1;
    if (tmp_node && expressionable_generic_type_is_value_expressionable(tmp_type))
    {
        left_node = tmp_node;
        expressionable_node_pop(expressionable);
    }

    expressionable_expect_op(expressionable, "(");
    expressionable_parse(expressionable);
    expressionable_expect_sym(expressionable, ')');
    void *exp_node = expressionable_node_pop(expressionable);
    expressionable_callbacks(expressionable)->make_parentheses_node(expressionable, exp_node);
    if (left_node)
    {
        void *parentheses_node = expressionable_node_pop(expressionable);
        expressionable_callbacks(expressionable)->make_expression_node(expressionable, left_node, parentheses_node, "()");
    }
    expressionable_deal_with_additional_expression(expressionable);
}

int expressionable_get_pointer_depth(struct expressionable *expressionable)
{
    int depth = 0;
    while (expressionable_token_next_is_operator(expressionable, "*"))
    {
        depth += 1;
        expressionable_token_next(expressionable);
    }

    return depth;
}

void expressionable_parse_for_indirection_unary(struct expressionable *expressionable)
{
    int depth = expressionable_get_pointer_depth(expressionable);
    expressionable_parse(expressionable);

    void *unary_operand_node = expressionable_node_pop(expressionable);
    expressionable_callbacks(expressionable)->make_unary_indirection_node(expressionable, depth, unary_operand_node);
}
void expressionable_parse_for_normal_unary(struct expressionable *expressionable)
{
    const char *unary_op = expressionable_token_next(expressionable)->sval;
    expressionable_parse(expressionable);

    void *unary_operand_node = expressionable_node_pop(expressionable);
    expressionable_callbacks(expressionable)->make_unary_node(expressionable, unary_op, unary_operand_node);
}

void expressionable_parse_unary(struct expressionable *expressionable)
{
    const char *unary_op = expressionable_peek_next(expressionable)->sval;
    if (op_is_indirection(unary_op))
    {
        expressionable_parse_for_indirection_unary(expressionable);
        return;
    }

    expressionable_parse_for_normal_unary(expressionable);

    expressionable_deal_with_additional_expression(expressionable);
}
void expressionable_parse_for_operator(struct expressionable *expressionable)
{
    struct token *op_token = expressionable_peek_next(expressionable);
    const char *op = op_token->sval;
    void *node_left = expressionable_node_peek_or_null(expressionable);
    if (!node_left)
    {

        if (!is_unary_operator(op))
        {
            expressionable_error(expressionable, "Not a unary operator..");
        }

        expressionable_parse_unary(expressionable);
        return;
    }

    // Pop the operator token
    expressionable_token_next(expressionable);

    // Pop the left node
    expressionable_node_pop(expressionable);

    if (expressionable_peek_next(expressionable)->type == TOKEN_TYPE_OPERATOR)
    {
        if (S_EQ(expressionable_peek_next(expressionable)->sval, "("))
        {
            expressionable_parse_parentheses(expressionable);
        }
        else if (is_unary_operator(expressionable_peek_next(expressionable)->sval))
        {
             expressionable_parse_unary(expressionable);
        }
        else
        {
            expressionable_error(expressionable, "Two operators are not expected for the given expression");
        }
    }
    else
    {
        // Parse the right operand.
        expressionable_parse(expressionable);
    }

    void *node_right = expressionable_node_pop(expressionable);
    expressionable_callbacks(expressionable)->make_expression_node(expressionable, node_left, node_right, op);
    void *exp_node = expressionable_node_pop(expressionable);
    expressionable_parser_reorder_expression(expressionable, &exp_node);
    expressionable_node_push(expressionable, exp_node);
}

void expressionable_parse_tenary(struct expressionable* expressionable)
{
    void* condition_operand = expressionable_node_pop(expressionable);
    expressionable_expect_op(expressionable, "?");

    // Parse the TRUE part of the tenary
    expressionable_parse(expressionable);
    void* true_result_node = expressionable_node_pop(expressionable);
    expressionable_expect_sym(expressionable, ':');

    // Parse the FALSE result
    expressionable_parse(expressionable);
    void* false_result_node = expressionable_node_pop(expressionable);

    expressionable_callbacks(expressionable)->make_tenary_node(expressionable, true_result_node, false_result_node);

    void* tenary_node = expressionable_node_pop(expressionable);
    expressionable_callbacks(expressionable)->make_expression_node(expressionable, condition_operand, tenary_node, "?");
}

int expressionable_parse_exp(struct expressionable *expressionable, struct token *token)
{
    if (S_EQ(expressionable_peek_next(expressionable)->sval, "("))
    {
        expressionable_parse_parentheses(expressionable);
    }
    else if(S_EQ(expressionable_peek_next(expressionable)->sval, "?"))
    {
        expressionable_parse_tenary(expressionable);
    }
    else
    {
        expressionable_parse_for_operator(expressionable);
    }

    return 0;
}

int expressionable_parse_token(struct expressionable *expressionable, struct token *token, int flags)
{
    int res = -1;
    switch (token->type)
    {
    // Parse the tokens..
    case TOKEN_TYPE_NUMBER:
        expressionable_parse_number(expressionable);
        res = 0;
        break;

    case TOKEN_TYPE_IDENTIFIER:
        expressionable_parse_identifier(expressionable);
        res = 0;
        break;

    case TOKEN_TYPE_OPERATOR:
        expressionable_parse_exp(expressionable, token);
        break;
    }

    return res;
}
int expressionable_parse_single_with_flags(struct expressionable *expressionable, int flags)
{
    int res = -1;
    struct token *token = expressionable_peek_next(expressionable);
    if (!token)
    {
        return -1;
    }

    if (expressionable_callbacks(expressionable)->is_custom_operator(expressionable, token))
    {
        token->flags |= TOKEN_FLAG_IS_CUSTOM_OPERATOR;
        expressionable_parse_exp(expressionable, token);
    }
    else
    {
        res = expressionable_parse_token(expressionable, token, flags);
    }

    void *node = expressionable_node_pop(expressionable);

    if (expressionable_callbacks(expressionable)->expecting_additional_node(expressionable, node))
    {
        expressionable_parse_single(expressionable);
        void *additional_node = expressionable_node_peek_or_null(expressionable);
        if (expressionable_callbacks(expressionable)->should_join_nodes(expressionable, node, additional_node))
        {
            void *new_node = expressionable_callbacks(expressionable)->join_nodes(expressionable, node, additional_node);
            expressionable_node_pop(expressionable);
            node = new_node;
        }
    }
    expressionable_node_push(expressionable, node);
    return res;
}
int expressionable_parse_single(struct expressionable *expressionable)
{
    return expressionable_parse_single_with_flags(expressionable, 0);
}
void expressionable_parse(struct expressionable *expressionable)
{
    while (expressionable_parse_single(expressionable) == 0)
    {
    }
}