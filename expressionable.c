#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

/**
 * Format: {operator1, operator2, operator3, NULL}
 */

struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS] = {
    {.operators={"++", "--", "()", "[]", "(", "[", ".", "->",  NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"*", "/", "%", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"+", "-", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"<<", ">>", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"<", "<=", ">", ">=", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"==", "!=", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"&", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"^", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"|", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"&&", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"||", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"?", ":", NULL}, .associtivity=ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators={"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", NULL}, .associtivity=ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators={",", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT}
};


void expressionable_error(struct expressionable* expressionable, const char* str)
{
    assert(0 == 1 && str);
}

int expressionable_parse_single(struct expressionable* expressionable);

struct expressionable_callbacks* expressionable_callbacks(struct expressionable* expressionable)
{
    return &expressionable->config.callbacks;
}
void* expressionable_node_pop(struct expressionable* expressionable)
{
    void* last_node = vector_back_ptr(expressionable->node_vec_out);
    vector_pop(expressionable->node_vec_out);
    return last_node;
}

struct token* expressionable_token_next(struct expressionable* expressionable)
{
    struct token* next_token = vector_peek_no_increment(expressionable->token_vec);
    expressionable_ignore_nl(expressionable, next_token);
    return vector_peek(expressionable->token_vec);
}

void expressionable_node_push(struct expressionable* expressionable, void* node_ptr)
{
    vector_push(expressionable->node_vec_out, &node_ptr);
}

void* expressionable_node_peek_or_null(struct expressionable* expressionable)
{
    return vector_back_ptr_or_null(expressionable->node_vec_out);
}

void expressionable_ignore_nl(struct expressionable* expressionable, struct token* next_token)
{
    while(next_token && token_is_symbol(next_token, '\\'))
    {
        // Skip the "\"
        vector_peek(expressionable->token_vec);
        // Skip the new line token
        struct token* nl_token = vector_peek(expressionable->token_vec);
        assert(nl_token->type == TOKEN_TYPE_NEWLINE);
        next_token = vector_peek_no_increment(expressionable->token_vec);
    }
}
struct token* expressionable_peek_next(struct expressionable* expressionable)
{
    struct token* next_token = vector_peek_no_increment(expressionable->token_vec);
    expressionable_ignore_nl(expressionable, next_token);
    return vector_peek_no_increment(expressionable->token_vec);
}

int expressionable_parse_number(struct expressionable* expressionable)
{
    void* node_ptr = expressionable_callbacks(expressionable)->handle_number_callback(expressionable);
    if (!node_ptr)
    {
        return -1;
    }

    expressionable_node_push(expressionable, node_ptr);
    return 0;
}
int expressionable_parse_identifier(struct expressionable* expressionable)
{
    void* node_ptr = expressionable_callbacks(expressionable)->handle_identifier_callback(expressionable);
    if (!node_ptr)
    {
        return -1;
    }

    expressionable_node_push(expressionable, node_ptr);
    return 0;
}

  b
void expressionable_parse_for_operator(struct expressionable* expressionable)
{
    struct token* op_token = expressionable_peek_next(expressionable);
    const char* op = op_token->sval;
    void* node_left = expressionable_node_peek_or_null(expressionable);
    if (!node_left)
    {
        #warning "deal with a unary"
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
            #warning "parse for parentheses
            //expressionable_parse_parentheses()
        }
        else if(is_unary_operator(expressionable_peek_next(expressionable)->sval))
        {
            #warning "parse the unary"
           // expressionable_parse_unary()
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

    void* node_right = expressionable_node_pop(expressionable);
    expressionable_callbacks(expressionable)->make_expression_node(expressionable, node_left, node_right, op);
    void* exp_node = expressionable_node_pop(expressionable);
    expressionable_parser_reorder_expression(expressionable, &exp_node);
    expressionable_node_push(expressionable, exp_node);
}   

int expressionable_parse_exp(struct expressionable* expressionable, struct token* token)
{
    #warning "parse parenteheses"
    #warning "parse tenary"


    expressionable_parse_for_operator(expressionable);
    return 0;
}

int expressionable_parse_token(struct expressionable* expressionable, struct token* token, int flags)
{
    int res = -1;
    switch(token->type)
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
int expressionable_parse_single_with_flags(struct expressionable* expressionable, int flags)
{
    int res = -1;
    struct token* token = expressionable_peek_next(expressionable);
    if (!token)
    {
        return -1;
    }

    if (expressionable_callbacks(expressionable)->is_custom_operator(expressionable, token))
    {
        token->flags |= TOKEN_FLAG_IS_CUSTOM_OPERATOR;
        #warning "Come back and implement parse_exp"
       // expressionable_parse_exp(expressionable, token);
    }
    else
    {
        res = expressionable_parse_token(expressionable, token, flags);
    }

    void* node = expressionable_node_pop(expressionable);

    if (expressionable_callbacks(expressionable)->expecting_additional_node(expressionable, node))
    {
        expressionable_parse_single(expressionable);
        void* additional_node = expressionable_node_peek_or_null(expressionable);
        if (expressionable_callbacks(expressionable)->should_join_nodes(expressionable, node, additional_node))
        {
            void* new_node = expressionable_callbacks(expressionable)->join_nodes(expressionable, node, additional_node);
            expressionable_node_pop(expressionable);
            node = new_node;
        }
    }
    expressionable_node_push(expressionable, node);
    return res;
}
int expressionable_parse_single(struct expressionable* expressionable)
{
    return expressionable_parse_single_with_flags(expressionable, 0);
}
void expressionable_parse(struct expressionable* expressionable)
{
    while(expressionable_parse_single(expressionable) == 0)
    {
    }
}