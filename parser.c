#include "compiler.h"
#include "helpers/vector.h"

static struct compile_process *current_process;
static struct token *parser_last_token;

struct history
{
    int flags;
};

struct history *history_begin(int flags)
{
    struct history *history = calloc(1, sizeof(struct history));
    history->flags = flags;
    return history;
}

struct history *history_down(struct history *history, int flags)
{
    struct history *new_history = calloc(1, sizeof(struct history));
    memcpy(new_history, history, sizeof(struct history));
    new_history->flags = flags;
    return new_history;
}

int parse_expressionable_single(struct history *history);
void parse_expressionable(struct history *history);


static void parser_ignore_nl_or_comment(struct token *token)
{
    while (token && token_is_nl_or_comment_or_newline_seperator(token))
    {
        // Skip the token
        vector_peek(current_process->token_vec);
        token = vector_peek_no_increment(current_process->token_vec);
    }
}

static struct token *token_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    current_process->pos = next_token->pos;
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static struct token *token_peek_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    return vector_peek_no_increment(current_process->token_vec);
}

void parse_single_token_to_node()
{
    struct token *token = token_next();
    struct node *node = NULL;
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        node = node_create(&(struct node){.type = NODE_TYPE_NUMBER, .llnum = token->llnum});
        break;
    case TOKEN_TYPE_IDENTIFIER:
        node = node_create(&(struct node){.type = NODE_TYPE_IDENTIFIER, .sval = token->sval});
        break;

    case TOKEN_TYPE_STRING:
        node = node_create(&(struct node){.type = NODE_TYPE_STRING, .sval = token->sval});
        break;

    default:
        compiler_error(current_process, "This is not a single token that can be converted to a node");
    }
}

void parse_expressionable_for_op(struct history* history, const char* op)
{
    parse_expressionable(history);
}

void parse_exp_normal(struct history* history)
{
    struct token* op_token = token_peek_next();
    const char* op = op_token->sval;
    struct node* node_left = node_peek_expressionable_or_null();
    if (!node_left)
    {
        return;
    }

    // Pop off the operator token
    token_next();

    // Pop off the left node
    node_pop();
    node_left->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    parse_expressionable_for_op(history_down(history, history->flags), op);
    struct node* node_right = node_pop();
    node_right->flags |= NODE_FLAG_INSIDE_EXPRESSION;

    make_exp_node(node_left, node_right, op);
    struct node* exp_node = node_pop();

    // Reorder the expression

    node_push(exp_node);
}

int parse_exp(struct history* history)
{
    parse_exp_normal(history);
    return 0;
}

int parse_expressionable_single(struct history *history)
{
    struct token *token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    history->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    int res = -1;
    switch(token->type)
    {
        case TOKEN_TYPE_NUMBER:
            parse_single_token_to_node();
            res = 0;
        break;

        case TOKEN_TYPE_OPERATOR:
            parse_exp(history);
            res = 0;
        break;
    }
    return res;
}
void parse_expressionable(struct history *history)
{
    while (parse_expressionable_single(history) == 0)
    {
    }
}
int parse_next()
{
    struct token *token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    int res = 0;
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_TYPE_STRING:
        parse_expressionable(history_begin(0));
        break;
    }
    return 0;
}

int parse(struct compile_process *process)
{
    current_process = process;
    parser_last_token = NULL;
    node_set_vector(process->node_vec, process->node_tree_vec);
    struct node *node = NULL;
    vector_set_peek_pointer(process->token_vec, 0);
    while (parse_next() == 0)
    {
        node = node_peek();
        vector_push(process->node_tree_vec, &node);
    }
    return PARSE_ALL_OK;
}