#include "compiler.h"
#include "helpers/vector.h"
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
static struct compile_process* current_process = NULL;

void codegen_new_scope(int flags)
{
    #warning "The resolver needs to exist for this to work"
}

void codegen_finish_scope()
{
    #warning "You need to invent a resolver for this to work"
}

struct node* codegen_node_next()
{
    return vector_peek_ptr(current_process->node_tree_vec);
}


void asm_push_args(const char* ins, va_list args)
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

void asm_push(const char* ins, ...)
{
    va_list args;
    va_start(args, ins);
    asm_push_args(ins, args);
    va_end(args);
}

void asm_push_no_nl(const char* ins, ...)
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


const char* codegen_get_label_for_string(const char* str)
{
    const char* result = NULL;
    struct code_generator* generator = current_process->generator;
    vector_set_peek_pointer(generator->string_table, 0);
    struct string_table_element* current = vector_peek_ptr(generator->string_table);
    while(current)
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

const char* codegen_register_string(const char* str)
{
    const char* label = codegen_get_label_for_string(str);
    if (label)
    {
        // We already registered this string, just return the label to the string memory.
        return label;
    }

    struct string_table_element* str_elem = calloc(1, sizeof(struct string_table_element));
    int label_id = codegen_label_count();
    sprintf((char*)str_elem->label, "str_%i", label_id);
    str_elem->str = str;
    vector_push(current_process->generator->string_table, &str_elem);
    return str_elem->label;
}


struct code_generator* codegenerator_new(struct compile_process* process)
{
    struct code_generator* generator= calloc(1, sizeof(struct code_generator));
    generator->string_table = vector_create(sizeof(struct string_table_element*));
    generator->entry_points = vector_create(sizeof(struct codegen_entry_point*));
    generator->exit_points = vector_create(sizeof(struct codegen_exit_point*));
    return generator;
}

void codegen_register_exit_point(int exit_point_id)
{
    struct code_generator* gen = current_process->generator;
    struct codegen_exit_point* exit_point = calloc(1, sizeof(struct codegen_exit_point));
    exit_point->id = exit_point_id;
    vector_push(gen->exit_points, &exit_point);
}

struct codegen_exit_point* codegen_current_exit_point()
{
    struct code_generator* gen = current_process->generator;
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
    struct code_generator* gen = current_process->generator;
    struct codegen_exit_point* exit_point = codegen_current_exit_point();
    assert(exit_point);
    asm_push(".exit_point_%i:", exit_point->id);
    free(exit_point);
    vector_pop(gen->exit_points);
}

void codegen_goto_exit_point(struct node* node)
{
    struct code_generator* gen = current_process->generator;
    struct codegen_exit_point* exit_point = codegen_current_exit_point();
    asm_push("jmp .exit_point_%i", exit_point->id);
}

void codegen_register_entry_point(int entry_point_id)
{
    struct code_generator* gen = current_process->generator;
    struct codegen_entry_point* entry_point = calloc(1, sizeof(struct codegen_entry_point));
    entry_point->id = entry_point_id;
    vector_push(gen->entry_points, &entry_point);
}

struct codegen_entry_point* codegen_current_entry_point()
{
    struct code_generator* gen = current_process->generator;
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
    struct code_generator* gen = current_process->generator;
    struct codegen_entry_point* entry_point = codegen_current_entry_point();
    assert(entry_point);
    free(entry_point);
    vector_pop(gen->entry_points);
}


void codegen_goto_entry_point(struct node* current_node)
{
    struct code_generator* gen = current_process->generator;
    struct codegen_entry_point* entry_point = codegen_current_entry_point();
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

static const char* asm_keyword_for_size(size_t size, char* tmp_buf)
{
    const char* keyword = NULL;
    switch(size)
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

void codegen_generate_global_variable_for_primitive(struct node* node)
{
    char tmp_buf[256];
    if (node->var.val != NULL)
    {
        // Handle the value.
        if (node->var.val->type == NODE_TYPE_STRING)
        {
            #warning "dont forget to handle the string value"
        }
        else
        {
            #warning "dont forget to handle the numeric value"
        }
    }

    asm_push("%s: %s 0", node->var.name, asm_keyword_for_size(variable_size(node), tmp_buf));
}
void codegen_generate_global_variable(struct node* node)
{
    asm_push("; %s %s", node->var.type.type_str, node->var.name);
    switch(node->var.type.type)
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
void codegen_generate_data_section_part(struct node* node)
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
    struct node* node = codegen_node_next();
    while(node)
    {
        codegen_generate_data_section_part(node);
        node = codegen_node_next();
    }
}

void codegen_generate_root_node(struct node* node)
{
    // PROCESS ANY FUNCTIONS.
}

void codegen_generate_root()
{
    asm_push("section .text");
    struct node* node = NULL;
    while((node = codegen_node_next()) != NULL)
    {
        codegen_generate_root_node(node);
    }
}

bool codegen_write_string_char_escaped(char c)
{
    const char* c_out = NULL;
    switch(c)
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
void codegen_write_string(struct string_table_element* element)
{
    asm_push_no_nl("%s: db ", element->label);
    size_t len = strlen(element->str);
    for (int i = 0; i < len; i++)
    {
        char c = element->str[i];
        bool handled = codegen_write_string_char_escaped(c);
        if(handled)
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
    struct code_generator* generator = current_process->generator;
    vector_set_peek_pointer(generator->string_table, 0);
    struct string_table_element* element = vector_peek_ptr(generator->string_table);
    while(element)
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

int codegen(struct compile_process* process)
{
    current_process = process;
    scope_create_root(process);
    vector_set_peek_pointer(process->node_tree_vec, 0);
    codegen_new_scope(0);
    codegen_generate_data_section();
    vector_set_peek_pointer(process->node_tree_vec, 0);
    codegen_generate_root();
    codegen_finish_scope();

    codegen_register_string("Hello world!!");
    codegen_register_string("Hello world!!");
    codegen_register_string("Hello world!!");
    codegen_register_string("Abc\n");

    // Generate read only data
    codegen_generate_rod();

    return 0;
}
