#include "compiler.h"

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
