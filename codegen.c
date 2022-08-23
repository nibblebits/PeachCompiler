#include "compiler.h"
#include "helpers/vector.h"
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
static struct compile_process *current_process = NULL;
static struct node *current_function = NULL;

enum
{
    RESPONSE_FLAG_ACKNOWLEDGED = 0b00000001,
    RESPONSE_FLAG_PUSHED_STRUCTURE = 0b00000010,
    RESPONSE_FLAG_RESOLVED_ENTITY = 0b00000100,
    RESPONSE_FLAG_UNARY_GET_ADDRESS = 0b00001000,
};

#define RESPONSE_SET(x) &(struct response{x})
#define RESPONSE_EMPTY_RESPONSE_SET()

struct response_data
{
    union
    {
        struct resolver_entity* resolved_entity;
    };
    
};

struct response
{
    int flags;
    struct response_data data;
};

void codegen_response_expect()
{
    struct response* res = calloc(1, sizeof(struct response));
    vector_push(current_process->generator->responses, &res);
}

struct response_data* codegen_response_data(struct response* response)
{
    return &response->data;
}

struct response* codegen_response_pull()
{
    struct response* res = vector_back_ptr_or_null(current_process->generator->responses);
    if (res)
    {
        vector_pop(current_process->generator->responses);
    }
    return res;
}

void codegen_response_acknowledge(struct response* response_in)
{
    struct response* res = vector_back_ptr_or_null(current_process->generator->responses);
    if (res)
    {
        res->flags |= response_in->flags;
        if (response_in->data.resolved_entity)
        {
            res->data.resolved_entity = response_in->data.resolved_entity;
        }
        res->flags |= RESPONSE_FLAG_ACKNOWLEDGED;
    }
}

bool codegen_response_acknowledged(struct response* res)
{
    return res && res->flags && RESPONSE_FLAG_ACKNOWLEDGED;
}

bool codegen_response_has_entity(struct response* res)
{
    return codegen_response_acknowledged(res) && res->flags & RESPONSE_FLAG_RESOLVED_ENTITY && res->data.resolved_entity;
}

struct history
{
    int flags;
};

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

void codegen_generate_exp_node(struct node *node, struct history *history);
const char *codegen_sub_register(const char *original_register, size_t size);

void codegen_new_scope(int flags)
{
    resolver_default_new_scope(current_process->resolver, flags);
}

void codegen_finish_scope()
{
    resolver_default_finish_scope(current_process->resolver);
}

struct node *codegen_node_next()
{
    return vector_peek_ptr(current_process->node_tree_vec);
}

struct resolver_default_entity_data *codegen_entity_private(struct resolver_entity *entity)
{
    return resolver_default_entity_private(entity);
}

void asm_push_args(const char *ins, va_list args)
{
    va_list args2;
    va_copy(args2, args);
    vfprintf(stdout, ins, args);
    fprintf(stdout, "\n");
    if (current_process->ofile)
    {
        vfprintf(current_process->ofile, ins, args2);
        fprintf(current_process->ofile, "\n");
    }
}

void asm_push(const char *ins, ...)
{
    va_list args;
    va_start(args, ins);
    asm_push_args(ins, args);
    va_end(args);
}

void asm_push_no_nl(const char *ins, ...)
{
    va_list args;
    va_start(args, ins);
    vfprintf(stdout, ins, args);
    va_end(args);

    if (current_process->ofile)
    {
        va_list args;
        va_start(args, ins);
        vfprintf(current_process->ofile, ins, args);
        va_end(args);
    }
}

void asm_push_ins_push(const char *fmt, int stack_entity_type, const char *stack_entity_name, ...)
{
    char tmp_buf[200];
    sprintf(tmp_buf, "push %s", fmt);
    va_list args;
    va_start(args, stack_entity_name);
    asm_push_args(tmp_buf, args);
    va_end(args);

    assert(current_function);
    stackframe_push(current_function, &(struct stack_frame_element){.type = stack_entity_type, .name = stack_entity_name});
}

int asm_push_ins_pop(const char *fmt, int expecting_stack_entity_type, const char *expecting_stack_entity_name, ...)
{
    char tmp_buf[200];
    sprintf(tmp_buf, "pop %s", fmt);
    va_list args;
    va_start(args, expecting_stack_entity_name);
    asm_push_args(tmp_buf, args);
    va_end(args);

    assert(current_function);
    struct stack_frame_element *element = stackframe_back(current_function);
    int flags = element->flags;
    stackframe_pop_expecting(current_function, expecting_stack_entity_type, expecting_stack_entity_name);
    return flags;
}

void asm_push_ins_push_with_data(const char *fmt, int stack_entity_type, const char *stack_entity_name, int flags, struct stack_frame_data *data, ...)
{
    char tmp_buf[200];
    sprintf(tmp_buf, "push %s", fmt);
    va_list args;
    va_start(args, data);
    asm_push_args(tmp_buf, args);
    va_end(args);

    flags |= STACK_FRAME_ELEMENT_FLAG_HAS_DATATYPE;
    assert(current_function);
    stackframe_push(current_function, &(struct stack_frame_element){.type = stack_entity_type, .name = stack_entity_name, .flags = flags, .data = *data});
}
void asm_push_ebp()
{
    asm_push_ins_push("ebp", STACK_FRAME_ELEMENT_TYPE_SAVED_BP, "function_entry_saved_ebp");
}

void asm_pop_ebp()
{
    asm_push_ins_pop("ebp", STACK_FRAME_ELEMENT_TYPE_SAVED_BP, "function_entry_saved_ebp");
}

void codegen_stack_sub_with_name(size_t stack_size, const char *name)
{
    if (stack_size != 0)
    {
        stackframe_sub(current_function, STACK_FRAME_ELEMENT_TYPE_UNKNOWN, name, stack_size);
        asm_push("sub esp, %lld", stack_size);
    }
}
void codegen_stack_sub(size_t stack_size)
{
    codegen_stack_sub_with_name(stack_size, "literal_stack_change");
}

void codegen_stack_add_with_name(size_t stack_size, const char *name)
{
    if (stack_size != 0)
    {
        stackframe_add(current_function, STACK_FRAME_ELEMENT_TYPE_UNKNOWN, name, stack_size);
        asm_push("add esp, %lld", stack_size);
    }
}

void codegen_stack_add(size_t stack_size)
{
    codegen_stack_add_with_name(stack_size, "literal_stack_change");
}

struct resolver_entity *codegen_new_scope_entity(struct node *var_node, int offset, int flags)
{
    return resolver_default_new_scope_entity(current_process->resolver, var_node, offset, flags);
}

const char *codegen_get_label_for_string(const char *str)
{
    const char *result = NULL;
    struct code_generator *generator = current_process->generator;
    vector_set_peek_pointer(generator->string_table, 0);
    struct string_table_element *current = vector_peek_ptr(generator->string_table);
    while (current)
    {
        if (S_EQ(current->str, str))
        {
            result = current->label;
            break;
        }

        current = vector_peek_ptr(generator->string_table);
    }

    return result;
}

const char *codegen_register_string(const char *str)
{
    const char *label = codegen_get_label_for_string(str);
    if (label)
    {
        // We already registered this string, just return the label to the string memory.
        return label;
    }

    struct string_table_element *str_elem = calloc(1, sizeof(struct string_table_element));
    int label_id = codegen_label_count();
    sprintf((char *)str_elem->label, "str_%i", label_id);
    str_elem->str = str;
    vector_push(current_process->generator->string_table, &str_elem);
    return str_elem->label;
}

struct code_generator *codegenerator_new(struct compile_process *process)
{
    struct code_generator *generator = calloc(1, sizeof(struct code_generator));
    generator->string_table = vector_create(sizeof(struct string_table_element *));
    generator->entry_points = vector_create(sizeof(struct codegen_entry_point *));
    generator->exit_points = vector_create(sizeof(struct codegen_exit_point *));
    generator->responses = vector_create(sizeof(struct response*));
    return generator;
}

void codegen_register_exit_point(int exit_point_id)
{
    struct code_generator *gen = current_process->generator;
    struct codegen_exit_point *exit_point = calloc(1, sizeof(struct codegen_exit_point));
    exit_point->id = exit_point_id;
    vector_push(gen->exit_points, &exit_point);
}

struct codegen_exit_point *codegen_current_exit_point()
{
    struct code_generator *gen = current_process->generator;
    return vector_back_ptr_or_null(gen->exit_points);
}

int codegen_label_count()
{
    static int count = 0;
    count++;
    return count;
}
void codegen_begin_exit_point()
{
    int exit_point_id = codegen_label_count();
    codegen_register_exit_point(exit_point_id);
}

void codegen_end_exit_point()
{
    struct code_generator *gen = current_process->generator;
    struct codegen_exit_point *exit_point = codegen_current_exit_point();
    assert(exit_point);
    asm_push(".exit_point_%i:", exit_point->id);
    free(exit_point);
    vector_pop(gen->exit_points);
}

void codegen_goto_exit_point(struct node *node)
{
    struct code_generator *gen = current_process->generator;
    struct codegen_exit_point *exit_point = codegen_current_exit_point();
    asm_push("jmp .exit_point_%i", exit_point->id);
}

void codegen_register_entry_point(int entry_point_id)
{
    struct code_generator *gen = current_process->generator;
    struct codegen_entry_point *entry_point = calloc(1, sizeof(struct codegen_entry_point));
    entry_point->id = entry_point_id;
    vector_push(gen->entry_points, &entry_point);
}

struct codegen_entry_point *codegen_current_entry_point()
{
    struct code_generator *gen = current_process->generator;
    return vector_back_ptr_or_null(gen->entry_points);
}

void codegen_begin_entry_point()
{
    int entry_point_id = codegen_label_count();
    codegen_register_entry_point(entry_point_id);
    asm_push(".entry_point_%i:", entry_point_id);
}

void codegen_end_entry_point()
{
    struct code_generator *gen = current_process->generator;
    struct codegen_entry_point *entry_point = codegen_current_entry_point();
    assert(entry_point);
    free(entry_point);
    vector_pop(gen->entry_points);
}

void codegen_goto_entry_point(struct node *current_node)
{
    struct code_generator *gen = current_process->generator;
    struct codegen_entry_point *entry_point = codegen_current_entry_point();
    asm_push("jmp .entry_point_%i", entry_point->id);
}

void codegen_begin_entry_exit_point()
{
    codegen_begin_entry_point();
    codegen_begin_exit_point();
}

void codegen_end_entry_exit_point()
{
    codegen_end_entry_point();
    codegen_end_exit_point();
}

static const char *asm_keyword_for_size(size_t size, char *tmp_buf)
{
    const char *keyword = NULL;
    switch (size)
    {
    case DATA_SIZE_BYTE:
        keyword = "db";
        break;
    case DATA_SIZE_WORD:
        keyword = "dw";
        break;
    case DATA_SIZE_DWORD:
        keyword = "dd";
        break;

    case DATA_SIZE_DDWORD:
        keyword = "dq";
        break;

    default:
        sprintf(tmp_buf, "times %lld db ", (unsigned long)size);
        return tmp_buf;
    }

    strcpy(tmp_buf, keyword);
    return tmp_buf;
}

void codegen_generate_global_variable_for_primitive(struct node *node)
{
    char tmp_buf[256];
    if (node->var.val != NULL)
    {
        // Handle the value.
        if (node->var.val->type == NODE_TYPE_STRING)
        {
            const char *label = codegen_register_string(node->var.val->sval);
            asm_push("%s: %s %s", node->var.name, asm_keyword_for_size(variable_size(node), tmp_buf), label);
        }
        else
        {
            asm_push("%s: %s %lld", node->var.name, asm_keyword_for_size(variable_size(node), tmp_buf), node->var.val->llnum);
        }
    }
    else
    {
        asm_push("%s: %s 0", node->var.name, asm_keyword_for_size(variable_size(node), tmp_buf));
    }
}
void codegen_generate_global_variable(struct node *node)
{
    asm_push("; %s %s", node->var.type.type_str, node->var.name);
    switch (node->var.type.type)
    {
    case DATA_TYPE_VOID:
    case DATA_TYPE_CHAR:
    case DATA_TYPE_SHORT:
    case DATA_TYPE_INTEGER:
    case DATA_TYPE_LONG:
        codegen_generate_global_variable_for_primitive(node);
        break;

    case DATA_TYPE_DOUBLE:
    case DATA_TYPE_FLOAT:
        compiler_error(current_process, "Doubles and floats are not supported in our subset of C\n");
        break;
    }
}
void codegen_generate_data_section_part(struct node *node)
{
    switch (node->type)
    {
    case NODE_TYPE_VARIABLE:
        codegen_generate_global_variable(node);
        break;

    default:
        break;
    }
}
void codegen_generate_data_section()
{
    asm_push("section .data");
    struct node *node = codegen_node_next();
    while (node)
    {
        codegen_generate_data_section_part(node);
        node = codegen_node_next();
    }
}

struct resolver_entity *codegen_register_function(struct node *func_node, int flags)
{
    return resolver_default_register_function(current_process->resolver, func_node, flags);
}
void codegen_generate_function_prototype(struct node *node)
{
    codegen_register_function(node, 0);
    asm_push("extern %s", node->func.name);
}

void codegen_generate_function_arguments(struct vector *argument_vector)
{
    vector_set_peek_pointer(argument_vector, 0);
    struct node *current = vector_peek_ptr(argument_vector);
    while (current)
    {
        codegen_new_scope_entity(current, current->var.aoffset, RESOLVER_DEFAULT_ENTITY_FLAG_IS_LOCAL_STACK);
        current = vector_peek_ptr(argument_vector);
    }
}

void codegen_generate_number_node(struct node *node, struct history *history)
{
    asm_push_ins_push_with_data("dword %i", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value", STACK_FRAME_ELEMENT_FLAG_IS_NUMERICAL, &(struct stack_frame_data){.dtype = datatype_for_numeric()}, node->llnum);
}

bool codegen_is_exp_root_for_flags(int flags)
{
    return !(flags & EXPRESSION_IS_NOT_ROOT_NODE);
}

bool codegen_is_exp_root(struct history *history)
{
    return codegen_is_exp_root_for_flags(history->flags);
}

void codegen_reduce_register(const char* reg, size_t size, bool is_signed)
{
    if (size != DATA_SIZE_DWORD)
    {
        const char* ins = "movsx";
        if (!is_signed)
        {
            ins = "movzx";
        }
        asm_push("%s eax, %s", codegen_sub_register("eax", size));
    }
}
void codegen_gen_mem_access(struct node* node, int flags, struct resolver_entity* entity)
{
    #warning "generate & address"
    #warning "generate structure non pointer access"

    if (datatype_element_size(&entity->dtype) != DATA_SIZE_DWORD)
    {
        asm_push("mov eax, [%s]", codegen_entity_private(entity)->address);
        codegen_reduce_register("eax", datatype_element_size(&entity->dtype), entity->dtype.flags & DATATYPE_FLAG_IS_SIGNED);
        asm_push_ins_push_with_data("eax", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value", 0, &(struct stack_frame_data){.dtype=entity->dtype});
    }
    else
    {
        // We can push this straight to the stack
        asm_push_ins_push_with_data("dword [%s]", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value", 0, &(struct stack_frame_data){.dtype=entity->dtype}, codegen_entity_private(entity)->address);
    }
}
void codegen_generate_variable_access_for_entity(struct node* node, struct resolver_entity* entity, struct history* history)
{
    codegen_gen_mem_access(node, history->flags, entity);
}
void codegen_generate_variable_access(struct node* node, struct resolver_entity* entity, struct history* history)
{
    codegen_generate_variable_access_for_entity(node, entity, history_down(history, history->flags));
}
void codegen_generate_identifier(struct node* node, struct history* history)
{
    struct resolver_result* result = resolver_follow(current_process->resolver, node);
    assert(resolver_result_ok(result));

    struct resolver_entity* entity = resolver_result_entity(result);
    codegen_generate_variable_access(node, entity, history);
    codegen_response_acknowledge(&(struct response){.flags=RESPONSE_FLAG_RESOLVED_ENTITY,.data.resolved_entity=entity});
}
void codegen_generate_expressionable(struct node *node, struct history *history)
{
    bool is_root = codegen_is_exp_root(history);
    if (is_root)
    {
        history->flags |= EXPRESSION_IS_NOT_ROOT_NODE;
    }

    switch (node->type)
    {

    case NODE_TYPE_IDENTIFIER:
        codegen_generate_identifier(node, history);
        break;
    case NODE_TYPE_NUMBER:
        codegen_generate_number_node(node, history);
        break;

    case NODE_TYPE_EXPRESSION:
        codegen_generate_exp_node(node, history);
        break;
    }
}

const char *codegen_sub_register(const char *original_register, size_t size)
{
    const char *reg = NULL;
    if (S_EQ(original_register, "eax"))
    {
        if (size == DATA_SIZE_BYTE)
        {
            reg = "al";
        }
        else if (size == DATA_SIZE_WORD)
        {
            reg = "ax";
        }
        else if (size == DATA_SIZE_DWORD)
        {
            reg = "eax";
        }
    }
    else if (S_EQ(original_register, "ebx"))
    {
        if (size == DATA_SIZE_BYTE)
        {
            reg = "bl";
        }
        else if (size == DATA_SIZE_WORD)
        {
            reg = "bx";
        }
        else if (size == DATA_SIZE_DWORD)
        {
            reg = "ebx";
        }
    }
    else if (S_EQ(original_register, "ecx"))
    {
        if (size == DATA_SIZE_BYTE)
        {
            reg = "cl";
        }
        else if (size == DATA_SIZE_WORD)
        {
            reg = "cx";
        }
        else if (size == DATA_SIZE_DWORD)
        {
            reg = "ecx";
        }
    }
    else if (S_EQ(original_register, "edx"))
    {
        if (size == DATA_SIZE_BYTE)
        {
            reg = "dl";
        }
        else if (size == DATA_SIZE_WORD)
        {
            reg = "dx";
        }
        else if (size == DATA_SIZE_DWORD)
        {
            reg = "edx";
        }
    }

    return reg;
}
const char *codegen_byte_word_or_dword_or_ddword(size_t size, const char **reg_to_use)
{
    const char *type = NULL;
    const char *new_register = *reg_to_use;
    if (size == DATA_SIZE_BYTE)
    {
        type = "byte";
        new_register = codegen_sub_register(*reg_to_use, DATA_SIZE_BYTE);
    }
    else if (size == DATA_SIZE_WORD)
    {
        type = "word";
        new_register = codegen_sub_register(*reg_to_use, DATA_SIZE_WORD);
    }
    else if (size == DATA_SIZE_DWORD)
    {
        type = "dword";
        new_register = codegen_sub_register(*reg_to_use, DATA_SIZE_DWORD);
    }
    else if (size == DATA_SIZE_DDWORD)
    {
        type = "ddword";
        new_register = codegen_sub_register(*reg_to_use, DATA_SIZE_DDWORD);
    }
    *reg_to_use = new_register;
    return type;
}

void codegen_generate_assignment_instruction_for_operator(const char *mov_type_keyword, const char *address, const char *reg_to_use, const char *op, bool is_signed)
{
    if (S_EQ(op, "="))
    {
        asm_push("mov %s [%s], %s", mov_type_keyword, address, reg_to_use);
    }
    else if (S_EQ(op, "+="))
    {
        asm_push("add %s [%s], %s", mov_type_keyword, address, reg_to_use);
    }
}
void codegen_generate_scope_variable(struct node *node)
{
    struct resolver_entity *entity = codegen_new_scope_entity(node, node->var.aoffset, RESOLVER_DEFAULT_ENTITY_FLAG_IS_LOCAL_STACK);
    if (node->var.val)
    {
        codegen_generate_expressionable(node->var.val, history_begin(EXPRESSION_IS_ASSIGNMENT | IS_RIGHT_OPERAND_OF_ASSIGNMENT));
        // pop eax
        asm_push_ins_pop("eax", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value");
        const char *reg_to_use = "eax";
        const char *mov_type = codegen_byte_word_or_dword_or_ddword(datatype_element_size(&entity->dtype), &reg_to_use);
        codegen_generate_assignment_instruction_for_operator(mov_type, codegen_entity_private(entity)->address, reg_to_use, "=", entity->dtype.flags & DATATYPE_FLAG_IS_SIGNED);
    }
}

void codegen_generate_entity_access_start(struct resolver_result *result, struct resolver_entity *root_assignment_entity, struct history *history)
{
    if (root_assignment_entity->type == RESOLVER_ENTITY_TYPE_UNSUPPORTED)
    {
        // Unsupported entity then process it.
        codegen_generate_expressionable(root_assignment_entity->node, history);
    }
    else if (result->flags & RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE)
    {
        asm_push_ins_push_with_data("dword [%s]", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value", 0, &(struct stack_frame_data){.dtype = root_assignment_entity->dtype}, result->base.address);
    }
    else if (result->flags & RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX)
    {
        if (root_assignment_entity->next && root_assignment_entity->next->flags & RESOLVER_ENTITY_FLAG_IS_POINTER_ARRAY_ENTITY)
        {
            asm_push("mov ebx, [%s]", result->base.address);
        }
        else
        {
            asm_push("lea ebx, [%s]", result->base.address);
        }
        asm_push_ins_push_with_data("ebx", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value", 0, &(struct stack_frame_data){.dtype = root_assignment_entity->dtype});
    }
}

void codegen_generate_entity_access_for_variable_or_general(struct resolver_result *result, struct resolver_entity *entity)
{
    // Restore the EBX register
    asm_push_ins_pop("ebx", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value");
    if (entity->flags & RESOLVER_ENTITY_FLAG_DO_INDIRECTION)
    {
        asm_push("mov ebx, [ebx]");
    }
    asm_push("add ebx, %i", entity->offset);
    asm_push_ins_push_with_data("ebx", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value", 0, &(struct stack_frame_data){.dtype = entity->dtype});
}

void codegen_generate_entity_access_for_entity_for_assignment_left_operand(struct resolver_result *result, struct resolver_entity *entity, struct history *history)
{
    switch (entity->type)
    {
    case RESOLVER_ENTITY_TYPE_ARRAY_BRACKET:
#warning "todo implement array bracket"
        break;

    case RESOLVER_ENTITY_TYPE_VARIABLE:
    case RESOLVER_ENTITY_TYPE_GENERAL:
        codegen_generate_entity_access_for_variable_or_general(result, entity);
        break;

    case RESOLVER_ENTITY_TYPE_FUNCTION_CALL:
#warning "too function call"
        break;

    case RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION:
#warning "unary indirection"
        break;

    case RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS:
#warning "unary get address"
        break;

    case RESOLVER_ENTITY_TYPE_UNSUPPORTED:
#warning "unsupported"
        break;

    case RESOLVER_ENTITY_TYPE_CAST:
#warning "cast"
        break;

    default:
        compiler_error(current_process, "COMPILER BUG...");
    }
}
void codegen_generate_entity_access_for_assignment_left_operand(struct resolver_result *result, struct resolver_entity *root_assignment_entity, struct node *top_most_node, struct history *history)
{
    codegen_generate_entity_access_start(result, root_assignment_entity, history);
    struct resolver_entity *current = resolver_result_entity_next(root_assignment_entity);
    while (current)
    {
        codegen_generate_entity_access_for_entity_for_assignment_left_operand(result, current, history);
        current = resolver_result_entity_next(current);
    }
}

void codegen_generate_assignment_part(struct node *node, const char *op, struct history *history)
{
    struct datatype right_operand_dtype;
    struct resolver_result *result = resolver_follow(current_process->resolver, node);
    assert(resolver_result_ok(result));
    struct resolver_entity *root_assignment_entity = resolver_result_entity_root(result);
    const char *reg_to_use = "eax";
    const char *mov_type = codegen_byte_word_or_dword_or_ddword(datatype_element_size(&result->last_entity->dtype), &reg_to_use);
    struct resolver_entity *next_entity = resolver_result_entity_next(root_assignment_entity);
    if (!next_entity)
    {
        if (datatype_is_struct_or_union_non_pointer(&result->last_entity->dtype))
        {
#warning "generate a move struct"
        }
        else
        {
            asm_push_ins_pop("eax", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value");
            codegen_generate_assignment_instruction_for_operator(mov_type, result->base.address, reg_to_use, op, result->last_entity->dtype.flags & DATATYPE_FLAG_IS_SIGNED);
        }
    }
    else
    {
        codegen_generate_entity_access_for_assignment_left_operand(result, root_assignment_entity, node, history);
        asm_push_ins_pop("edx", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value");
        asm_push_ins_pop("eax", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value");
        codegen_generate_assignment_instruction_for_operator(mov_type, "edx", reg_to_use, op, result->last_entity->flags & DATATYPE_FLAG_IS_SIGNED);
    }
}
void codegen_generate_assignment_expression(struct node *node, struct history *history)
{
    codegen_generate_expressionable(node->exp.right, history_down(history, EXPRESSION_IS_ASSIGNMENT | IS_RIGHT_OPERAND_OF_ASSIGNMENT));
    codegen_generate_assignment_part(node->exp.left, node->exp.op, history);
}

void codegen_generate_entity_access_for_entity(struct resolver_result *result, struct resolver_entity *entity, struct history *history)
{
    switch (entity->type)
    {
    case RESOLVER_ENTITY_TYPE_ARRAY_BRACKET:
#warning "todo implement array bracket"
        break;

    case RESOLVER_ENTITY_TYPE_VARIABLE:
    case RESOLVER_ENTITY_TYPE_GENERAL:
        codegen_generate_entity_access_for_variable_or_general(result, entity);
        break;

    case RESOLVER_ENTITY_TYPE_FUNCTION_CALL:
#warning "too function call"
        break;

    case RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION:
#warning "unary indirection"
        break;

    case RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS:
#warning "unary get address"
        break;

    case RESOLVER_ENTITY_TYPE_UNSUPPORTED:
#warning "unsupported"
        break;

    case RESOLVER_ENTITY_TYPE_CAST:
#warning "cast"
        break;

    default:
        compiler_error(current_process, "COMPILER BUG...");
    }
}
void codegen_generate_entity_access(struct resolver_result *result, struct resolver_entity *root_assignment_entity, struct node *top_most_node, struct history *history)
{
    codegen_generate_entity_access_start(result, root_assignment_entity, history);
    struct resolver_entity *current = resolver_result_entity_next(root_assignment_entity);
    while (current)
    {
        codegen_generate_entity_access_for_entity(result, current, history);
        current = resolver_result_entity_next(current);
    }
}
bool codegen_resolve_node_return_result(struct node *node, struct history *history, struct resolver_result **result_out)
{
    struct resolver_result *result = resolver_follow(current_process->resolver, node);
    if (resolver_result_ok(result))
    {
        struct resolver_entity *root_assignment_entity = resolver_result_entity_root(result);
        codegen_generate_entity_access(result, root_assignment_entity, node, history);
        if (result_out)
        {
            *result_out = result;
        }
        codegen_response_acknowledge(&(struct response){.flags=RESPONSE_FLAG_RESOLVED_ENTITY, .data.resolved_entity=result->last_entity});
        return true;
    }

    return false;
}

bool codegen_resolve_node_for_value(struct node *node, struct history *history)
{
    struct resolver_result *result = NULL;
    if (!codegen_resolve_node_return_result(node, history, &result))
    {
        return false;
    }

    #warning "MORE TO GO FOR RESOLVING NODE VALUE"

    return true;
}

int get_additional_flags(int current_flags, struct node* node)
{
    if (node->type != NODE_TYPE_EXPRESSION)
    {
        return 0;
    }

    int additional_flags = 0;
    bool maintain_function_call_argument_flag = (current_flags & EXPRESSION_IN_FUNCTION_CALL_ARGUMENTS) && S_EQ(node->exp.op, ",");
    if (maintain_function_call_argument_flag)
    {
        additional_flags |= EXPRESSION_IN_FUNCTION_CALL_ARGUMENTS;
    }
    return additional_flags;
}

int codegen_set_flag_for_operator(const char* op)
{
    int flag = 0;
    if (S_EQ(op, "+"))
    {
        flag |= EXPRESSION_IS_ADDITION;
    }
    else if(S_EQ(op, "-"))
    {
        flag |= EXPRESSION_IS_SUBTRACTION;
    }
    else if(S_EQ(op, "*"))
    {
        flag |= EXPRESSION_IS_MULTIPLICATION;
    }
    else if(S_EQ(op, "/"))
    {
        flag |= EXPRESSION_IS_DIVISION;
    }
    else if(S_EQ(op, "%"))
    {
        flag |= EXPRESSION_IS_MODULAS;
    }
    else if(S_EQ(op, ">"))
    {
        flag |= EXPRESSION_IS_ABOVE;
    }
    else if(S_EQ(op, "<"))
    {
        flag |= EXPRESSION_IS_BELOW;
    }
    else if(S_EQ(op, ">="))
    {
        flag |= EXPRESSION_IS_ABOVE_OR_EQUAL;
    }
    else if(S_EQ(op, "<="))
    {
        flag |= EXPRESSION_IS_BELOW_OR_EQUAL;
    }
    else if(S_EQ(op, "!="))
    {
        flag |= EXPRESSION_IS_NOT_EQUAL;
    }
    else if(S_EQ(op, "=="))
    {
        flag |= EXPRESSION_IS_EQUAL;
    }
    else if(S_EQ(op, "&&"))
    {
        flag |= EXPRESSION_LOGICAL_AND;
    }
    else if(S_EQ(op, "<<"))
    {
        flag |= EXPRESSION_IS_BITSHIFT_LEFT;
    }
    else if(S_EQ(op, ">>"))
    {
        flag |= EXPRESSION_IS_BITSHIFT_RIGHT;
    }
    else if(S_EQ(op, "&"))
    {
        flag |= EXPRESSION_IS_BITWISE_AND;
    }
    else if(S_EQ(op, "|"))
    {
        flag |= EXPRESSION_IS_BITWISE_OR;
    }
    else if(S_EQ(op, "^"))
    {
        flag |= EXPRESSION_IS_BITWISE_XOR;
    }
    return flag;
}

struct stack_frame_element* asm_stack_back()
{
    return stackframe_back(current_function);
}

struct stack_frame_element* asm_stack_peek()
{
    return stackframe_peek(current_function);
}

void asm_stack_peek_start()
{
    stackframe_peek_start(current_function);
}

bool asm_datatype_back(struct datatype* dtype_out)
{
    struct stack_frame_element* last_stack_frame_element = asm_stack_back();
    if (!last_stack_frame_element)
    {
        return false;
    }

    if (!(last_stack_frame_element->flags & STACK_FRAME_ELEMENT_FLAG_HAS_DATATYPE))
    {
        return false;
    }

    *dtype_out = last_stack_frame_element->data.dtype;
    return true;
}

bool codegen_can_gen_math(int flags)
{
    return flags & EXPRESSION_GEN_MATHABLE;
}

void codegen_gen_cmp(const char* value, const char* set_ins)
{
    asm_push("cmp eax, %s", value);
    asm_push("%s al", set_ins);
    asm_push("movzx eax, al");
}

void codegen_gen_math_for_value(const char* reg, const char* value, int flags, bool is_signed)
{
    if (flags & EXPRESSION_IS_ADDITION)
    {
        asm_push("add %s, %s", reg, value);
    }
    else if(flags & EXPRESSION_IS_SUBTRACTION)
    {
        asm_push("sub %s, %s", reg, value);
    }
    else if(flags & EXPRESSION_IS_MULTIPLICATION)
    {
        asm_push("mov ecx, %s", value);
        if (is_signed)
        {
            asm_push("imul ecx");
        }
        else
        {
            asm_push("mul ecx");
        }
    }
    else if(flags & EXPRESSION_IS_DIVISION)
    {
        asm_push("mov ecx, %s", value);
        asm_push("cdq");
        if (is_signed)
        {
            asm_push("idiv ecx");
        }
        else
        {
            asm_push("div ecx");
        }
    }
    else if(flags & EXPRESSION_IS_MODULAS)
    {
        asm_push("mov ecx, %s", value);
        asm_push("cdq");
        if (is_signed)
        {
            asm_push("idiv ecx");
        }
        else
        {
            asm_push("div ecx");
        }

        asm_push("mov eax, edx");
    }
    else if(flags & EXPRESSION_IS_ABOVE)
    {
        codegen_gen_cmp(value, "setg");
    }
    else if(flags & EXPRESSION_IS_BELOW)
    {
        codegen_gen_cmp(value, "setl");
    }
    else if(flags & EXPRESSION_IS_EQUAL)
    {
        codegen_gen_cmp(value, "sete");
    }
    else if(flags & EXPRESSION_IS_ABOVE_OR_EQUAL)
    {
        codegen_gen_cmp(value, "setge");
    }
    else if(flags & EXPRESSION_IS_BELOW_OR_EQUAL)
    {
        codegen_gen_cmp(value, "setle");
    }
    else if(flags & EXPRESSION_IS_NOT_EQUAL)
    {
        codegen_gen_cmp(value, "setne");
    }
    else if(flags & EXPRESSION_IS_BITSHIFT_LEFT)
    {
        value = codegen_sub_register(value, DATA_SIZE_BYTE);
        asm_push("sal %s, %s", reg, value);
    }
    else if(flags & EXPRESSION_IS_BITSHIFT_RIGHT)
    {
        value = codegen_sub_register(value, DATA_SIZE_BYTE);
        asm_push("sar %s, %s", reg, value);
    }
    else if(flags & EXPRESSION_IS_BITWISE_AND)
    {
        asm_push("and %s, %s", reg, value);
    }
    else if(flags & EXPRESSION_IS_BITWISE_OR)
    {
        asm_push("or %s, %s", reg, value);
    }
    else if(flags & EXPRESSION_IS_BITWISE_XOR)
    {
        asm_push("xor %s, %s", reg, value);
    }

}
void codegen_generate_exp_node_for_arithmetic(struct node* node, struct history* history)
{
    assert(node->type == NODE_TYPE_EXPRESSION);
    int flags = history->flags;

    // if (is_logical_operator(node->exp.op))
    // {
    //     codegen_generate_exp_node_for_logical_arithmetic
    // }

    struct node* left_node = node->exp.left;
    struct node* right_node = node->exp.right;
    int op_flags = codegen_set_flag_for_operator(node->exp.op);
    codegen_generate_expressionable(left_node, history_down(history, flags));
    codegen_generate_expressionable(right_node, history_down(history, flags));
    struct datatype last_dtype = datatype_for_numeric();
    asm_datatype_back(&last_dtype);
    if (codegen_can_gen_math(op_flags))
    {
        struct datatype right_dtype = datatype_for_numeric();
        asm_datatype_back(&right_dtype);
        asm_push_ins_pop("ecx", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value");
        if (last_dtype.flags & DATATYPE_FLAG_IS_LITERAL)
        {
            asm_datatype_back(&last_dtype);
        }

        struct datatype left_dtype = datatype_for_numeric();
        asm_datatype_back(&left_dtype);
        asm_push_ins_pop("eax", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value");
        struct datatype* pointer_datatype = datatype_thats_a_pointer(&left_dtype, &right_dtype);
        if (pointer_datatype && datatype_size(datatype_pointer_reduce(pointer_datatype, 1)) > DATA_SIZE_BYTE)
        {
            const char* reg = "ecx";
            if (pointer_datatype == &right_dtype)
            {
                reg = "eax";
            }
            asm_push("imul %s, %i", reg, datatype_size(datatype_pointer_reduce(pointer_datatype, 1)));
        }

        codegen_gen_math_for_value("eax", "ecx", op_flags, last_dtype.flags & DATATYPE_FLAG_IS_SIGNED);
    }

    asm_push_ins_push_with_data("eax", STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE, "result_value", 0, &(struct stack_frame_data){.dtype=last_dtype});
}

int codegen_remove_uninheritable_flags(int flags)
{
    return flags & ~EXPRESSION_UNINHERITABLE_FLAGS;
}
void codegen_generate_exp_node(struct node *node, struct history *history)
{
    if (is_node_assignment(node))
    {
        codegen_generate_assignment_expression(node, history);
        return;
    }

    // Can we locate a variable for a given expression?
    if (codegen_resolve_node_for_value(node, history))
    {
        return;
    }

    int additional_flags = get_additional_flags(history->flags, node);
    codegen_generate_exp_node_for_arithmetic(node, history_down(history, codegen_remove_uninheritable_flags(history->flags) | additional_flags));
}
void codegen_generate_statement(struct node *node, struct history *history)
{
    switch (node->type)
    {
    case NODE_TYPE_EXPRESSION:
        codegen_generate_exp_node(node, history_begin(history->flags));
        break;

    case NODE_TYPE_VARIABLE:
        codegen_generate_scope_variable(node);
        break;
    }
}
void codegen_generate_scope_no_new_scope(struct vector *statements, struct history *history)
{
    vector_set_peek_pointer(statements, 0);
    struct node *statement_node = vector_peek_ptr(statements);
    while (statement_node)
    {
        codegen_generate_statement(statement_node, history);
        statement_node = vector_peek_ptr(statements);
    }
}
void codegen_generate_stack_scope(struct vector *statements, size_t scope_size, struct history *history)
{
    codegen_new_scope(RESOLVER_SCOPE_FLAG_IS_STACK);
    codegen_generate_scope_no_new_scope(statements, history);
    codegen_finish_scope();
}

void codegen_generate_body(struct node *node, struct history *history)
{
    codegen_generate_stack_scope(node->body.statements, node->body.size, history);
}
void codegen_generate_function_with_body(struct node *node)
{
    codegen_register_function(node, 0);
    asm_push("global %s", node->func.name);
    asm_push("; %s function", node->func.name);
    asm_push("%s:", node->func.name);

    asm_push_ebp();
    asm_push("mov ebp, esp");
    codegen_stack_sub(C_ALIGN(function_node_stack_size(node)));
    codegen_new_scope(RESOLVER_DEFAULT_ENTITY_FLAG_IS_LOCAL_STACK);
    codegen_generate_function_arguments(function_node_argument_vec(node));

    codegen_generate_body(node->func.body_n, history_begin(IS_ALONE_STATEMENT));
    codegen_finish_scope();
    codegen_stack_add(C_ALIGN(function_node_stack_size(node)));
    asm_pop_ebp();
    stackframe_assert_empty(current_function);
    asm_push("ret");
}
void codegen_generate_function(struct node *node)
{
    current_function = node;
    if (function_node_is_prototype(node))
    {
        codegen_generate_function_prototype(node);
        return;
    }

    codegen_generate_function_with_body(node);
}

void codegen_generate_root_node(struct node *node)
{
    switch (node->type)
    {
    case NODE_TYPE_VARIABLE:
        // We processed this earlier in data section.
        break;

    case NODE_TYPE_FUNCTION:
        codegen_generate_function(node);
        break;
    }
}

void codegen_generate_root()
{
    asm_push("section .text");
    struct node *node = NULL;
    while ((node = codegen_node_next()) != NULL)
    {
        codegen_generate_root_node(node);
    }
}

bool codegen_write_string_char_escaped(char c)
{
    const char *c_out = NULL;
    switch (c)
    {
    case '\n':
        c_out = "10";
        break;

    case '\t':
        c_out = "9";
        break;
    }

    if (c_out)
    {
        asm_push_no_nl("%s, ", c_out);
    }
    return c_out != NULL;
}
void codegen_write_string(struct string_table_element *element)
{
    asm_push_no_nl("%s: db ", element->label);
    size_t len = strlen(element->str);
    for (int i = 0; i < len; i++)
    {
        char c = element->str[i];
        bool handled = codegen_write_string_char_escaped(c);
        if (handled)
        {
            continue;
        }
        asm_push_no_nl("'%c', ", c);
    }

    asm_push_no_nl("0");
    asm_push("");
}

void codegen_write_strings()
{
    struct code_generator *generator = current_process->generator;
    vector_set_peek_pointer(generator->string_table, 0);
    struct string_table_element *element = vector_peek_ptr(generator->string_table);
    while (element)
    {
        codegen_write_string(element);
        element = vector_peek_ptr(generator->string_table);
    }
}

void codegen_generate_rod()
{
    asm_push("section .rodata");
    codegen_write_strings();
}

int codegen(struct compile_process *process)
{
    current_process = process;
    scope_create_root(process);
    vector_set_peek_pointer(process->node_tree_vec, 0);
    codegen_new_scope(0);
    codegen_generate_data_section();
    vector_set_peek_pointer(process->node_tree_vec, 0);
    codegen_generate_root();
    codegen_finish_scope();

    // Generate read only data
    codegen_generate_rod();

    return 0;
}
