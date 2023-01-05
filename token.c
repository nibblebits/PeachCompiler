#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#define PRIMTIIVE_TYPES_TOTAL 7
const char* primitive_types[PRIMTIIVE_TYPES_TOTAL] = {
    "void", "char", "short", "int", "long", "float", "double"
};

bool token_is_identifier(struct token* token)
{
    return token && token->type == TOKEN_TYPE_IDENTIFIER;
}

bool token_is_keyword(struct token *token, const char *value)
{
    return token && token->type == TOKEN_TYPE_KEYWORD && S_EQ(token->sval, value);
}

bool token_is_symbol(struct token* token, char c)
{
    return token && token->type == TOKEN_TYPE_SYMBOL && token->cval == c;
}

bool token_is_operator(struct token* token, const char* val)
{
    return token && token->type == TOKEN_TYPE_OPERATOR && S_EQ(token->sval, val);
}


bool is_operator_token(struct token* token)
{
    return token && token->type == TOKEN_TYPE_OPERATOR;
}

bool token_is_nl_or_comment_or_newline_seperator(struct token *token)
{
    if (!token)
        return false;
        
    return token->type == TOKEN_TYPE_NEWLINE ||
           token->type == TOKEN_TYPE_COMMENT ||
           token_is_symbol(token, '\\');
}


bool token_is_primitive_keyword(struct token* token)
{
    if (!token)
        return false;

    if (token->type != TOKEN_TYPE_KEYWORD)
        return false;

    for (int i = 0; i < PRIMTIIVE_TYPES_TOTAL; i++)
    {
        if (S_EQ(primitive_types[i], token->sval))
            return true;
    }

    return false;
}

void tokens_join_buffer_write_token(struct buffer* fmt_buf, struct token* token)
{
    switch(token->type)
    {
        case TOKEN_TYPE_IDENTIFIER:
        case TOKEN_TYPE_OPERATOR:
        case TOKEN_TYPE_KEYWORD:
            buffer_printf(fmt_buf, "%s", token->sval);
            break;
        
        case TOKEN_TYPE_STRING:
            buffer_printf(fmt_buf, "\"%s\"", token->sval);
            break;

        case TOKEN_TYPE_NUMBER:
            buffer_printf(fmt_buf, "%lld", token->llnum);
            break;

        case TOKEN_TYPE_NEWLINE:
            buffer_printf(fmt_buf, "\n");
            break;

        case TOKEN_TYPE_SYMBOL:
            buffer_printf(fmt_buf, "%c", token->cval);
            break;
        default:
            FAIL_ERR("BUG: Incompatible token");
    }
}
struct vector* tokens_join_vector(struct compile_process* compiler, struct vector* token_vec)
{
    struct buffer* buf = buffer_create();
    vector_set_peek_pointer(token_vec, 0);
    struct token* token = vector_peek(token_vec);
    while(token)
    {
        tokens_join_buffer_write_token(buf, token);
        token = vector_peek(token_vec);
    }

    struct lex_process* lex_process = tokens_build_for_string(compiler, buffer_ptr(buf));
    assert(lex_process);
    return lex_process->token_vec;
}