#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <assert.h>

enum
{
    TYPEDEF_TYPE_STANDARD,
    // A structure typedef is basically "typedef struct ABC { int x; } AAA;"
    TYPEDEF_TYPE_STRUCTURE_TYPEDEF
};

struct typedef_type
{
    int type;
    const char *definiton_name;
    struct vector *value;
    struct typedef_structure
    {
        const char *sname;
    } structure;
};

enum
{
    PREPROCESSOR_FLAG_EVALUATE_NODE = 0b00000001
};

enum
{
    PREPROCESSOR_NUMBER_NODE,
    PREPROCESSOR_IDENTIFIER_NODE,
    PREPROCESSOR_KEYWORD_NODE,
    PREPROCESSOR_UNARY_NODE,
    PREPROCESSOR_EXPRESSION_NODE,
    PREPROCESSOR_PARENTHESES_NODE,
    PREPROCESSOR_JOINED_NODE,
    PREPROCESSOR_TENARY_NODE
};

struct preprocessor_node
{
    int type;
    struct preprocessor_const_val
    {
        union
        {
            char cval;
            unsigned int inum;
            long lnum;
            long long llnum;
            unsigned long ulnum;
            unsigned long long ullnum;
        };

    } const_val;

    union
    {
        struct preprocessor_exp_node
        {
            struct preprocessor_node *left;
            struct preprocessor_node *right;
            const char *op;
        } exp;

        struct preprocessor_unary_node
        {
            struct preprocessor_node *operand_node;
            const char *op;
            struct preprocessor_unary_indirection
            {
                int depth;
            } indirection;
        } unary_node;

        struct preprocessor_parenthesis
        {
            struct preprocessor_node *exp;
        } parenthesis;

        struct preprocessor_joined_node
        {
            struct preprocessor_node *left;
            struct preprocessor_node *right;
        } joined;

        struct preprocessor_tenary_node
        {
            struct preprocessor_node *true_node;
            struct preprocessor_node *false_node;
        } tenary;
    };

    const char *sval;
};

void preprocessor_handle_token(struct compile_process *compiler, struct token *token);
int preprocessor_parse_evaluate(struct compile_process *compiler, struct vector *token_vec);
int preprocessor_evaluate(struct compile_process *compiler, struct preprocessor_node *root_node);
int preprocessor_handle_identifier_for_token_vector(struct compile_process *compiler, struct vector *src_vec, struct vector *dst_vec, struct token *token);
struct vector *preprocessor_definition_value(struct preprocessor_definition *definition);
void preprocessor_token_vec_push_src_resolve_definition(struct compile_process *compiler, struct vector *src_vec, struct vector *dst_vec, struct token *token);
void preprocessor_macro_function_push_something(struct compile_process *compiler, struct preprocessor_definition *definition, struct preprocessor_function_arguments *arguments, struct token *arg_token, struct vector *definition_token_vec, struct vector *value_vec_target);

void preprocessor_execute_warning(struct compile_process *compiler, const char *msg)
{
    compiler_warning(compiler, "#warning %s", msg);
}

void preprocessor_execute_error(struct compile_process *compiler, const char *msg)
{
    compiler_error(compiler, "#error %s", msg);
}

struct preprocessor_included_file *preprocessor_add_included_file(struct preprocessor *preprocessor, const char *filename)
{
    struct preprocessor_included_file *included_file = calloc(1, sizeof(struct preprocessor_included_file));
    strncpy(included_file->filename, filename, sizeof(included_file->filename));
    vector_push(preprocessor->includes, &included_file);
    return included_file;
}

void preprocessor_create_static_include(struct preprocessor *preprocessor, const char *filename, PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION creation_handler)
{
    struct preprocessor_included_file *included_file = preprocessor_add_included_file(preprocessor, filename);
    creation_handler(preprocessor, included_file);
}

bool preprocessor_is_keyword(const char *type)
{
    return S_EQ(type, "defined");
}

struct vector *preprocessor_build_value_vector_for_integer(int value)
{
    struct vector *token_vec = vector_create(sizeof(struct token));
    struct token t1 = {};
    t1.type = TOKEN_TYPE_NUMBER;
    t1.llnum = value;
    vector_push(token_vec, &t1);
    return token_vec;
}

void preprocessor_token_vec_push_keyword_and_identifier(struct vector *token_vec, const char *keyword, const char *identifier)
{
    struct token t1 = {};
    t1.type = TOKEN_TYPE_KEYWORD;
    t1.sval = keyword;
    struct token t2 = {};
    t2.type = TOKEN_TYPE_IDENTIFIER;
    t2.sval = identifier;
    vector_push(token_vec, &t1);
    vector_push(token_vec, &t2);
}

void *preprocessor_node_create(struct preprocessor_node *node)
{
    struct preprocessor_node *result = calloc(1, sizeof(struct preprocessor_node));
    memcpy(result, node, sizeof(struct preprocessor_node));
    return result;
}

int preprocessor_definition_argument_exists(struct preprocessor_definition *definition, const char *name)
{
    vector_set_peek_pointer(definition->standard.arguments, 0);
    int i = 0;
    const char *current = vector_peek(definition->standard.arguments);
    while (current)
    {
        if (S_EQ(current, name))
            return i;

        i++;
        current = vector_peek(definition->standard.arguments);
    }

    return -1;
}

struct preprocessor_function_argument *preprocessor_function_argument_at(struct preprocessor_function_arguments *arguments, int index)
{
    struct preprocessor_function_argument *argument = vector_at(arguments->arguments, index);
    return argument;
}

void preprocessor_token_push_to_function_arguments(struct preprocessor_function_arguments *arguments, struct token *token)
{
    struct preprocessor_function_argument arg = {};
    arg.tokens = vector_create(sizeof(struct token));
    vector_push(arg.tokens, token);
    vector_push(arguments->arguments, &arg);
}

void preprocessor_function_argument_push_to_vec(struct preprocessor_function_argument *argument, struct vector *vector_out)
{
    vector_set_peek_pointer(argument->tokens, 0);
    struct token *token = vector_peek(argument->tokens);
    while (token)
    {
        vector_push(vector_out, token);
        token = vector_peek(argument->tokens);
    }
}

void preprocessor_token_push_to_dst(struct vector *token_vec, struct token *token)
{
    struct token t = *token;
    vector_push(token_vec, &t);
}

void preprocessor_token_push_dst(struct compile_process *compiler, struct token *token)
{
    preprocessor_token_push_to_dst(compiler->token_vec, token);
}

void preprocessor_token_vec_push_src_to_dst(struct compile_process *compiler, struct vector *src_vec, struct vector *dst_vec)
{
    vector_set_peek_pointer(src_vec, 0);
    struct token *token = vector_peek(src_vec);
    while (token)
    {
        vector_push(dst_vec, token);
        token = vector_peek(src_vec);
    }
}

void preprocessor_token_vec_push_src(struct compile_process *compiler, struct vector *src_vec)
{
    preprocessor_token_vec_push_src_to_dst(compiler, src_vec, compiler->token_vec);
}

void preprocessor_token_vec_push_src_token(struct compile_process *compiler, struct token *token)
{
    vector_push(compiler->token_vec, token);
}

void preprocessor_initialize(struct preprocessor *preprocessor)
{
    memset(preprocessor, 0, sizeof(struct preprocessor));
    preprocessor->definitions = vector_create(sizeof(struct preprocessor_definition *));
    preprocessor->includes = vector_create(sizeof(struct preprocessor_included_file *));
    preprocessor_create_definitions(preprocessor);
}

struct preprocessor *preprocessor_create(struct compile_process *compiler)
{
    struct preprocessor *preprocessor = calloc(1, sizeof(struct preprocessor));
    preprocessor_initialize(preprocessor);
    preprocessor->compiler = compiler;
    return preprocessor;
}
struct token *preprocessor_previous_token(struct compile_process *compiler)
{
    return vector_peek_at(compiler->token_vec_original, compiler->token_vec_original->pindex - 1);
}

struct token *preprocessor_next_token(struct compile_process *compiler)
{
    return vector_peek(compiler->token_vec_original);
}

struct token *preprocessor_next_token_no_increment(struct compile_process *compiler)
{
    return vector_peek_no_increment(compiler->token_vec_original);
}

struct token *preprocessor_peek_next_token_skip_nl(struct compile_process *compiler)
{
    struct token *token = preprocessor_next_token_no_increment(compiler);
    while (token && token->type == TOKEN_TYPE_NEWLINE)
    {
        token = preprocessor_next_token(compiler);
    }
    token = preprocessor_next_token_no_increment(compiler);
    return token;
}

struct token *preprocessor_peek_next_token_with_vector_no_increment(struct compile_process *compiler, struct vector *priority_token_vec, bool overflow_use_compiler_tokens)
{
    struct token *token = vector_peek_no_increment(priority_token_vec);
    if (token == NULL && overflow_use_compiler_tokens)
    {
        token = preprocessor_peek_next_token_skip_nl(compiler);
    }

    return token;
}
struct token *preprocessor_next_token_with_vector(struct compile_process *compiler, struct vector *priority_token_vec, bool overflow_use_compiler_tokens)
{
    struct token *token = vector_peek(priority_token_vec);
    if (token == NULL && overflow_use_compiler_tokens)
    {
        token = preprocessor_peek_next_token_skip_nl(compiler);
    }

    return token;
}

void *preprocessor_handle_number_token(struct expressionable *expressionable)
{
    struct token *token = expressionable_token_next(expressionable);
    return preprocessor_node_create(&(struct preprocessor_node){.type = PREPROCESSOR_NUMBER_NODE, .const_val.llnum = token->llnum});
}

void *preprocessor_handle_identifier_token(struct expressionable *expressionable)
{
    struct token *token = expressionable_token_next(expressionable);
    bool is_preprocessor_keyword = preprocessor_is_keyword(token->sval);
    int type = PREPROCESSOR_IDENTIFIER_NODE;
    if (is_preprocessor_keyword)
    {
        type = PREPROCESSOR_KEYWORD_NODE;
    }
    return preprocessor_node_create(&(struct preprocessor_node){.type = type, .sval = token->sval});
}

void preprocessor_make_unary_node(struct expressionable *expressionable, const char *op, void *right_operand_node_ptr)
{
    struct preprocessor_node *right_operand_node = right_operand_node_ptr;
    void *unary_node = preprocessor_node_create(&(struct preprocessor_node){.type = PREPROCESSOR_UNARY_NODE, .unary_node.op = op, .unary_node.operand_node = right_operand_node});
    expressionable_node_push(expressionable, unary_node);
}

void preprocessor_make_expression_node(struct expressionable *expressionable, void *left_node_ptr, void *right_node_ptr, const char *op)
{
    struct preprocessor_node exp_node;
    exp_node.type = PREPROCESSOR_EXPRESSION_NODE;
    exp_node.exp.left = left_node_ptr;
    exp_node.exp.right = right_node_ptr;
    exp_node.exp.op = op;
    expressionable_node_push(expressionable, preprocessor_node_create(&exp_node));
}

void preprocessor_make_parentheses_node(struct expressionable *expressionable, void *node_ptr)
{
    struct preprocessor_node *node = node_ptr;
    struct preprocessor_node parentheses_node;
    parentheses_node.type = PREPROCESSOR_PARENTHESES_NODE;
    parentheses_node.parenthesis.exp = node_ptr;
    expressionable_node_push(expressionable, preprocessor_node_create(&parentheses_node));
}
void preprocessor_make_tenary_node(struct expressionable *expressionable, void *true_result_node_ptr, void *false_result_node_ptr)
{
    struct preprocessor_node *true_result_node = true_result_node_ptr;
    struct preprocessor_node *false_result_node = false_result_node_ptr;

    expressionable_node_push(expressionable, preprocessor_node_create(&(struct preprocessor_node){.type = PREPROCESSOR_TENARY_NODE, .tenary.true_node = true_result_node, .tenary.false_node = false_result_node}));
}

int preprocessor_get_node_type(struct expressionable *expressionable, void *node)
{
    int generic_type = EXPRESSIONABLE_GENERIC_TYPE_NON_GENERIC;
    struct preprocessor_node *preprocessor_node = node;
    switch (preprocessor_node->type)
    {
    case PREPROCESSOR_NUMBER_NODE:
        generic_type = EXPRESSIONABLE_GENERIC_TYPE_NUMBER;
        break;

    case PREPROCESSOR_IDENTIFIER_NODE:
    case PREPROCESSOR_KEYWORD_NODE:
        generic_type = EXPRESSIONABLE_GENERIC_TYPE_IDENTIFIER;
        break;

    case PREPROCESSOR_UNARY_NODE:
        generic_type = EXPRESSIONABLE_GENERIC_TYPE_UNARY;
        break;
    case PREPROCESSOR_EXPRESSION_NODE:
        generic_type = EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION;
        break;

    case PREPROCESSOR_PARENTHESES_NODE:
        generic_type = EXPRESSIONABLE_GENERIC_TYPE_PARENTHESES;
        break;
    }

    return generic_type;
}

void *preprocessor_get_left_node(struct expressionable *expressionable, void *target_node)
{
    struct preprocessor_node *node = target_node;
    return node->exp.left;
}

void *preprocessor_get_right_node(struct expressionable *expressionable, void *target_node)
{
    struct preprocessor_node *node = target_node;
    return node->exp.right;
}

const char *preprocessor_get_node_operator(struct expressionable *expressionable, void *target_node)
{
    struct preprocessor_node *preprocessor_node = target_node;
    return preprocessor_node->exp.op;
}

void **preprocessor_get_left_node_address(struct expressionable *expressionable, void *target_node)
{
    return (void **)&((struct preprocessor_node *)(target_node))->exp.left;
}

void **preprocessor_get_right_node_address(struct expressionable *expressionable, void *target_node)
{
    return (void **)&((struct preprocessor_node *)(target_node))->exp.right;
}

void preprocessor_set_expression_node(struct expressionable *expressionable, void *node, void *left_node, void *right_node, const char *op)
{
    struct preprocessor_node *preprocessor_node = node;
    preprocessor_node->exp.left = left_node;
    preprocessor_node->exp.right = right_node;
    preprocessor_node->exp.op = op;
}

bool preprocessor_should_join_nodes(struct expressionable *expressionable, void *previous_node_ptr, void *node_ptr)
{
    return true;
}

void *preprocessor_join_nodes(struct expressionable *expressionable, void *previous_node_ptr, void *node_ptr)
{
    struct preprocessor_node *previous_node = previous_node_ptr;
    struct preprocessor_node *node = node_ptr;
    return preprocessor_node_create(&(struct preprocessor_node){.type = PREPROCESSOR_JOINED_NODE, .joined.left = previous_node, .joined.right = node});
}

bool preprocessor_expecting_additional_node(struct expressionable *expressionable, void *node_ptr)
{
    struct preprocessor_node *node = node_ptr;
    return node->type == PREPROCESSOR_KEYWORD_NODE && S_EQ(node->sval, "defined");
}

bool preprocessor_is_custom_operator(struct expressionable *expressionable, struct token *token)
{
    return false;
}

struct expressionable_config preprocessor_expressionable_config =
    {
        .callbacks.handle_number_callback = preprocessor_handle_number_token,
        .callbacks.handle_identifier_callback = preprocessor_handle_identifier_token,
        .callbacks.make_unary_node = preprocessor_make_unary_node,
        .callbacks.make_expression_node = preprocessor_make_expression_node,
        .callbacks.make_parentheses_node = preprocessor_make_parentheses_node,
        .callbacks.make_tenary_node = preprocessor_make_tenary_node,
        .callbacks.get_node_type = preprocessor_get_node_type,
        .callbacks.get_left_node = preprocessor_get_left_node,
        .callbacks.get_right_node = preprocessor_get_right_node,
        .callbacks.get_node_operator = preprocessor_get_node_operator,
        .callbacks.get_left_node_address = preprocessor_get_left_node_address,
        .callbacks.get_right_node_address = preprocessor_get_right_node_address,
        .callbacks.set_exp_node = preprocessor_set_expression_node,
        .callbacks.should_join_nodes = preprocessor_should_join_nodes,
        .callbacks.join_nodes = preprocessor_join_nodes,
        .callbacks.expecting_additional_node = preprocessor_expecting_additional_node,
        .callbacks.is_custom_operator = preprocessor_is_custom_operator,
};

bool preprocessor_is_preprocessor_keyword(const char *value)
{
    return S_EQ(value, "define") ||
           S_EQ(value, "undef") ||
           S_EQ(value, "warning") ||
           S_EQ(value, "error") ||
           S_EQ(value, "if") ||
           S_EQ(value, "eleif") ||
           S_EQ(value, "ifdef") ||
           S_EQ(value, "ifndef") ||
           S_EQ(value, "endif") ||
           S_EQ(value, "include") ||
           S_EQ(value, "typedef");
}
bool preprocessor_token_is_preprocessor_keyword(struct token *token)
{
    return token->type == TOKEN_TYPE_IDENTIFIER || token->type == TOKEN_TYPE_KEYWORD && preprocessor_is_preprocessor_keyword(token->sval);
}
bool preprocessor_token_is_define(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "define"));
}
bool preprocessor_token_is_undef(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "undef"));
}

bool preprocessor_token_is_warning(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "warning"));
}
bool preprocessor_token_is_error(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "error"));
}

bool preprocessor_token_is_if(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "if"));
}

bool preprocessor_token_is_ifdef(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "ifdef"));
}

bool preprocessor_token_is_ifndef(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "ifndef"));
}

bool preprocessor_token_is_include(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return (S_EQ(token->sval, "include"));
}



struct buffer *preprocessor_multi_value_string(struct compile_process *compiler)
{
    struct buffer *buffer = buffer_create();
    struct token *value_token = preprocessor_next_token(compiler);
    while (value_token)
    {
        if (value_token->type == TOKEN_TYPE_NEWLINE)
        {
            break;
        }

        if (token_is_symbol(value_token, '\\'))
        {
            // Skip the new line
            preprocessor_next_token(compiler);
            value_token = preprocessor_next_token(compiler);
            continue;
        }

        buffer_printf(buffer, "%s", value_token->sval);
        value_token = preprocessor_next_token(compiler);
    }

    return buffer;
}

void preprocessor_multi_value_insert_to_vector(struct compile_process *compiler, struct vector *value_token_vec)
{
    struct token *value_token = preprocessor_next_token(compiler);
    while (value_token)
    {
        if (value_token->type == TOKEN_TYPE_NEWLINE)
        {
            break;
        }

        if (token_is_symbol(value_token, '\\'))
        {
            // This allows for another line skip the new line
            preprocessor_next_token(compiler);
            value_token = preprocessor_next_token(compiler);
            continue;
        }

        vector_push(value_token_vec, value_token);
        value_token = preprocessor_next_token(compiler);
    }
}

void preprocessor_definition_remove(struct preprocessor *preprocessor, const char *name)
{
    vector_set_peek_pointer(preprocessor->definitions, 0);
    struct preprocessor_definition *current_definition = vector_peek_ptr(preprocessor->definitions);
    while (current_definition)
    {
        if (S_EQ(current_definition->name, name))
        {
            // Remove the definition
            vector_pop_last_peek(preprocessor->definitions);
        }
        current_definition = vector_peek_ptr(preprocessor->definitions);
    }
}
struct preprocessor_definition *preprocessor_definition_create(const char *name, struct vector *value_vec, struct vector *arguments, struct preprocessor *preprocessor)
{
    // Unset the definition if its already created
    preprocessor_definition_remove(preprocessor, name);

    struct preprocessor_definition *definition = calloc(1, sizeof(struct preprocessor_definition));
    definition->type = PREPROCESSOR_DEFINITION_STANDARD;
    definition->name = name;
    definition->standard.value = value_vec;
    definition->standard.arguments = arguments;
    definition->preprocessor = preprocessor;

    if (arguments && vector_count(definition->standard.arguments))
    {
        definition->type = PREPROCESSOR_DEFINITION_MACRO_FUNCTION;
    }

    vector_push(preprocessor->definitions, &definition);
    return definition;
}

struct preprocessor_definition *preprocessor_definition_create_native(const char *name,
                                                                      PREPROCESSOR_DEFINITION_NATIVE_CALL_EVALUATE evaluate,
                                                                      PREPROCESSOR_DEFINITION_NATIVE_CALL_VALUE value,
                                                                      struct preprocessor *preprocessor)
{
    struct preprocessor_definition *definition = calloc(sizeof(struct preprocessor_definition), 1);
    definition->type = PREPROCESSOR_DEFINITION_NATIVE_CALLBACK;
    definition->name = name;
    definition->native.evaluate = evaluate;
    definition->native.value = value;
    definition->preprocessor = preprocessor;
    vector_push(preprocessor->definitions, &definition);
    return definition;
}

struct preprocessor_definition *preprocessor_definition_create_typedef(const char *name, struct vector *value_vec, struct preprocessor *preprocessor)
{
    struct preprocessor_definition *definition = calloc(1, sizeof(struct preprocessor_definition));
    definition->type = PREPROCESSOR_DEFINITION_TYPEDEF;
    definition->name = name;
    definition->_typedef.value = value_vec;
    definition->preprocessor = preprocessor;
    vector_push(preprocessor->definitions, &definition);
    return definition;
}

struct preprocessor_definition *preprocessor_get_definition(struct preprocessor *preprocessor, const char *name)
{
    vector_set_peek_pointer(preprocessor->definitions, 0);
    struct preprocessor_definition *definition = vector_peek_ptr(preprocessor->definitions);
    while (definition)
    {
        if (S_EQ(definition->name, name))
        {
            break;
        }

        definition = vector_peek_ptr(preprocessor->definitions);
    }

    return definition;
}

struct vector *preprocessor_definition_value_for_standard(struct preprocessor_definition *definition)
{
    return definition->standard.value;
}

struct vector *preprocessor_definition_value_for_typedef_or_other(struct preprocessor_definition *definition)
{
    if (definition->type != PREPROCESSOR_DEFINITION_TYPEDEF)
    {
        return preprocessor_definition_value(definition);
    }

    return definition->_typedef.value;
}

struct vector *preprocessor_definition_value_for_typedef(struct preprocessor_definition *definition)
{
    return preprocessor_definition_value_for_typedef_or_other(definition);
}

struct vector *preprocessor_definition_value_for_native(struct preprocessor_definition *definition,
                                                        struct preprocessor_function_arguments *arguments)
{
    return definition->native.value(definition, arguments);
}

struct vector *preprocessor_definition_value_with_arguments(struct preprocessor_definition *definition, struct preprocessor_function_arguments *arguments)
{
    if (definition->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK)
    {
        return preprocessor_definition_value_for_native(definition, arguments);
    }
    else if (definition->type == PREPROCESSOR_DEFINITION_TYPEDEF)
    {
        return preprocessor_definition_value_for_typedef(definition);
    }

    return preprocessor_definition_value_for_standard(definition);
}
struct vector *preprocessor_definition_value(struct preprocessor_definition *definition)
{
    return preprocessor_definition_value_with_arguments(definition, NULL);
}

int preprocessor_parse_evaluate_token(struct compile_process *compiler, struct token *token)
{
    struct vector *token_vec = vector_create(sizeof(struct token));
    vector_push(token_vec, token);
    return preprocessor_parse_evaluate(compiler, token_vec);
}

int preprocessor_definition_evaluated_value_for_standard(struct preprocessor_definition *definition)
{
    struct token *token = vector_back(definition->standard.value);
    if (token->type == TOKEN_TYPE_IDENTIFIER)
    {
        return preprocessor_parse_evaluate_token(definition->preprocessor->compiler, token);
    }

    if (token->type != TOKEN_TYPE_NUMBER)
    {
        compiler_error(definition->preprocessor->compiler, "The definition must hold a number value. Unable to use macro IF");
    }
    return token->llnum;
}

int preprocessor_definition_evaluated_value_for_native(struct preprocessor_definition* definition,
    struct preprocessor_function_arguments* arguments)
{
    return definition->native.evaluate(definition, arguments);
}

int preprocessor_definition_evaluated_value(struct preprocessor_definition *definition, struct preprocessor_function_arguments *arguments)
{
    if (definition->type == PREPROCESSOR_DEFINITION_STANDARD)
    {
        return preprocessor_definition_evaluated_value_for_standard(definition);
    }
    else if (definition->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK)
    {
        return preprocessor_definition_evaluated_value_for_native(definition, arguments);
    }

    compiler_error(definition->preprocessor->compiler, "The definition cannot be evaluated into a number");
}
bool preprocessor_is_next_macro_arguments(struct compile_process *compiler)
{
    bool res = false;
    vector_save(compiler->token_vec_original);
    struct token *last_token = preprocessor_previous_token(compiler);
    struct token *current_token = preprocessor_next_token(compiler);

    if (token_is_operator(current_token, "(") && (!last_token || !last_token->whitespace))
    {
        res = true;
    }

    vector_restore(compiler->token_vec_original);
    return res;
}

void preprocessor_parse_macro_argument_declaration(struct compile_process *compiler, struct vector *arguments)
{
    if (token_is_operator(preprocessor_next_token_no_increment(compiler), "("))
    {
        // Skip the (
        preprocessor_next_token(compiler);
        struct token *next_token = preprocessor_next_token(compiler);
        while (!token_is_symbol(next_token, ')'))
        {
            if (next_token->type != TOKEN_TYPE_IDENTIFIER)
            {
                compiler_error(compiler, "You must provide an identifier in the preprocessor definition!");
            }

            vector_push(arguments, (void *)next_token->sval);
            next_token = preprocessor_next_token(compiler);
            if (!token_is_operator(next_token, ",") && !token_is_symbol(next_token, ')'))
            {
                compiler_error(compiler, "Incomplete sequence for macro arguments");
            }

            if (token_is_symbol(next_token, ')'))
            {
                break;
            }

            // Skip the "," operator
            next_token = preprocessor_next_token(compiler);
        }
    }
}
void preprocessor_handle_definition_token(struct compile_process *compiler)
{
    struct token *name_token = preprocessor_next_token(compiler);
    struct vector *arguments = vector_create(sizeof(const char *));

    if (preprocessor_is_next_macro_arguments(compiler))
    {
        preprocessor_parse_macro_argument_declaration(compiler, arguments);
    }
    // Value can be composed of many tokens
    struct vector *value_token_vec = vector_create(sizeof(struct token));
    preprocessor_multi_value_insert_to_vector(compiler, value_token_vec);

    struct preprocessor *preprocessor = compiler->preprocessor;
    preprocessor_definition_create(name_token->sval, value_token_vec, arguments, preprocessor);
}

void preprocessor_handle_undef_token(struct compile_process *compiler)
{
    struct token *name_token = preprocessor_next_token(compiler);
    preprocessor_definition_remove(compiler->preprocessor, name_token->sval);
}
void preprocessor_handle_warning_token(struct compile_process *compiler)
{
    struct buffer *str_buf = preprocessor_multi_value_string(compiler);
    preprocessor_execute_warning(compiler, buffer_ptr(str_buf));
}

void preprocessor_handle_error_token(struct compile_process *compiler)
{
    struct buffer *str_buf = preprocessor_multi_value_string(compiler);
    preprocessor_execute_error(compiler, buffer_ptr(str_buf));
}

struct token *preprocessor_hashtag_and_identifier(struct compile_process *compiler, const char *str)
{
    if (!preprocessor_next_token_no_increment(compiler))
    {
        return NULL;
    }

    if (!token_is_symbol(preprocessor_next_token_no_increment(compiler), '#'))
    {
        return NULL;
    }

    vector_save(compiler->token_vec_original);
    // Skip the hashtag symbol
    preprocessor_next_token(compiler);

    struct token *target_token = preprocessor_next_token_no_increment(compiler);
    if ((token_is_identifier(target_token) && S_EQ(target_token->sval, str)) || token_is_keyword(target_token, str))
    {
        // Pop off the target token
        preprocessor_next_token(compiler);
        // Purge the vector save
        vector_save_purge(compiler->token_vec_original);
        return target_token;
    }

    vector_restore(compiler->token_vec_original);
    return NULL;
}

/**
 * @brief Return true if their is a hastag and any type of preprocessor if statement
 * elif is not included.
 *
 * @param compiler
 * @return true
 * @return false
 */
bool preprocessor_is_hashtag_and_any_starting_if(struct compile_process *compiler)
{
    return preprocessor_hashtag_and_identifier(compiler, "if") ||
           preprocessor_hashtag_and_identifier(compiler, "ifdef") ||
           preprocessor_hashtag_and_identifier(compiler, "ifndef");
}

void preprocessor_skip_to_endif(struct compile_process *compiler)
{
    while (!preprocessor_hashtag_and_identifier(compiler, "endif"))
    {
        if (preprocessor_is_hashtag_and_any_starting_if(compiler))
        {
            preprocessor_skip_to_endif(compiler);
            continue;
        }
        preprocessor_next_token(compiler);
    }
}

void preprocessor_read_to_end_if(struct compile_process *compiler, bool true_clause)
{
    while (preprocessor_next_token_no_increment(compiler) && !preprocessor_hashtag_and_identifier(compiler, "endif"))
    {
        if (true_clause)
        {
            preprocessor_handle_token(compiler, preprocessor_next_token(compiler));
            continue;
        }

        // Skip the unexpected token
        preprocessor_next_token(compiler);

        if (preprocessor_is_hashtag_and_any_starting_if(compiler))
        {
            preprocessor_skip_to_endif(compiler);
        }
    }
}
int preprocessor_evaluate_number(struct preprocessor_node *node)
{
    return node->const_val.llnum;
}

int preprocessor_evaluate_identifier(struct compile_process *compiler, struct preprocessor_node *node)
{
    struct preprocessor *preprocessor = compiler->preprocessor;
    struct preprocessor_definition *definition = preprocessor_get_definition(preprocessor, node->sval);
    if (!definition)
    {
        return true;
    }

    if (vector_count(preprocessor_definition_value(definition)) > 1)
    {
        struct vector *node_vector = vector_create(sizeof(struct preprocessor_node *));
        struct expressionable *expressionable = expressionable_create(&preprocessor_expressionable_config, preprocessor_definition_value(definition), node_vector, EXPRESSIONABLE_FLAG_IS_PREPROCESSOR_EXPRESSION);
        expressionable_parse(expressionable);
        struct preprocessor_node *node = expressionable_node_pop(expressionable);
        int val = preprocessor_evaluate(compiler, node);
        return val;
    }

    if (vector_count(preprocessor_definition_value(definition)) == 0)
    {
        return false;
    }

    return preprocessor_definition_evaluated_value(definition, NULL);
}

int preprocessor_arithmetic(struct compile_process *compiler, long left_operand, long right_operand, const char *op)
{
    bool success = false;
    long result = arithmetic(compiler, left_operand, right_operand, op, &success);
    if (!success)
    {
        compiler_error(compiler, "We do not support the operator %s for preprocessor arithmetic\n", op);
    }
    return result;
}

struct preprocessor_function_arguments *preprocessor_function_arguments_create()
{
    struct preprocessor_function_arguments *args = calloc(1, sizeof(struct preprocessor_function_arguments));
    args->arguments = vector_create(sizeof(struct preprocessor_function_argument));
    return args;
}

void preprocessor_number_push_to_function_arguments(struct preprocessor_function_arguments *arguments, int64_t number)
{
    struct token t;
    t.type = TOKEN_TYPE_NUMBER;
    t.llnum = number;
    preprocessor_token_push_to_function_arguments(arguments, &t);
}

bool preprocessor_exp_is_macro_function_call(struct preprocessor_node *node)
{
    return node->type == PREPROCESSOR_EXPRESSION_NODE && S_EQ(node->exp.op, "()") && node->exp.left->type == PREPROCESSOR_IDENTIFIER_NODE;
}

void preprocessor_evaluate_function_call_argument(struct compile_process *compiler, struct preprocessor_node *node, struct preprocessor_function_arguments *arguments)
{
    if (node->type == PREPROCESSOR_EXPRESSION_NODE && S_EQ(node->exp.op, ","))
    {
        preprocessor_evaluate_function_call_argument(compiler, node->exp.left, arguments);
        preprocessor_evaluate_function_call_argument(compiler, node->exp.right, arguments);
        return;
    }
    else if (node->type == PREPROCESSOR_PARENTHESES_NODE)
    {
        preprocessor_evaluate_function_call_argument(compiler, node->parenthesis.exp, arguments);
        return;
    }

    preprocessor_number_push_to_function_arguments(arguments, preprocessor_evaluate(compiler, node));
}

void preprocessor_evaluate_function_call_arguments(struct compile_process *compiler, struct preprocessor_node *node, struct preprocessor_function_arguments *arguments)
{
    preprocessor_evaluate_function_call_argument(compiler, node, arguments);
}

bool preprocessor_is_macro_function(struct preprocessor_definition *definition)
{
    return definition->type == PREPROCESSOR_DEFINITION_MACRO_FUNCTION || definition->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK;
}
int preprocessor_function_arguments_count(struct preprocessor_function_arguments *arguments)
{
    if (!arguments)
    {
        return 0;
    }

    return vector_count(arguments->arguments);
}

int preprocessor_macro_function_push_argument(struct compile_process *compiler, struct preprocessor_definition *definition, struct preprocessor_function_arguments *arguments, const char *arg_name, struct vector *definition_token_vec, struct vector *value_vec_target)
{
    int argument_index = preprocessor_definition_argument_exists(definition, arg_name);
    if (argument_index != -1)
    {
        preprocessor_function_argument_push_to_vec(preprocessor_function_argument_at(arguments, argument_index), value_vec_target);
    }
    return argument_index;
}

void preprocessor_token_vec_push_src_token_to_dst(struct compile_process *compiler, struct token *token, struct vector *dst_vec)
{
    vector_push(dst_vec, token);
}

bool preprocessor_token_is_typedef(struct token *token)
{
    if (!preprocessor_token_is_preprocessor_keyword(token))
    {
        return false;
    }

    return S_EQ(token->sval, "typedef");
}

void preprocessor_handle_typedef_body_for_non_struct_or_union(struct compile_process *compiler, struct vector *token_vec, struct typedef_type *td, struct vector *src_vec, bool overflow_use_token_vec)
{
    td->type = TYPEDEF_TYPE_STANDARD;
    struct token *token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
    while (token)
    {
        if (token_is_symbol(token, ';'))
        {
            break;
        }

        preprocessor_token_vec_push_src_resolve_definition(compiler, src_vec, token_vec, token);
        token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
    }
}

void preprocessor_handle_typedef_body_for_brackets(struct compile_process *compiler, struct vector *token_vec, struct vector *src_vec, bool overflow_use_token_vec)
{
    struct token *token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
    while (token)
    {
        if (token_is_symbol(token, '{'))
        {
            vector_push(token_vec, token);
            preprocessor_handle_typedef_body_for_brackets(compiler, token_vec, src_vec, overflow_use_token_vec);
            token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
            continue;
        }

        vector_push(token_vec, token);
        if (token_is_symbol(token, '}'))
        {
            break;
        }
        token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
    }
}
void preprocessor_handle_typedef_body_for_struct_or_union(struct compile_process *compiler, struct vector *token_vec, struct typedef_type *td, struct vector *src_vec, bool overflow_use_token_vec)
{
    struct token *token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
    assert(token_is_keyword(token, "struct"));

    td->type = TYPEDEF_TYPE_STRUCTURE_TYPEDEF;

    // Push the struct keyword
    vector_push(token_vec, token);

    token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
    // Do we have a name for this structure
    if (token->type == TOKEN_TYPE_IDENTIFIER)
    {
        td->structure.sname = token->sval;
        vector_push(token_vec, token);
        token = preprocessor_peek_next_token_with_vector_no_increment(compiler, src_vec, overflow_use_token_vec);
        if (token->type == TOKEN_TYPE_IDENTIFIER)
        {
            // just a declaration typedef struct Point point;
            vector_push(token_vec, token);
            return;
        }
    }

    // Handle typedef structure with body.
    while (token)
    {
        if (token_is_symbol(token, '{'))
        {
            // Structure typedef
            td->type = TYPEDEF_TYPE_STRUCTURE_TYPEDEF;
            vector_push(token_vec, token);
            preprocessor_handle_typedef_body_for_brackets(compiler, token_vec, src_vec, overflow_use_token_vec);
            token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
            continue;
        }

        if (token_is_symbol(token, ';'))
        {
            break;
        }

        preprocessor_token_vec_push_src_resolve_definition(compiler, src_vec, token_vec, token);
        token = preprocessor_next_token_with_vector(compiler, src_vec, overflow_use_token_vec);
    }
}

void preprocessor_handle_typedef_body(struct compile_process *compiler, struct vector *token_vec, struct typedef_type *td, struct vector *src_vec, bool overflow_use_token_vec)
{
    memset(td, 0, sizeof(struct typedef_type));

    struct token *token = preprocessor_peek_next_token_with_vector_no_increment(compiler, src_vec, overflow_use_token_vec);
    if (token_is_keyword(token, "struct") || token_is_keyword(token, "union"))
    {
        preprocessor_handle_typedef_body_for_struct_or_union(compiler, token_vec, td, src_vec, overflow_use_token_vec);
    }
    else
    {
        preprocessor_handle_typedef_body_for_non_struct_or_union(compiler, token_vec, td, src_vec, overflow_use_token_vec);
    }
}

void preprocessor_token_push_semicolon(struct compile_process *compiler)
{
    struct token t1;
    t1.type = TOKEN_TYPE_SYMBOL;
    t1.cval = ';';
    vector_push(compiler->token_vec, &t1);
}

void preprocessor_handle_typedef_token(struct compile_process *compiler, struct vector *src_vec, bool overflow_use_token_vec)
{
    // We expect a format like " typedef unsigned int ABC;"
    struct vector *token_vec = vector_create(sizeof(struct token));
    struct typedef_type td;
    preprocessor_handle_typedef_body(compiler, token_vec, &td, src_vec, overflow_use_token_vec);

    struct token *name_token = vector_back_or_null(token_vec);
    if (!name_token)
    {
        compiler_error(compiler, "We expected a name token for your typedef");
    }

    if (name_token->type != TOKEN_TYPE_IDENTIFIER)
    {
        compiler_error(compiler, "The typedef name has to be a valid identifier");
    }

    td.definiton_name = name_token->sval;

    // Pop off the name token
    vector_pop(token_vec);

    // THis is a typedef structure we need to push the structure body
    // to the output so it can be found.
    if (td.type == TYPEDEF_TYPE_STRUCTURE_TYPEDEF)
    {
        preprocessor_token_vec_push_src(compiler, token_vec);
        preprocessor_token_push_semicolon(compiler);

        token_vec = vector_create(sizeof(struct token));
        preprocessor_token_vec_push_keyword_and_identifier(token_vec, "struct", td.structure.sname);
    }

    struct preprocessor *preprocessor = compiler->preprocessor;
    preprocessor_definition_create_typedef(td.definiton_name, token_vec, preprocessor);
}
void preprocessor_token_vec_push_src_resolve_definition(struct compile_process *compiler, struct vector *src_vec, struct vector *dst_vec, struct token *token)
{
    if (preprocessor_token_is_typedef(token))
    {
        preprocessor_handle_typedef_token(compiler, src_vec, true);
        return;
    }

    if (token->type == TOKEN_TYPE_IDENTIFIER)
    {
        preprocessor_handle_identifier_for_token_vector(compiler, src_vec, dst_vec, token);
        return;
    }

    preprocessor_token_vec_push_src_token_to_dst(compiler, token, dst_vec);
}

void preprocessor_token_vec_push_src_resolve_definitions(struct compile_process *compiler, struct vector *src_vec, struct vector *dst_vec)
{
    assert(src_vec != compiler->token_vec);
    vector_set_peek_pointer(src_vec, 0);
    struct token *token = vector_peek(src_vec);
    while (token)
    {
        preprocessor_token_vec_push_src_resolve_definition(compiler, src_vec, dst_vec, token);
        token = vector_peek(src_vec);
    }
}
int preprocessor_macro_function_push_something_definition(struct compile_process *compiler, struct preprocessor_definition *definition, struct preprocessor_function_arguments *arguments, struct token *arg_token, struct vector *definition_token_vec, struct vector *value_vec_target)
{
    if (arg_token->type != TOKEN_TYPE_IDENTIFIER)
    {
        return -1;
    }

    const char *arg_name = arg_token->sval;
    int res = preprocessor_macro_function_push_argument(compiler, definition, arguments, arg_name, definition_token_vec, value_vec_target);
    if (res != -1)
    {
        return 0;
    }

    // We failed so theirs no argument
    struct preprocessor_definition *arg_definition = preprocessor_get_definition(compiler->preprocessor, arg_name);
    if (arg_definition)
    {
        preprocessor_token_vec_push_src_resolve_definitions(compiler, preprocessor_definition_value(arg_definition), compiler->token_vec);
        return 0;
    }

    // Sad day something went wrong.
    return -1;
}

void preprocessor_handle_concat_part(struct compile_process *compiler, struct preprocessor_definition *definition,
                                     struct preprocessor_function_arguments *arguments, struct token *token, struct vector *definition_token_vec,
                                     struct vector *value_vec_target)
{
    preprocessor_macro_function_push_something(compiler, definition, arguments, token, definition_token_vec, value_vec_target);
}

void preprocessor_handle_concat_finalize(struct compile_process *compiler, struct vector *tmp_vec, struct vector *value_vec_target)
{
    struct vector *joined_vec = tokens_join_vector(compiler, tmp_vec);
    vector_insert(value_vec_target, joined_vec, 0);
}

void preprocessor_handle_concat(struct compile_process *compiler, struct preprocessor_definition *definition,
                                struct preprocessor_function_arguments *arguments, struct token *arg_token, struct vector *definition_token_vec,
                                struct vector *value_vec_target)
{

    // Lets skip the hashtags ##
    vector_peek(definition_token_vec);
    vector_peek(definition_token_vec);

    struct token *right_token = vector_peek(definition_token_vec);
    if (!right_token)
    {
        compiler_error(compiler, "No right operand provided for concat preprocessor operator ##");
    }

    struct vector *tmp_vec = vector_create(sizeof(struct token));
    preprocessor_handle_concat_part(compiler, definition, arguments, arg_token, definition_token_vec, tmp_vec);
    preprocessor_handle_concat_part(compiler, definition, arguments, right_token, definition_token_vec, tmp_vec);
    preprocessor_handle_concat_finalize(compiler, tmp_vec, value_vec_target);
}
bool preprocessor_is_next_double_hash(struct vector *definition_token_vec)
{
    bool is_double_hash = true;
    vector_save(definition_token_vec);
    struct token *next_token = vector_peek(definition_token_vec);
    if (!token_is_symbol(next_token, '#'))
    {
        is_double_hash = false;
        goto out;
    }

    next_token = vector_peek(definition_token_vec);
    if (!token_is_symbol(next_token, '#'))
    {
        is_double_hash = false;
        goto out;
    }

out:
    vector_restore(definition_token_vec);
    return is_double_hash;
}
void preprocessor_macro_function_push_something(struct compile_process *compiler, struct preprocessor_definition *definition, struct preprocessor_function_arguments *arguments, struct token *arg_token, struct vector *definition_token_vec, struct vector *value_vec_target)
{
    if (preprocessor_is_next_double_hash(definition_token_vec))
    {
        preprocessor_handle_concat(compiler, definition, arguments, arg_token, definition_token_vec, value_vec_target);
        return;
    }

    int res = preprocessor_macro_function_push_something_definition(compiler, definition, arguments, arg_token, definition_token_vec, value_vec_target);
    if (res == -1)
    {
        vector_push(value_vec_target, arg_token);
    }
}

void preprocessor_handle_function_argument_to_string(struct compile_process *compiler, struct vector *src_vec, struct vector *value_vec_target, struct preprocessor_definition *definition, struct preprocessor_function_arguments *arguments)
{
    struct token *next_token = vector_peek(src_vec);
    if (!next_token || next_token->type != TOKEN_TYPE_IDENTIFIER)
    {
        compiler_error(compiler, "No macro function argument was provided to convert to a string");
    }

    int argument_index = preprocessor_definition_argument_exists(definition, next_token->sval);
    if (argument_index < 0)
    {
        compiler_error(compiler, "Unexpected macro function argument %s", next_token->sval);
    }

    struct preprocessor_function_argument *argument = preprocessor_function_argument_at(arguments, argument_index);
    if (!argument)
    {
        compiler_error(compiler, "BUG: argument exists but failed to pull");
    }

    struct token *first_token_for_argument = vector_peek_at(argument->tokens, 0);
    // Lets create a string token
    struct token str_token = {0};
    str_token.type = TOKEN_TYPE_STRING;
    str_token.sval = first_token_for_argument->between_arguments;
    vector_push(value_vec_target, &str_token);
}

int preprocessor_macro_function_execute(struct compile_process *compiler, const char *function_name, struct preprocessor_function_arguments *arguments, int flags)
{
    struct preprocessor *preprocessor = compiler->preprocessor;
    struct preprocessor_definition *definition = preprocessor_get_definition(preprocessor, function_name);
    if (!definition)
    {
        compiler_error(compiler, "Trying to call unknown macro function %s", function_name);
    }

    if (!preprocessor_is_macro_function(definition))
    {
        compiler_error(compiler, "This definition %s is not a macro function", function_name);
    }

    if (vector_count(definition->standard.arguments) != preprocessor_function_arguments_count(arguments) && definition->type != PREPROCESSOR_DEFINITION_NATIVE_CALLBACK)
    {
        compiler_error(compiler, "You passed too many arguments to function %s", function_name);
    }

    struct vector *value_vec_target = vector_create(sizeof(struct token));
    struct vector *definition_token_vec = preprocessor_definition_value_with_arguments(definition, arguments);
    vector_set_peek_pointer(definition_token_vec, 0);
    struct token *token = vector_peek(definition_token_vec);
    while (token)
    {
        if (token_is_symbol(token, '#'))
        {
            preprocessor_handle_function_argument_to_string(compiler, definition_token_vec, value_vec_target, definition, arguments);
            token = vector_peek(definition_token_vec);
            continue;
        }

        preprocessor_macro_function_push_something(compiler, definition, arguments, token, definition_token_vec, value_vec_target);
        token = vector_peek(definition_token_vec);
    }

    if (flags & PREPROCESSOR_FLAG_EVALUATE_NODE)
    {
        return preprocessor_parse_evaluate(compiler, value_vec_target);
    }
    preprocessor_token_vec_push_src(compiler, value_vec_target);
    return 0;
}
int preprocessor_evaluate_function_call(struct compile_process *compiler, struct preprocessor_node *node)
{
    const char *macro_func_name = node->exp.left->sval;
    struct preprocessor_node *call_arguments = node->exp.right->parenthesis.exp;
    struct preprocessor_function_arguments *arguments = preprocessor_function_arguments_create();

    // Evaluate all of the preprocessor arguments
    preprocessor_evaluate_function_call_arguments(compiler, call_arguments, arguments);
    return preprocessor_macro_function_execute(compiler, macro_func_name, arguments, PREPROCESSOR_FLAG_EVALUATE_NODE);
}

int preprocessor_evaluate_exp(struct compile_process *compiler, struct preprocessor_node *node)
{
    if (preprocessor_exp_is_macro_function_call(node))
    {
        return preprocessor_evaluate_function_call(compiler, node);
    }

    long left_operand = preprocessor_evaluate(compiler, node->exp.left);
    if (node->exp.right->type == PREPROCESSOR_TENARY_NODE)
    {
        if (left_operand)
        {
            return preprocessor_evaluate(compiler, node->exp.right->tenary.true_node);
        }
        else
        {
            return preprocessor_evaluate(compiler, node->exp.right->tenary.false_node);
        }
    }

    long right_operand = preprocessor_evaluate(compiler, node->exp.right);
    return preprocessor_arithmetic(compiler, left_operand, right_operand, node->exp.op);
}

int preprocessor_evaluate_unary(struct compile_process *compiler, struct preprocessor_node *node)
{
    int res = 0;
    const char *op = node->unary_node.op;
    struct preprocessor_node *right_operand = node->unary_node.operand_node;
    if (S_EQ(op, "!"))
    {
        res = !preprocessor_evaluate(compiler, right_operand);
    }
    else if (S_EQ(op, "~"))
    {
        res = ~preprocessor_evaluate(compiler, right_operand);
    }
    else if (S_EQ(op, "-"))
    {
        res = -preprocessor_evaluate(compiler, right_operand);
    }
    else
    {
        compiler_error(compiler, "The operator is not supported");
    }
    return res;
}

int preprocessor_evaluate_parentheses(struct compile_process *compiler, struct preprocessor_node *node)
{
    return preprocessor_evaluate(compiler, node->parenthesis.exp);
}

const char *preprocessor_pull_string_from(struct preprocessor_node *root_node)
{
    const char *result = NULL;
    switch (root_node->type)
    {
    case PREPROCESSOR_PARENTHESES_NODE:
        result = preprocessor_pull_string_from(root_node->parenthesis.exp);
        break;

    case PREPROCESSOR_KEYWORD_NODE:
    case PREPROCESSOR_IDENTIFIER_NODE:
        result = root_node->sval;
        break;

    case PREPROCESSOR_EXPRESSION_NODE:
        result = preprocessor_pull_string_from(root_node->exp.left);
        break;
    }

    return result;
}

const char *preprocessor_pull_defined_value(struct compile_process *compiler, struct preprocessor_node *joined_node)
{
    const char *val = preprocessor_pull_string_from(joined_node->joined.right);
    if (!val)
    {
        compiler_error(compiler, "Expecting an identifier node for defined keyword right operand");
    }

    return val;
}

int preprocessor_evaluate_joined_node_defined(struct compile_process *compiler, struct preprocessor_node *node)
{
    const char *right_val = preprocessor_pull_defined_value(compiler, node);
    return preprocessor_get_definition(compiler->preprocessor, right_val) != NULL;
}
int preprocessor_evaluate_joined_node(struct compile_process *compiler, struct preprocessor_node *node)
{
    if (node->joined.left->type != PREPROCESSOR_KEYWORD_NODE)
    {
        return 0;
    }

    int res = 0;
    if (S_EQ(node->joined.left->sval, "defined"))
    {
        res = preprocessor_evaluate_joined_node_defined(compiler, node);
    }

    return res;
}

int preprocessor_evaluate(struct compile_process *compiler, struct preprocessor_node *root_node)
{
    struct preprocessor_node *current = root_node;
    int result = 0;
    switch (current->type)
    {
    case PREPROCESSOR_NUMBER_NODE:
        result = preprocessor_evaluate_number(current);
        break;

    case PREPROCESSOR_IDENTIFIER_NODE:
        result = preprocessor_evaluate_identifier(compiler, current);
        break;

    case PREPROCESSOR_UNARY_NODE:
        result = preprocessor_evaluate_unary(compiler, current);
        break;

    case PREPROCESSOR_EXPRESSION_NODE:
        result = preprocessor_evaluate_exp(compiler, current);
        break;

    case PREPROCESSOR_PARENTHESES_NODE:
        result = preprocessor_evaluate_parentheses(compiler, current);
        break;

    case PREPROCESSOR_JOINED_NODE:
        result = preprocessor_evaluate_joined_node(compiler, current);
        break;
    }
    return result;
}

int preprocessor_parse_evaluate(struct compile_process *compiler, struct vector *token_vec)
{
    struct vector *node_vector = vector_create(sizeof(struct preprocessor_node *));
    struct expressionable *expressionable = expressionable_create(&preprocessor_expressionable_config, token_vec, node_vector, 0);
    expressionable_parse(expressionable);
    struct preprocessor_node *root_node = expressionable_node_pop(expressionable);
    return preprocessor_evaluate(compiler, root_node);
}

void preprocessor_handle_if_token(struct compile_process *compiler)
{
    int result = preprocessor_parse_evaluate(compiler, compiler->token_vec_original);
    preprocessor_read_to_end_if(compiler, result > 0);
}

void preprocessor_handle_ifdef_token(struct compile_process *compiler)
{
    struct token *condition_token = preprocessor_next_token(compiler);
    if (!condition_token)
    {
        compiler_error(compiler, "No condition token was provided.\n");
    }

    struct preprocessor_definition *definition = preprocessor_get_definition(compiler->preprocessor, condition_token->sval);

    // Read the body of the ifdef
    preprocessor_read_to_end_if(compiler, definition != NULL);
}

void preprocessor_handle_ifndef_token(struct compile_process *compiler)
{
    struct token *condition_token = preprocessor_next_token(compiler);
    if (!condition_token)
    {
        compiler_error(compiler, "No condition token was provided\n");
    }
    struct preprocessor_definition *definition = preprocessor_get_definition(compiler->preprocessor, condition_token->sval);
    preprocessor_read_to_end_if(compiler, definition == NULL);
}

struct token* preprocessor_next_token_skip_nl(struct compile_process* compiler)
{
    struct token* token = preprocessor_next_token(compiler);
    while(token && token->type == TOKEN_TYPE_NEWLINE)
    {
        token = preprocessor_next_token(compiler);
    }

    return token;
}

void preprocessor_handle_include_token(struct compile_process* compiler)
{
    struct token* file_path_token = preprocessor_next_token_skip_nl(compiler);
    if (!file_path_token)
    {
        compiler_error(compiler, "No file path provided for include");
    }

    struct compile_process* new_compile_process = compile_include(file_path_token->sval, compiler);
    if (!new_compile_process)
    {
        PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION handler = 
            preprocessor_static_include_handler_for(file_path_token->sval);
        if (handler)
        {
            // Handle it
            preprocessor_create_static_include(compiler->preprocessor, file_path_token->sval, handler);
            return;
        }

        compiler_error(compiler, "The file does not exist %s unable to include", file_path_token->sval);
    }

    preprocessor_token_vec_push_src(compiler, new_compile_process->token_vec);
}

int preprocessor_handle_hashtag_token(struct compile_process *compiler, struct token *token)
{
    bool is_preprocessed = false;
    struct token *next_token = preprocessor_next_token(compiler);
    if (preprocessor_token_is_define(next_token))
    {
        // handle the definition token.
        preprocessor_handle_definition_token(compiler);
        is_preprocessed = true;
    }
    else if (preprocessor_token_is_undef(next_token))
    {
        preprocessor_handle_undef_token(compiler);
        is_preprocessed = true;
    }
    else if (preprocessor_token_is_warning(next_token))
    {
        preprocessor_handle_warning_token(compiler);
        is_preprocessed = true;
    }
    else if (preprocessor_token_is_error(next_token))
    {
        preprocessor_handle_error_token(compiler);
        is_preprocessed = true;
    }
    else if (preprocessor_token_is_if(next_token))
    {
        preprocessor_handle_if_token(compiler);
        is_preprocessed = true;
    }
    else if (preprocessor_token_is_ifdef(next_token))
    {
        preprocessor_handle_ifdef_token(compiler);
        is_preprocessed = true;
    }
    else if (preprocessor_token_is_ifndef(next_token))
    {
        preprocessor_handle_ifndef_token(compiler);
        is_preprocessed = true;
    }
    else if(preprocessor_token_is_include(next_token))
    {
        preprocessor_handle_include_token(compiler);
        is_preprocessed = true;
    }

    return is_preprocessed;
}
void preprocessor_handle_symbol(struct compile_process *compiler, struct token *token)
{
    int is_preprocessed = false;
    if (token->cval == '#')
    {
        is_preprocessed = preprocessor_handle_hashtag_token(compiler, token);
    }

    if (!is_preprocessed)
    {
        preprocessor_token_push_dst(compiler, token);
    }
}

struct token *preprocessor_handle_identifier_macro_call_argument_parse_parentheses(struct compile_process *compiler, struct vector *src_vec, struct vector *value_vec, struct preprocessor_function_arguments *arguments, struct token *left_bracket_token)
{
    // Push the  left bracket token to the stack
    vector_push(value_vec, left_bracket_token);

    struct token *next_token = vector_peek(src_vec);
    while (next_token && !token_is_symbol(next_token, ')'))
    {
        if (token_is_operator(next_token, "("))
        {
            next_token = preprocessor_handle_identifier_macro_call_argument_parse_parentheses(compiler, src_vec, value_vec, arguments, next_token);
        }
        vector_push(value_vec, next_token);
        next_token = vector_peek(src_vec);
    }

    if (!next_token)
    {
        compiler_error(compiler, "You did not end your parentheses expecting a )");
    }
    vector_push(value_vec, next_token);
    return vector_peek(src_vec);
}

void preprocessor_function_argument_push(struct preprocessor_function_arguments *arguments, struct vector *value_vec)
{
    struct preprocessor_function_argument arg;
    arg.tokens = vector_clone(value_vec);
    vector_push(arguments->arguments, &arg);
}

void preprocessor_handle_identifier_macro_call_argument(struct preprocessor_function_arguments *arguments, struct vector *token_vec)
{
    preprocessor_function_argument_push(arguments, token_vec);
}

struct token *preprocessor_handle_identifier_macro_call_argument_parse(struct compile_process *compiler, struct vector *src_vec, struct vector *value_vec, struct preprocessor_function_arguments *arguments, struct token *token)
{
    if (token_is_operator(token, "("))
    {
        return preprocessor_handle_identifier_macro_call_argument_parse_parentheses(compiler, src_vec, value_vec, arguments, token);
    }
    if (token_is_symbol(token, ')'))
    {
        // We are done handling the call argument
        preprocessor_handle_identifier_macro_call_argument(arguments, value_vec);
        return NULL;
    }

    if (token_is_operator(token, ","))
    {
        preprocessor_handle_identifier_macro_call_argument(arguments, value_vec);
        // Clear the value vector ready for next argument
        vector_clear(value_vec);
        token = vector_peek(src_vec);
        return token;
    }

    vector_push(value_vec, token);
    token = vector_peek(src_vec);
    return token;
}
struct preprocessor_function_arguments *preprocessor_handle_identifier_macro_call_arguments(struct compile_process *compiler, struct vector *src_vec)
{
    // Skip the left bracket
    vector_peek(src_vec);

    struct preprocessor_function_arguments *arguments = preprocessor_function_arguments_create();
    struct token *token = vector_peek(src_vec);
    struct vector *value_vec = vector_create(sizeof(struct token));
    while (token)
    {
        token = preprocessor_handle_identifier_macro_call_argument_parse(compiler, src_vec, value_vec, arguments, token);
    }

    vector_free(value_vec);
    return arguments;
}
int preprocessor_handle_identifier_for_token_vector(struct compile_process *compiler, struct vector *src_vec, struct vector *dst_vec, struct token *token)
{
    struct preprocessor_definition *definition = preprocessor_get_definition(compiler->preprocessor, token->sval);
    if (!definition)
    {
        // Nothing to do with us, maybe a variable of some kind. Not macro related.
        preprocessor_token_push_to_dst(dst_vec, token);
        return -1;
    }

    if (definition->type == PREPROCESSOR_DEFINITION_TYPEDEF)
    {
        preprocessor_token_vec_push_src_to_dst(compiler, preprocessor_definition_value(definition), dst_vec);
        return 0;
    }

    if (token_is_operator(vector_peek_no_increment(src_vec), "("))
    {
        struct preprocessor_function_arguments *arguments = preprocessor_handle_identifier_macro_call_arguments(compiler, src_vec);
        const char *function_name = token->sval;
        preprocessor_macro_function_execute(compiler, function_name, arguments, 0);
        return 0;
    }

    struct vector *definition_val = preprocessor_definition_value(definition);
    preprocessor_token_vec_push_src_resolve_definitions(compiler, preprocessor_definition_value(definition), dst_vec);
    return 0;
}
int preprocessor_handle_identifier(struct compile_process *compiler, struct token *token)
{
    return preprocessor_handle_identifier_for_token_vector(compiler, compiler->token_vec_original, compiler->token_vec, token);
}
void preprocessor_handle_keyword(struct compile_process *compiler, struct token *token)
{
    if (preprocessor_token_is_typedef(token))
    {
        preprocessor_handle_typedef_token(compiler, compiler->token_vec_original, false);
        return;
    }

    // This is not for us to deal with
    preprocessor_token_push_dst(compiler, token);
}

void preprocessor_handle_token(struct compile_process *compiler, struct token *token)
{
    switch (token->type)
    {
        // Handle all tokens here..

    case TOKEN_TYPE_SYMBOL:
        preprocessor_handle_symbol(compiler, token);
        break;

    case TOKEN_TYPE_KEYWORD:
        preprocessor_handle_keyword(compiler, token);
        break;

    case TOKEN_TYPE_IDENTIFIER:
        preprocessor_handle_identifier(compiler, token);
        break;
    case TOKEN_TYPE_NEWLINE:
        // Ignored
        break;

    default:
        preprocessor_token_push_dst(compiler, token);
    };
}
int preprocessor_run(struct compile_process *compiler)
{
    preprocessor_add_included_file(compiler->preprocessor, compiler->cfile.abs_path);
    vector_set_peek_pointer(compiler->token_vec_original, 0);
    struct token *token = preprocessor_next_token(compiler);
    while (token)
    {
        preprocessor_handle_token(compiler, token);
        token = preprocessor_next_token(compiler);
    }

    return 0;
}