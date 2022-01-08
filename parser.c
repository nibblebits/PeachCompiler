#include "compiler.h"
#include "helpers/vector.h"

static struct compile_process* current_process;
static struct token* parser_last_token;

static void parser_ignore_nl_or_comment(struct token* token)
{
    while(token && token_is_nl_or_comment_or_newline_seperator(token))
    {
        // Skip the token
        vector_peek(current_process->token_vec);
        token = vector_peek_no_increment(current_process->token_vec);
    }
}

static struct token* token_next()
{
    struct token* next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    current_process->pos = next_token->pos;
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static struct token* token_peek_next()
{
    struct token* next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    return vector_peek_no_increment(current_process->token_vec);
}

void parse_single_token_to_node()
{
    struct token* token = token_next();
    struct node* node = NULL;
    switch(token->type)
    {
        case TOKEN_TYPE_NUMBER:
        node = node_create(&(struct node){.type=NODE_TYPE_NUMBER, .llnum=token->llnum});
        break;
        case TOKEN_TYPE_IDENTIFIER:
        node = node_create(&(struct node){.type=NODE_TYPE_IDENTIFIER,.sval=token->sval});
        break;

        case TOKEN_TYPE_STRING:
        node = node_create(&(struct node){.type=NODE_TYPE_STRING, .sval=token->sval});
        break;

        default:
        compiler_error(current_process, "This is not a single token that can be converted to a node");
    }
}

int parse_next()
{
    struct token* token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    int res = 0;
    switch(token->type)
    {
       case TOKEN_TYPE_NUMBER:
       case TOKEN_TYPE_IDENTIFIER:
       case TOKEN_TYPE_STRING:
        parse_single_token_to_node();
       break;
    }
    return 0;
}

int parse(struct compile_process* process)
{
    current_process = process;
    parser_last_token = NULL;
    node_set_vector(process->node_vec, process->node_tree_vec);
    struct node* node = NULL;
    vector_set_peek_pointer(process->token_vec, 0);
    while(parse_next() == 0)
    {
        node = node_peek();
        vector_push(process->node_tree_vec, &node);
    }
    return PARSE_ALL_OK;
}