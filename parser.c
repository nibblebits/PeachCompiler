#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

static struct compile_process *current_process;
static struct fixup_system *parser_fixup_sys;
static struct token *parser_last_token;

extern struct node *parser_current_body;
extern struct node *parser_current_function;

// NODE_TYPE_BLANK
struct node *parser_blank_node;

extern struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS];

enum
{
    PARSER_SCOPE_ENTITY_ON_STACK = 0b00000001,
    PARSER_SCOPE_ENTITY_STRUCTURE_SCOPE = 0b00000010,
};

struct parser_scope_entity
{
    // The entity flags of the scope entity
    int flags;

    // The stack offset of the scope entity.
    int stack_offset;

    // Variable declaration
    struct node *node;
};

struct parser_scope_entity *parser_new_scope_entity(struct node *node, int stack_offset, int flags)
{
    struct parser_scope_entity *entity = calloc(1, sizeof(struct parser_scope_entity));
    entity->node = node;
    entity->flags = flags;
    entity->stack_offset = stack_offset;
    return entity;
}

struct parser_scope_entity *parser_scope_last_entity_stop_global_scope()
{
    return scope_last_entity_stop_at(current_process, current_process->scope.root);
}

enum
{
    HISTORY_FLAG_INSIDE_UNION = 0b00000001,
    HISTORY_FLAG_IS_UPWARD_STACK = 0b00000010,
    HISTORY_FLAG_IS_GLOBAL_SCOPE = 0b00000100,
    HISTORY_FLAG_INSIDE_STRUCTURE = 0b00001000,
    HISTORY_FLAG_INSIDE_FUNCTION_BODY = 0b00010000,
    HISTORY_FLAG_IN_SWITCH_STATEMENT = 0b00100000,
    HISTORY_FLAG_PARENTHESES_IS_NOT_A_FUNCTION_CALL = 0b01000000,

};

struct history_cases
{
    // A vector of parsed_switch_case
    struct vector *cases;
    // Is there a default keyword inthe switch statement body
    bool has_default_case;
};

struct history
{
    int flags;
    struct parser_history_switch
    {
        struct history_cases* case_data;
    } _switch;
};

int parser_get_pointer_depth();
void parser_deal_with_additional_expression();
void parse_for_parentheses(struct history *history);

static struct history *history_begin(int flags)
{
    struct history *history = calloc(1, sizeof(struct history));
    history->flags = flags;
    return history;
}

static struct history *history_down(struct history *history, int flags)
{
    struct history *new_history = calloc(1, sizeof(struct history));
    memcpy(new_history, history, sizeof(struct history));
    new_history->flags = flags;
    return new_history;
}

struct parser_history_switch parser_new_switch_statement(struct history *history)
{
    memset(&history->_switch, 0, sizeof(&history->_switch));
    history->_switch.case_data = calloc(1, sizeof(struct history_cases));
    history->_switch.case_data->cases = vector_create(sizeof(struct parsed_switch_case));
    history->flags |= HISTORY_FLAG_IN_SWITCH_STATEMENT;
    return history->_switch;
}

void parser_end_switch_statement(struct parser_history_switch *switch_history)
{
    // Do nothing.
}

void parser_register_case(struct history *history, struct node *case_node)
{
    assert(history->flags & HISTORY_FLAG_IN_SWITCH_STATEMENT);
    struct parsed_switch_case scase;
    scase.index = case_node->stmt._case.exp->llnum;
    vector_push(history->_switch.case_data->cases, &scase);
}

int parse_expressionable_single(struct history *history);
void parse_expressionable(struct history *history);
void parse_body(size_t *variable_size, struct history *history);
void parse_keyword(struct history *history);
struct vector *parse_function_arguments(struct history *history);
void parse_expressionable_root(struct history *history);
void parse_label(struct history *history);
void parse_for_tenary(struct history *history);
void parse_datatype(struct datatype *dtype);
void parse_for_cast();

void parser_scope_new()
{
    scope_new(current_process, 0);
}

void parser_scope_finish()
{
    scope_finish(current_process);
}

struct parser_scope_entity *parser_scope_last_entity()
{
    return scope_last_entity(current_process);
}

void parser_scope_push(struct parser_scope_entity *entity, size_t size)
{
    scope_push(current_process, entity, size);
}

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
    if (next_token)
    {
        current_process->pos = next_token->pos;
    }
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static struct token *token_peek_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    return vector_peek_no_increment(current_process->token_vec);
}

static bool token_next_is_operator(const char *op)
{
    struct token *token = token_peek_next();
    return token_is_operator(token, op);
}

static bool token_next_is_keyword(const char *keyword)
{
    struct token *token = token_peek_next();
    return token_is_keyword(token, keyword);
}

static bool token_next_is_symbol(char c)
{
    struct token *token = token_peek_next();
    return token_is_symbol(token, c);
}

static void expect_sym(char c)
{
    struct token *next_token = token_next();
    if (!next_token || next_token->type != TOKEN_TYPE_SYMBOL || next_token->cval != c)
    {
        compiler_error(current_process, "Expecting symbol %c however something else was provided\n", c);
    }
}

static void expect_op(const char *op)
{
    struct token *next_token = token_next();
    if (!next_token || next_token->type != TOKEN_TYPE_OPERATOR || !S_EQ(next_token->sval, op))
    {
        compiler_error(current_process, "Expecting the operator %s but something else was provided\n", next_token->sval);
    }
}

static void expect_keyword(const char *keyword)
{
    struct token *next_token = token_next();
    if (!next_token || next_token->type != TOKEN_TYPE_KEYWORD || !S_EQ(next_token->sval, keyword))
    {
        compiler_error(current_process, "Expecting the keyword %s but something was provided\n", keyword);
    }
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

void parse_expressionable_for_op(struct history *history, const char *op)
{
    parse_expressionable(history);
}

static int parser_get_precedence_for_operator(const char *op, struct expressionable_op_precedence_group **group_out)
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

static bool parser_left_op_has_priority(const char *op_left, const char *op_right)
{
    struct expressionable_op_precedence_group *group_left = NULL;
    struct expressionable_op_precedence_group *group_right = NULL;

    if (S_EQ(op_left, op_right))
    {
        return false;
    }

    int precdence_left = parser_get_precedence_for_operator(op_left, &group_left);
    int precdence_right = parser_get_precedence_for_operator(op_right, &group_right);
    if (group_left->associtivity == ASSOCIATIVITY_RIGHT_TO_LEFT)
    {
        return false;
    }

    return precdence_left <= precdence_right;
}

void parser_node_shift_children_left(struct node *node)
{
    assert(node->type == NODE_TYPE_EXPRESSION);
    assert(node->exp.right->type == NODE_TYPE_EXPRESSION);

    const char *right_op = node->exp.right->exp.op;
    struct node *new_exp_left_node = node->exp.left;
    struct node *new_exp_right_node = node->exp.right->exp.left;
    make_exp_node(new_exp_left_node, new_exp_right_node, node->exp.op);

    // (50*20)
    struct node *new_left_operand = node_pop();
    // 120
    struct node *new_right_operand = node->exp.right->exp.right;
    node->exp.left = new_left_operand;
    node->exp.right = new_right_operand;
    node->exp.op = right_op;
}

void parser_node_move_right_left_to_left(struct node *node)
{
    make_exp_node(node->exp.left, node->exp.right->exp.left, node->exp.op);
    struct node *completed_node = node_pop();

    // We still need to deal with the right node
    const char *new_op = node->exp.right->exp.op;
    node->exp.left = completed_node;
    node->exp.right = node->exp.right->exp.right;
    node->exp.op = new_op;
}
void parser_reorder_expression(struct node **node_out)
{
    struct node *node = *node_out;
    if (node->type != NODE_TYPE_EXPRESSION)
    {
        return;
    }

    // No expressions, nothing to do
    if (node->exp.left->type != NODE_TYPE_EXPRESSION &&
        node->exp.right && node->exp.right->type != NODE_TYPE_EXPRESSION)
    {
        return;
    }

    // 50*E(30+20)
    // 50*EXPRESSION
    // EXPRESSION(50*EXPRESSION(30+20))
    // (50*30)+20
    if (node->exp.left->type != NODE_TYPE_EXPRESSION &&
        node->exp.right && node->exp.right->type == NODE_TYPE_EXPRESSION)
    {
        const char *right_op = node->exp.right->exp.op;
        if (parser_left_op_has_priority(node->exp.op, right_op))
        {
            // 50*E(20+120)
            // E(50*20)+120
            parser_node_shift_children_left(node);

            parser_reorder_expression(&node->exp.left);
            parser_reorder_expression(&node->exp.right);
        }
    }

    if ((is_array_node(node->exp.left) && is_node_assignment(node->exp.right)) || ((node_is_expression(node->exp.left, "()") || node_is_expression(node->exp.left, "[]")) && node_is_expression(node->exp.right, ",")))
    {
        parser_node_move_right_left_to_left(node);
    }
}

bool parser_is_unary_operator(const char *op)
{
    return is_unary_operator(op);
}

void parse_for_indirection_unary()
{
    int depth = parser_get_pointer_depth();
    parse_expressionable(history_begin(EXPRESSION_IS_UNARY));
    struct node *unary_operand_node = node_pop();
    make_unary_node("*", unary_operand_node, 0);

    struct node *unary_node = node_pop();
    unary_node->unary.indirection.depth = depth;
    node_push(unary_node);
}

void parse_for_normal_unary()
{
    const char *unary_op = token_next()->sval;
    parse_expressionable(history_begin(EXPRESSION_IS_UNARY));
    struct node *unary_operand_node = node_pop();
    make_unary_node(unary_op, unary_operand_node, 0);
}

void parse_for_unary()
{
    const char *unary_op = token_peek_next()->sval;
    if (op_is_indirection(unary_op))
    {
        parse_for_indirection_unary();
        return;
    }

    parse_for_normal_unary();
    parser_deal_with_additional_expression();
}

void parse_for_left_operanded_unary(struct node* left_operand_node, const char* unary_op)
{
    make_unary_node(unary_op, left_operand_node, UNARY_FLAG_IS_LEFT_OPERANDED_UNARY);
}
void parse_exp_normal(struct history *history)
{
    struct token *op_token = token_peek_next();
    const char *op = op_token->sval;
    struct node *node_left = node_peek_expressionable_or_null();
    if (!node_left)
    {
        if (!parser_is_unary_operator(op))
        {
            compiler_error(current_process, "The given expression has no left operand");
        }

        parse_for_unary();
        return;
    }

    // Pop off the operator token
    token_next();

    // Pop off the left node
    node_pop();

    if (is_left_operanded_unary_operator(op))
    {
        parse_for_left_operanded_unary(node_left, op);
        return;
    }

    node_left->flags |= NODE_FLAG_INSIDE_EXPRESSION;

    if (token_peek_next()->type == TOKEN_TYPE_OPERATOR)
    {
        if (S_EQ(token_peek_next()->sval, "("))
        {
            parse_for_parentheses(history_down(history, history->flags | HISTORY_FLAG_PARENTHESES_IS_NOT_A_FUNCTION_CALL));
        }
        else if (parser_is_unary_operator(token_peek_next()->sval))
        {
            parse_for_unary();
        }
        else
        {
            compiler_error(current_process, "Two operators are expected for a given expression for operator %s\n", token_peek_next()->sval);
        }
    }
    else
    {
        parse_expressionable_for_op(history_down(history, history->flags), op);
    }

    struct node *node_right = node_pop();
    node_right->flags |= NODE_FLAG_INSIDE_EXPRESSION;

    make_exp_node(node_left, node_right, op);
    struct node *exp_node = node_pop();

    // Reorder the expression
    parser_reorder_expression(&exp_node);
    node_push(exp_node);
}

void parser_deal_with_additional_expression()
{
    if (token_peek_next()->type == TOKEN_TYPE_OPERATOR)
    {
        parse_expressionable(history_begin(0));
    }
}

void parse_for_parentheses(struct history *history)
{
    expect_op("(");
    if (token_peek_next()->type == TOKEN_TYPE_KEYWORD)
    {
        parse_for_cast();
        return;
    }

    struct node *left_node = NULL;
    struct node *tmp_node = node_peek_or_null();

    // test(50+20)
    if (tmp_node && node_is_value_type(tmp_node))
    {
        left_node = tmp_node;
        node_pop();
    }

    struct node *exp_node = parser_blank_node;
    if (!token_next_is_symbol(')'))
    {
        parse_expressionable_root(history_begin(0));
        exp_node = node_pop();
    }
    expect_sym(')');

    make_exp_parentheses_node(exp_node);

    if (left_node)
    {
        struct node *parentheses_node = node_pop();
        make_exp_node(left_node, parentheses_node, "()");
    }

    parser_deal_with_additional_expression();
}

void parse_for_comma(struct history *history)
{
    // Skip the comma
    token_next();
    struct node *left_node = node_pop();
    parse_expressionable_root(history);
    struct node *right_node = node_pop();
    make_exp_node(left_node, right_node, ",");
}

void parse_for_array(struct history *history)
{
    struct node *left_node = node_peek_or_null();
    if (left_node)
    {
        node_pop();
    }

    expect_op("[");
    parse_expressionable_root(history);
    expect_sym(']');

    struct node *exp_node = node_pop();
    make_bracket_node(exp_node);

    if (left_node)
    {
        struct node *bracket_node = node_pop();
        make_exp_node(left_node, bracket_node, "[]");
    }
}

void parse_for_cast()
{
    // "(" is already parsed i.e (char) seen as char)
    struct datatype dtype = {};
    parse_datatype(&dtype);
    expect_sym(')');

    parse_expressionable(history_begin(0));
    struct node *operand_node = node_pop();
    make_cast_node(&dtype, operand_node);
}
int parse_exp(struct history *history)
{

    if (history->flags & EXPRESSION_IS_UNARY && !unary_operand_compatible(token_peek_next()))
    {
        return -1;
    }

    if (S_EQ(token_peek_next()->sval, "("))
    {
        parse_for_parentheses(history);
    }
    else if (S_EQ(token_peek_next()->sval, "["))
    {
        parse_for_array(history);
    }
    else if (S_EQ(token_peek_next()->sval, "?"))
    {
        parse_for_tenary(history);
    }
    else if (S_EQ(token_peek_next()->sval, ","))
    {
        parse_for_comma(history);
    }
    else
    {
        parse_exp_normal(history);
    }
    return 0;
}

void parse_identifier(struct history *history)
{
    assert(token_peek_next()->type == TOKEN_TYPE_IDENTIFIER);
    parse_single_token_to_node();
}

static bool is_keyword_variable_modifier(const char *val)
{
    return S_EQ(val, "unsigned") ||
           S_EQ(val, "signed") ||
           S_EQ(val, "static") ||
           S_EQ(val, "const") ||
           S_EQ(val, "extern") ||
           S_EQ(val, "__ignore_typecheck__");
}

void parse_datatype_modifiers(struct datatype *dtype)
{
    struct token *token = token_peek_next();
    while (token && token->type == TOKEN_TYPE_KEYWORD)
    {
        if (!is_keyword_variable_modifier(token->sval))
        {
            break;
        }

        if (S_EQ(token->sval, "signed"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_SIGNED;
        }
        else if (S_EQ(token->sval, "unsigned"))
        {
            dtype->flags &= ~DATATYPE_FLAG_IS_SIGNED;
        }
        else if (S_EQ(token->sval, "static"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_STATIC;
        }
        else if (S_EQ(token->sval, "const"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_CONST;
        }
        else if (S_EQ(token->sval, "extern"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_EXTERN;
        }
        else if (S_EQ(token->sval, "__ignore_typecheck__"))
        {
            dtype->flags |= DATATYPE_FLAG_IGNORE_TYPE_CHECKING;
        }

        token_next();
        token = token_peek_next();
    }
}

void parser_get_datatype_tokens(struct token **datatype_token, struct token **datatype_secondary_token)
{
    *datatype_token = token_next();
    struct token *next_token = token_peek_next();
    if (token_is_primitive_keyword(next_token))
    {
        *datatype_secondary_token = next_token;
        token_next();
    }
}

int parser_datatype_expected_for_type_string(const char *str)
{
    int type = DATA_TYPE_EXPECT_PRIMITIVE;
    if (S_EQ(str, "union"))
    {
        type = DATA_TYPE_EXPECT_UNION;
    }
    else if (S_EQ(str, "struct"))
    {
        type = DATA_TYPE_EXPECT_STRUCT;
    }

    return type;
}

int parser_get_random_type_index()
{
    static int x = 0;
    x++;
    return x;
}

struct token *parser_build_random_type_name()
{
    char tmp_name[25];
    sprintf(tmp_name, "customtypename_%i", parser_get_random_type_index());
    char *sval = malloc(sizeof(tmp_name));
    strncpy(sval, tmp_name, sizeof(tmp_name));
    struct token *token = calloc(1, sizeof(struct token));
    token->type = TOKEN_TYPE_IDENTIFIER;
    token->sval = sval;
    return token;
}

int parser_get_pointer_depth()
{
    int depth = 0;
    while (token_next_is_operator("*"))
    {
        depth++;
        token_next();
    }
    return depth;
}

bool parser_datatype_is_secondary_allowed(int expected_type)
{
    return expected_type == DATA_TYPE_EXPECT_PRIMITIVE;
}

bool parser_datatype_is_secondary_allowed_for_type(const char *type)
{
    return S_EQ(type, "long") || S_EQ(type, "short") || S_EQ(type, "double") || S_EQ(type, "float");
}

void parser_datatype_init_type_and_size_for_primitive(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out);
void parser_datatype_adjust_size_for_secondary(struct datatype *datatype, struct token *datatype_secondary_token)
{
    if (!datatype_secondary_token)
    {
        return;
    }

    struct datatype *secondary_data_type = calloc(1, sizeof(struct datatype));
    parser_datatype_init_type_and_size_for_primitive(datatype_secondary_token, NULL, secondary_data_type);
    datatype->size += secondary_data_type->size;
    datatype->secondary = secondary_data_type;
    datatype->flags |= DATATYPE_FLAG_IS_SECONDARY;
}

void parser_datatype_init_type_and_size_for_primitive(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out)
{
    if (!parser_datatype_is_secondary_allowed_for_type(datatype_token->sval) && datatype_secondary_token)
    {
        compiler_error(current_process, "Your not allowed a secondary datatype here for the given datatype %s\n", datatype_token->sval);
    }

    if (S_EQ(datatype_token->sval, "void"))
    {
        datatype_out->type = DATA_TYPE_VOID;
        datatype_out->size = DATA_SIZE_ZERO;
    }
    else if (S_EQ(datatype_token->sval, "char"))
    {
        datatype_out->type = DATA_TYPE_CHAR;
        datatype_out->size = DATA_SIZE_BYTE;
    }
    else if (S_EQ(datatype_token->sval, "short"))
    {
        datatype_out->type = DATA_TYPE_SHORT;
        datatype_out->size = DATA_SIZE_WORD;
    }
    else if (S_EQ(datatype_token->sval, "int"))
    {
        datatype_out->type = DATA_TYPE_INTEGER;
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else if (S_EQ(datatype_token->sval, "long"))
    {
        datatype_out->type = DATA_TYPE_LONG;
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else if (S_EQ(datatype_token->sval, "float"))
    {
        datatype_out->type = DATA_TYPE_FLOAT;
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else if (S_EQ(datatype_token->sval, "double"))
    {
        datatype_out->size = DATA_TYPE_DOUBLE;
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else
    {
        compiler_error(current_process, "BUG: Invalid primitive datatype\n");
    }

    parser_datatype_adjust_size_for_secondary(datatype_out, datatype_secondary_token);
}

size_t size_of_struct(const char *struct_name)
{
    struct symbol *sym = symresolver_get_symbol(current_process, struct_name);
    if (!sym)
    {
        return 0;
    }

    assert(sym->type == SYMBOL_TYPE_NODE);
    struct node *node = sym->data;
    assert(node->type == NODE_TYPE_STRUCT);
    return node->_struct.body_n->body.size;
}

size_t size_of_union(const char *union_name)
{
    struct symbol *sym = symresolver_get_symbol(current_process, union_name);
    if (!sym)
    {
        return 0;
    }

    assert(sym->type == SYMBOL_TYPE_NODE);
    struct node *node = sym->data;
    assert(node->type == NODE_TYPE_UNION);
    return node->_union.body_n->body.size;
}

void parser_datatype_init_type_and_size(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out, int pointer_depth, int expected_type)
{
    if (!parser_datatype_is_secondary_allowed(expected_type) && datatype_secondary_token)
    {
        compiler_error(current_process, "You provided an invalid secondary datatype\n");
    }

    switch (expected_type)
    {
    case DATA_TYPE_EXPECT_PRIMITIVE:
        parser_datatype_init_type_and_size_for_primitive(datatype_token, datatype_secondary_token, datatype_out);
        break;

    case DATA_TYPE_EXPECT_STRUCT:
        datatype_out->type = DATA_TYPE_STRUCT;
        datatype_out->size = size_of_struct(datatype_token->sval);
        datatype_out->struct_node = struct_node_for_name(current_process, datatype_token->sval);
        break;
    case DATA_TYPE_EXPECT_UNION:
        datatype_out->type = DATA_TYPE_UNION;
        datatype_out->size = size_of_union(datatype_token->sval);
        datatype_out->struct_node = union_node_for_name(current_process, datatype_token->sval);
        break;

    default:
        compiler_error(current_process, "BUG: Unsupported datatype expectation\n");
    }

    if (pointer_depth > 0)
    {
        datatype_out->flags |= DATATYPE_FLAG_IS_POINTER;
        datatype_out->pointer_depth = pointer_depth;
    }
}

void parser_datatype_init(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out, int pointer_depth, int expected_type)
{
    parser_datatype_init_type_and_size(datatype_token, datatype_secondary_token, datatype_out, pointer_depth, expected_type);
    datatype_out->type_str = datatype_token->sval;

    if (S_EQ(datatype_token->sval, "long") && datatype_secondary_token && S_EQ(datatype_secondary_token->sval, "long"))
    {
        compiler_warning(current_process, "Our compiler does not support 64 bit longs, therefore your long long is defaulting to 32 bits\n");
        datatype_out->size = DATA_SIZE_DWORD;
    }
}
void parse_datatype_type(struct datatype *dtype)
{
    struct token *datatype_token = NULL;
    struct token *datatype_secondary_token = NULL;
    parser_get_datatype_tokens(&datatype_token, &datatype_secondary_token);
    int expected_type = parser_datatype_expected_for_type_string(datatype_token->sval);
    if (datatype_is_struct_or_union_for_name(datatype_token->sval))
    {
        if (token_peek_next()->type == TOKEN_TYPE_IDENTIFIER)
        {
            datatype_token = token_next();
        }
        else
        {
            // This structure has no name, so we need to handle it.
            datatype_token = parser_build_random_type_name();
            dtype->flags |= DATATYPE_FLAG_STRUCT_UNION_NO_NAME;
        }
    }

    // int**
    int pointer_depth = parser_get_pointer_depth();
    parser_datatype_init(datatype_token, datatype_secondary_token, dtype, pointer_depth, expected_type);
}
void parse_datatype(struct datatype *dtype)
{
    memset(dtype, 0, sizeof(struct datatype));
    dtype->flags |= DATATYPE_FLAG_IS_SIGNED;

    parse_datatype_modifiers(dtype);
    parse_datatype_type(dtype);
    parse_datatype_modifiers(dtype);
}

bool parser_is_int_valid_after_datatype(struct datatype *dtype)
{
    return dtype->type == DATA_TYPE_LONG || dtype->type == DATA_TYPE_FLOAT || dtype->type == DATA_TYPE_DOUBLE;
}

/**
 * long int abc;
 *
 */
void parser_ignore_int(struct datatype *dtype)
{
    if (!token_is_keyword(token_peek_next(), "int"))
    {
        // No integer to ignore.
        return;
    }

    if (!parser_is_int_valid_after_datatype(dtype))
    {
        compiler_error(current_process, "You provided a secondary \"int\" type however its not supported with this current abbrevation\n");
    }

    // Ignore the "int" token
    token_next();
}

void parse_expressionable_root(struct history *history)
{
    parse_expressionable(history);
    struct node *result_node = node_pop();
    node_push(result_node);
}

struct datatype_struct_node_fix_private
{
    // Node to fix.
    struct node *node;
};

bool datatype_struct_node_fix(struct fixup *fixup)
{
    struct datatype_struct_node_fix_private *private = fixup_private(fixup);
    struct datatype *dtype = &private->node->var.type;
    dtype->type = DATA_TYPE_STRUCT;
    dtype->size = size_of_struct(dtype->type_str);
    dtype->struct_node = struct_node_for_name(current_process, dtype->type_str);
    if (!dtype->struct_node)
    {
        return false;
    }

    return true;
}

void datatype_struct_node_end(struct fixup *fixup)
{
    free(fixup_private(fixup));
}

void make_variable_node(struct datatype *dtype, struct token *name_token, struct node *value_node)
{
    const char *name_str = NULL;
    if (name_token)
    {
        name_str = name_token->sval;
    }

    node_create(&(struct node){.type = NODE_TYPE_VARIABLE, .var.name = name_str, .var.type = *dtype, .var.val = value_node});
    struct node *var_node = node_peek_or_null();
    if (var_node->var.type.type == DATA_TYPE_STRUCT && !var_node->var.type.struct_node)
    {
        struct datatype_struct_node_fix_private *private = calloc(1, sizeof(struct datatype_struct_node_fix_private));
        private
            ->node = var_node;
        fixup_register(parser_fixup_sys, &(struct fixup_config){.fix = datatype_struct_node_fix, .end = datatype_struct_node_end, .private = private});
    }
}

void parser_scope_offset_for_stack(struct node *node, struct history *history)
{
    struct parser_scope_entity *last_entity = parser_scope_last_entity_stop_global_scope();
    bool upward_stack = history->flags & HISTORY_FLAG_IS_UPWARD_STACK;
    int offset = -variable_size(node);
    if (upward_stack)
    {
        size_t stack_addition = function_node_argument_stack_addition(parser_current_function);
        offset = stack_addition;
        if (last_entity)
        {
            offset = datatype_size(&variable_node(last_entity->node)->var.type);
        }
    }

    if (last_entity)
    {
        offset += variable_node(last_entity->node)->var.aoffset;
        if (variable_node_is_primitive(node))
        {
            variable_node(node)->var.padding = padding(upward_stack ? offset : -offset, node->var.type.size);
        }
    }

    bool first_entity = !last_entity;
    if (node_is_struct_or_union_variable(node) && variable_struct_or_union_body_node(node)->body.padded)
    {
        variable_node(node)->var.padding = padding(upward_stack ? offset : -offset, DATA_SIZE_DWORD);
    }
    variable_node(node)->var.aoffset = offset + (upward_stack ? variable_node(node)->var.padding : -variable_node(node)->var.padding);
}

void parser_scope_offset_for_global(struct node *node, struct history *history)
{
}

void parser_scope_offset_for_structure(struct node *node, struct history *history)
{
    int offset = 0;
    struct parser_scope_entity *last_entity = parser_scope_last_entity();
    if (last_entity)
    {
        offset += last_entity->stack_offset + last_entity->node->var.type.size;
        if (variable_node_is_primitive(node))
        {
            node->var.padding = padding(offset, node->var.type.size);
        }

        node->var.aoffset = offset + node->var.padding;
    }
}

void parser_scope_offset(struct node *node, struct history *history)
{
    if (history->flags & HISTORY_FLAG_IS_GLOBAL_SCOPE)
    {
        parser_scope_offset_for_global(node, history);
        return;
    }

    if (history->flags & HISTORY_FLAG_INSIDE_STRUCTURE)
    {
        parser_scope_offset_for_structure(node, history);
        return;
    }

    parser_scope_offset_for_stack(node, history);
}

void make_variable_node_and_register(struct history *history, struct datatype *dtype, struct token *name_token, struct node *value_node)
{
    make_variable_node(dtype, name_token, value_node);
    struct node *var_node = node_pop();

    // Calculate the scope offset
    parser_scope_offset(var_node, history);
    // Push the variable node to the scope
    parser_scope_push(parser_new_scope_entity(var_node, var_node->var.aoffset, 0), var_node->var.type.size);

    resolver_default_new_scope_entity(current_process->resolver, var_node, var_node->var.aoffset, 0);
    node_push(var_node);
}

void make_variable_list_node(struct vector *var_list_vec)
{
    node_create(&(struct node){.type = NODE_TYPE_VARIABLE_LIST, .var_list.list = var_list_vec});
}

struct array_brackets *parse_array_brackets(struct history *history)
{
    struct array_brackets *brackets = array_brackets_new();
    while (token_next_is_operator("["))
    {
        expect_op("[");
        if (token_is_symbol(token_peek_next(), ']'))
        {
            // Nothing between the brackets
            expect_sym(']');
            break;
        }

        parse_expressionable_root(history);
        expect_sym(']');

        struct node *exp_node = node_pop();
        make_bracket_node(exp_node);

        struct node *bracket_node = node_pop();
        array_brackets_add(brackets, bracket_node);
    }

    return brackets;
}

void parse_variable(struct datatype *dtype, struct token *name_token, struct history *history)
{
    struct node *value_node = NULL;
    // int a; int b[30];
    // Check for array brackets.
    struct array_brackets *brackets = NULL;
    if (token_next_is_operator("["))
    {
        brackets = parse_array_brackets(history);
        dtype->array.brackets = brackets;
        dtype->array.size = array_brackets_calculate_size(dtype, brackets);
        dtype->flags |= DATATYPE_FLAG_IS_ARRAY;
    }

    // int c = 50;
    if (token_next_is_operator("="))
    {
        // Ignore the = operator
        token_next();
        parse_expressionable_root(history);
        value_node = node_pop();
    }

    make_variable_node_and_register(history, dtype, name_token, value_node);
}

void parse_function_body(struct history *history)
{
    parse_body(NULL, history_down(history, history->flags | HISTORY_FLAG_INSIDE_FUNCTION_BODY));
}

void parse_function(struct datatype *ret_type, struct token *name_token, struct history *history)
{
    struct vector *arguments_vector = NULL;
    parser_scope_new();
    resolver_default_new_scope(current_process->resolver, 0);
    make_function_node(ret_type, name_token->sval, NULL, NULL);
    struct node *function_node = node_peek();
    parser_current_function = function_node;
    if (datatype_is_struct_or_union(ret_type))
    {
        function_node->func.args.stack_addition += DATA_SIZE_DWORD;
    }

    expect_op("(");
    arguments_vector = parse_function_arguments(history_begin(0));
    expect_sym(')');

    function_node->func.args.vector = arguments_vector;
    if (symresolver_get_symbol_for_native_function(current_process, name_token->sval))
    {
        function_node->func.flags |= FUNCTION_NODE_FLAG_IS_NATIVE;
    }

    if (token_next_is_symbol('{'))
    {
        parse_function_body(history_begin(0));
        struct node *body_node = node_pop();
        function_node->func.body_n = body_node;
    }
    else
    {
        expect_sym(';');
    }

    parser_current_function = NULL;
    resolver_default_finish_scope(current_process->resolver);
    parser_scope_finish();
}

void parse_symbol()
{
    if (token_next_is_symbol('{'))
    {
        size_t variable_size = 0;
        struct history *history = history_begin(HISTORY_FLAG_IS_GLOBAL_SCOPE);
        parse_body(&variable_size, history);
        struct node *body_node = node_pop();

        node_push(body_node);
    }
    else if (token_next_is_symbol(':'))
    {
        parse_label(history_begin(0));
        return;
    }

    compiler_error(current_process, "Invalid symbol was provided");
}

void parse_statement(struct history *history)
{
    if (token_peek_next()->type == TOKEN_TYPE_KEYWORD)
    {
        parse_keyword(history);
        return;
    }

    parse_expressionable_root(history);
    if (token_peek_next()->type == TOKEN_TYPE_SYMBOL && !token_is_symbol(token_peek_next(), ';'))
    {
        parse_symbol();
        return;
    }

    // All statements end with semicolons;
    expect_sym(';');
}

void parser_append_size_for_node_struct_union(struct history *history, size_t *_variable_size, struct node *node)
{
    *_variable_size += variable_size(node);
    if (node->var.type.flags & DATATYPE_FLAG_IS_POINTER)
    {
        return;
    }

    struct node *largest_var_node = variable_struct_or_union_body_node(node)->body.largest_var_node;
    if (largest_var_node)
    {
        *_variable_size += align_value(*_variable_size, largest_var_node->var.type.size);
    }
}

void parser_append_size_for_node(struct history *history, size_t *_variable_size, struct node *node);

void parser_append_size_for_variable_list(struct history *history, size_t *variable_size, struct vector *vec)
{
    vector_set_peek_pointer(vec, 0);
    struct node *node = vector_peek_ptr(vec);
    while (node)
    {
        parser_append_size_for_node(history, variable_size, node);
        node = vector_peek_ptr(vec);
    }
}

void parser_append_size_for_node(struct history *history, size_t *_variable_size, struct node *node)
{
    if (!node)
    {
        return;
    }

    if (node->type == NODE_TYPE_VARIABLE)
    {
        if (node_is_struct_or_union_variable(node))
        {
            parser_append_size_for_node_struct_union(history, _variable_size, node);
            return;
        }

        *_variable_size += variable_size(node);
    }
    else if (node->type == NODE_TYPE_VARIABLE_LIST)
    {
        parser_append_size_for_variable_list(history, _variable_size, node->var_list.list);
    }
}

void parser_finalize_body(struct history *history, struct node *body_node, struct vector *body_vec, size_t *_variable_size, struct node *largest_align_eligible_var_node, struct node *largest_possible_var_node)
{
    if (history->flags & HISTORY_FLAG_INSIDE_UNION)
    {
        if (largest_possible_var_node)
        {
            *_variable_size = variable_size(largest_possible_var_node);
        }
    }
    int padding = compute_sum_padding(body_vec);
    *_variable_size += padding;

    if (largest_align_eligible_var_node)
    {
        *_variable_size = align_value(*_variable_size, largest_align_eligible_var_node->var.type.size);
    }

    bool padded = padding != 0;
    body_node->body.largest_var_node = largest_align_eligible_var_node;
    body_node->body.padded = padded;
    body_node->body.size = *_variable_size;
    body_node->body.statements = body_vec;
}

void parse_body_single_statement(size_t *variable_size, struct vector *body_vec, struct history *history)
{
    make_body_node(NULL, 0, false, NULL);
    struct node *body_node = node_pop();
    body_node->binded.owner = parser_current_body;
    parser_current_body = body_node;
    struct node *stmt_node = NULL;
    parse_statement(history_down(history, history->flags));
    stmt_node = node_pop();
    vector_push(body_vec, &stmt_node);

    // Change the variable_size variable by the size of stmt_node.
    parser_append_size_for_node(history, variable_size, stmt_node);
    struct node *largest_var_node = NULL;
    if (stmt_node->type == NODE_TYPE_VARIABLE)
    {
        largest_var_node = stmt_node;
    }

    parser_finalize_body(history, body_node, body_vec, variable_size, largest_var_node, largest_var_node);
    parser_current_body = body_node->binded.owner;

    node_push(body_node);
}

void parse_body_multiple_statements(size_t *variable_size, struct vector *body_vec, struct history *history)
{
    // Create a blank body node
    make_body_node(NULL, 0, false, NULL);
    struct node *body_node = node_pop();
    body_node->binded.owner = parser_current_body;
    parser_current_body = body_node;

    struct node *stmt_node = NULL;
    struct node *largest_possible_var_node = NULL;
    struct node *largest_align_eligible_var_node = NULL;

    // We have a body i.e { } therefore we must pop off the left curly brace..
    expect_sym('{');

    while (!token_next_is_symbol('}'))
    {
        parse_statement(history_down(history, history->flags));
        stmt_node = node_pop();
        if (stmt_node->type == NODE_TYPE_VARIABLE)
        {
            if (!largest_possible_var_node ||
                (largest_possible_var_node->var.type.size <= stmt_node->var.type.size))
            {
                largest_possible_var_node = stmt_node;
            }

            if (variable_node_is_primitive(stmt_node))
            {
                if (!largest_align_eligible_var_node ||
                    (largest_align_eligible_var_node->var.type.size <= stmt_node->var.type.size))
                {
                    largest_align_eligible_var_node = stmt_node;
                }
            }
        }

        // Push the statement node to the body vector
        vector_push(body_vec, &stmt_node);

        // We may have to change the variable size if this statement is a variable
        parser_append_size_for_node(history, variable_size, variable_node_or_list(stmt_node));
    }

    // Pop off the right curly brace
    expect_sym('}');

    parser_finalize_body(history, body_node, body_vec, variable_size, largest_align_eligible_var_node, largest_possible_var_node);
    parser_current_body = body_node->binded.owner;

    // Let's now push the body node back to the stack :)
    node_push(body_node);
}

/**
 * @brief
 *
 * @param variable_size Set to the sum of all variable sizes encountered in the parsed body
 * @param history
 */
void parse_body(size_t *variable_size, struct history *history)
{
    parser_scope_new();
    resolver_default_new_scope(current_process->resolver, 0);
    size_t tmp_size = 0x00;
    if (!variable_size)
    {
        variable_size = &tmp_size;
    }

    struct vector *body_vec = vector_create(sizeof(struct node *));
    if (!token_next_is_symbol('{'))
    {
        parse_body_single_statement(variable_size, body_vec, history);
        resolver_default_finish_scope(current_process->resolver);
        parser_scope_finish();
        return;
    }

    // We have some statements between curly braces { int a; int b; int c; }
    parse_body_multiple_statements(variable_size, body_vec, history);
    resolver_default_finish_scope(current_process->resolver);
    parser_scope_finish();

    if (variable_size)
    {
        if (history->flags & HISTORY_FLAG_INSIDE_FUNCTION_BODY)
        {
            parser_current_function->func.stack_size += *variable_size;
        }
    }
}

void parse_struct_no_new_scope(struct datatype *dtype, bool is_forward_declaration)
{
    struct node *body_node = NULL;
    size_t body_variable_size = 0;

    if (!is_forward_declaration)
    {
        parse_body(&body_variable_size, history_begin(HISTORY_FLAG_INSIDE_STRUCTURE));
        body_node = node_pop();
    }

    make_struct_node(dtype->type_str, body_node);
    struct node *struct_node = node_pop();
    if (body_node)
    {
        dtype->size = body_node->body.size;
    }
    dtype->struct_node = struct_node;

    if (token_is_identifier(token_peek_next()))
    {
        struct token *var_name = token_next();
        struct_node->flags |= NODE_FLAG_HAS_VARIABLE_COMBINED;
        if (dtype->flags & DATATYPE_FLAG_STRUCT_UNION_NO_NAME)
        {
            dtype->type_str = var_name->sval;
            dtype->flags &= ~DATATYPE_FLAG_STRUCT_UNION_NO_NAME;
            struct_node->_struct.name = var_name->sval;
        }

        make_variable_node_and_register(history_begin(0), dtype, var_name, NULL);
        struct_node->_struct.var = node_pop();
    }

    // All structures end with semicolons.
    expect_sym(';');

    // We are done creating the structure
    node_push(struct_node);
}

void parse_union_no_scope(struct datatype *dtype, bool is_forward_declaration)
{
    struct node *body_node = NULL;
    size_t body_variable_size = 0;
    if (!is_forward_declaration)
    {
        parse_body(&body_variable_size, history_begin(HISTORY_FLAG_INSIDE_UNION));
        body_node = node_pop();
    }

    make_union_node(dtype->type_str, body_node);
    struct node *union_node = node_pop();
    if (body_node)
    {
        dtype->size = body_node->body.size;
    }
    if (token_peek_next()->type == TOKEN_TYPE_IDENTIFIER)
    {
        struct token *var_name = token_next();
        union_node->flags |= NODE_FLAG_HAS_VARIABLE_COMBINED;
        make_variable_node_and_register(history_begin(0), dtype, var_name, NULL);
        union_node->_union.var = node_pop();
    }

    expect_sym(';');
    node_push(union_node);
}

void parse_union(struct datatype *dtype)
{
    bool is_forward_declaration = !token_is_symbol(token_peek_next(), '{');
    if (!is_forward_declaration)
    {
        parser_scope_new();
        resolver_default_new_scope(current_process->resolver, 0);
    }
    parse_union_no_scope(dtype, is_forward_declaration);

    if (!is_forward_declaration)
    {
        resolver_default_finish_scope(current_process->resolver);
        parser_scope_finish();
    }
}

void parse_struct(struct datatype *dtype)
{
    bool is_forward_declaration = !token_is_symbol(token_peek_next(), '{');
    if (!is_forward_declaration)
    {
        parser_scope_new();
        resolver_default_new_scope(current_process->resolver, 0);
    }
    parse_struct_no_new_scope(dtype, is_forward_declaration);

    if (!is_forward_declaration)
    {
        resolver_default_finish_scope(current_process->resolver);
        parser_scope_finish();
    }
}
void parse_struct_or_union(struct datatype *dtype)
{
    switch (dtype->type)
    {
    case DATA_TYPE_STRUCT:
        parse_struct(dtype);
        break;

    case DATA_TYPE_UNION:
        parse_union(dtype);
        break;

    default:
        compiler_error(current_process, "COMPILER BUG: The provided datatype is not a structure or union\n");
    }
}

void token_read_dots(size_t amount)
{
    for (size_t i = 0; i < amount; i++)
    {
        expect_op(".");
    }
}

void parse_variable_full(struct history *history)
{
    struct datatype dtype;
    parse_datatype(&dtype);

    struct token *name_token = NULL;
    if (token_peek_next()->type == TOKEN_TYPE_IDENTIFIER)
    {
        name_token = token_next();
    }
    parse_variable(&dtype, name_token, history);
}

struct vector *parse_function_arguments(struct history *history)
{
    parser_scope_new();
    struct vector *arguments_vec = vector_create(sizeof(struct node *));
    while (!token_next_is_symbol(')'))
    {
        if (token_next_is_operator("."))
        {
            token_read_dots(3);
            parser_scope_finish();
            return arguments_vec;
        }

        parse_variable_full(history_down(history, history->flags | HISTORY_FLAG_IS_UPWARD_STACK));
        struct node *argument_node = node_pop();
        vector_push(arguments_vec, &argument_node);

        if (!token_next_is_operator(","))
        {
            break;
        }

        // Pop off the comma.
        token_next();
    }

    parser_scope_finish();
    return arguments_vec;
}

void parse_forward_declaration(struct datatype *dtype)
{
    // Since this is a forward declaration, parse the structure
    parse_struct(dtype);
}
void parse_variable_function_or_struct_union(struct history *history)
{
    struct datatype dtype;
    parse_datatype(&dtype);

    if (datatype_is_struct_or_union(&dtype) && token_next_is_symbol('{'))
    {
        parse_struct_or_union(&dtype);

        struct node *su_node = node_pop();
        symresolver_build_for_node(current_process, su_node);
        node_push(su_node);
        return;
    }

    if (token_next_is_symbol(';'))
    {
        parse_forward_declaration(&dtype);
        return;
    }

    // Ignore integer abbrevations if neccesary i.e "long int" becomes just "long"
    parser_ignore_int(&dtype);

    // int abc;
    struct token *name_token = token_next();
    if (name_token->type != TOKEN_TYPE_IDENTIFIER)
    {
        compiler_error(current_process, "Expecting a valid name for the given variable declaration\n");
    }

    // int abc()
    // Check if this is a function declaration
    if (token_next_is_operator("("))
    {
        parse_function(&dtype, name_token, history);
        return;
    }

    parse_variable(&dtype, name_token, history);
    if (token_is_operator(token_peek_next(), ","))
    {
        struct vector *var_list = vector_create(sizeof(struct node *));
        // Pop off the original variable
        struct node *var_node = node_pop();
        vector_push(var_list, &var_node);
        while (token_is_operator(token_peek_next(), ","))
        {
            // Get rid of the comma
            token_next();
            name_token = token_next();
            parse_variable(&dtype, name_token, history);
            var_node = node_pop();
            vector_push(var_list, &var_node);
        }

        make_variable_list_node(var_list);
    }

    expect_sym(';');
}

void parse_if_stmt(struct history *history);

struct node *parse_else(struct history *history)
{
    size_t var_size = 0;
    parse_body(&var_size, history);
    struct node *body_node = node_pop();
    make_else_node(body_node);
    return node_pop();
}

struct node *parse_else_or_else_if(struct history *history)
{
    struct node *node = NULL;
    if (token_next_is_keyword("else"))
    {
        // We have an else or an else if
        // pop off "else"
        token_next();

        if (token_next_is_keyword("if"))
        {
            // Okay this is an else if not an else
            parse_if_stmt(history_down(history, 0));
            node = node_pop();
            return node;
        }

        // Its an else statement
        node = parse_else(history_down(history, 0));
    }
    return node;
}

void parse_if_stmt(struct history *history)
{
    expect_keyword("if");
    expect_op("(");
    // Cond
    parse_expressionable_root(history);
    expect_sym(')');

    struct node *cond_node = node_pop();
    size_t var_size = 0;
    // if(0) { }
    parse_body(&var_size, history);
    struct node *body_node = node_pop();
    make_if_node(cond_node, body_node, parse_else_or_else_if(history));
}

void parse_keyword_parentheses_expression(const char *keyword)
{
    expect_keyword(keyword);
    expect_op("(");
    parse_expressionable_root(history_begin(0));
    expect_sym(')');
}

void parse_default(struct history *history)
{
    expect_keyword("default");
    expect_sym(':');
    make_default_node();
    history->_switch.case_data->has_default_case = true;
}
void parse_case(struct history *history)
{
    expect_keyword("case");
    parse_expressionable_root(history);
    struct node *case_exp_node = node_pop();
    expect_sym(':');
    make_case_node(case_exp_node);

    if (case_exp_node->type != NODE_TYPE_NUMBER)
    {
        compiler_error(current_process, "We only support numbers in our subset of C at this time\n");
    }

    struct node *case_node = node_peek();
    parser_register_case(history, case_node);
}
void parse_switch(struct history *history)
{
    struct parser_history_switch _switch = parser_new_switch_statement(history);
    parse_keyword_parentheses_expression("switch");
    struct node *switch_exp_node = node_pop();
    size_t variable_size = 0;
    parse_body(&variable_size, history);
    struct node *body_node = node_pop();
    // Make the switch node
    make_switch_node(switch_exp_node, body_node, _switch.case_data->cases, _switch.case_data->has_default_case);
    parser_end_switch_statement(&_switch);
}

void parse_do_while(struct history *history)
{
    expect_keyword("do");
    size_t var_size = 0;
    parse_body(&var_size, history);
    struct node *body_node = node_pop();
    parse_keyword_parentheses_expression("while");
    struct node *exp_node = node_pop();
    expect_sym(';');

    make_do_while_node(body_node, exp_node);
}
void parse_while(struct history *history)
{
    parse_keyword_parentheses_expression("while");
    struct node *exp_node = node_pop();
    size_t variable_size = 0;
    parse_body(&variable_size, history);
    struct node *body_node = node_pop();
    make_while_node(exp_node, body_node);
}

bool parse_for_loop_part(struct history *history)
{
    if (token_next_is_symbol(';'))
    {
        // We have nothing here i.e "for (;"
        // Ignore the semicolon
        token_next();
        return false;
    }

    parse_expressionable_root(history);

    expect_sym(';');
    return true;
}

bool parse_for_loop_part_loop(struct history *history)
{
    if (token_next_is_symbol(')'))
    {
        return false;
    }

    parse_expressionable_root(history);
    return true;
}

void parse_for_stmt(struct history *history)
{
    struct node *init_node = NULL;
    struct node *cond_node = NULL;
    struct node *loop_node = NULL;
    struct node *body_node = NULL;

    expect_keyword("for");
    expect_op("(");
    if (parse_for_loop_part(history))
    {
        init_node = node_pop();
    }

    if (parse_for_loop_part(history))
    {
        cond_node = node_pop();
    }

    if (parse_for_loop_part_loop(history))
    {
        loop_node = node_pop();
    }

    expect_sym(')');

    size_t variable_size = 0;
    parse_body(&variable_size, history);
    body_node = node_pop();
    make_for_node(init_node, cond_node, loop_node, body_node);
}

void parse_return(struct history *history)
{
    expect_keyword("return");

    // For returns with no expressions
    if (token_next_is_symbol(';'))
    {
        expect_sym(';');
        make_return_node(NULL);
        return;
    }

    // Expressionable return i.e return 50;
    parse_expressionable_root(history);
    struct node *exp_node = node_pop();
    make_return_node(exp_node);
    expect_sym(';');
}

void parse_continue(struct history *history)
{
    expect_keyword("continue");
    expect_sym(';');
    make_continue_node();
}

void parse_break(struct history *history)
{
    expect_keyword("break");
    expect_sym(';');
    make_break_node();
}

void parse_goto(struct history *history)
{
    expect_keyword("goto");
    parse_identifier(history_begin(0));
    expect_sym(';');

    struct node *label_node = node_pop();
    make_goto_node(label_node);
}
void parse_label(struct history *history)
{
    expect_sym(':');

    struct node *label_name_node = node_pop();
    if (label_name_node->type != NODE_TYPE_IDENTIFIER)
    {
        compiler_error(current_process, "Expecting an identifier for labels something else was provided");
    }

    make_label_node(label_name_node);
}

void parse_for_tenary(struct history *history)
{
    struct node *condition_node = node_pop();
    expect_op("?");
    parse_expressionable_root(history_down(history, HISTORY_FLAG_PARENTHESES_IS_NOT_A_FUNCTION_CALL));
    struct node *true_result_node = node_pop();
    expect_sym(':');
    parse_expressionable_root(history_down(history, HISTORY_FLAG_PARENTHESES_IS_NOT_A_FUNCTION_CALL));
    struct node *false_result_node = node_pop();
    make_tenary_node(true_result_node, false_result_node);
    struct node *tenary_node = node_pop();
    make_exp_node(condition_node, tenary_node, "?");
}

void parse_sizeof(struct history* history)
{
    expect_keyword("sizeof");
    expect_op("(");
    struct datatype dtype;
    parse_datatype(&dtype);
    node_create(&(struct node){NODE_TYPE_NUMBER,.llnum=datatype_size(&dtype)});
    expect_sym(')');
}

void parse_keyword(struct history *history)
{
    struct token *token = token_peek_next();
    if (S_EQ(token->sval, "sizeof"))
    {
        parse_sizeof(history);
        return;
    }
    if (is_keyword_variable_modifier(token->sval) || keyword_is_datatype(token->sval))
    {
        parse_variable_function_or_struct_union(history);
        return;
    }

    if (S_EQ(token->sval, "break"))
    {
        parse_break(history);
        return;
    }
    else if (S_EQ(token->sval, "continue"))
    {
        parse_continue(history);
        return;
    }
    else if (S_EQ(token->sval, "return"))
    {
        parse_return(history);
        return;
    }
    else if (S_EQ(token->sval, "if"))
    {
        parse_if_stmt(history);
        return;
    }
    else if (S_EQ(token->sval, "for"))
    {
        parse_for_stmt(history);
        return;
    }
    else if (S_EQ(token->sval, "while"))
    {
        parse_while(history);
        return;
    }
    else if (S_EQ(token->sval, "do"))
    {
        parse_do_while(history);
        return;
    }
    else if (S_EQ(token->sval, "switch"))
    {
        parse_switch(history);
        return;
    }
    else if (S_EQ(token->sval, "goto"))
    {
        parse_goto(history);
        return;
    }
    else if (S_EQ(token->sval, "case"))
    {
        parse_case(history);
        return;
    }
    else if (S_EQ(token->sval, "default"))
    {
        parse_default(history);
        return;
    }

    compiler_error(current_process, "Invalid keyword\n");
}

void parse_string(struct history *history)
{
    parse_single_token_to_node();
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
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        parse_single_token_to_node();
        res = 0;
        break;

    case TOKEN_TYPE_IDENTIFIER:
        parse_identifier(history);
        res = 0;
        break;

    case TOKEN_TYPE_OPERATOR:
        res = parse_exp(history);
        break;

    case TOKEN_TYPE_KEYWORD:
        parse_keyword(history);
        res = 0;
        break;

    case TOKEN_TYPE_STRING:
        parse_string(history);
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

void parse_keyword_for_global()
{
    parse_keyword(history_begin(HISTORY_FLAG_IS_GLOBAL_SCOPE));
    struct node *node = node_pop();
    switch (node->type)
    {
    case NODE_TYPE_VARIABLE:
    case NODE_TYPE_FUNCTION:
    case NODE_TYPE_STRUCT:
    case NODE_TYPE_UNION:
        symresolver_build_for_node(current_process, node);
        break;
    }
    node_push(node);
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

    case TOKEN_TYPE_KEYWORD:
        parse_keyword_for_global();
        break;

    case TOKEN_TYPE_SYMBOL:
        parse_symbol();
        break;
    }
    return 0;
}

int parse(struct compile_process *process)
{
    scope_create_root(process);
    current_process = process;
    parser_last_token = NULL;
    node_set_vector(process->node_vec, process->node_tree_vec);
    parser_blank_node = node_create(&(struct node){.type = NODE_TYPE_BLANK});
    parser_fixup_sys = fixup_sys_new();

    struct node *node = NULL;
    vector_set_peek_pointer(process->token_vec, 0);
    while (parse_next() == 0)
    {
        node = node_peek();
        vector_push(process->node_tree_vec, &node);
    }

    assert(fixups_resolve(parser_fixup_sys));
    scope_free_root(process);

    return PARSE_ALL_OK;
}