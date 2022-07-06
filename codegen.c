#include "compiler.h"
#include "helpers/vector.h"
#include <stdarg.h>
#include <stdio.h>
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

void codegen_write_strings()
{
    #warning "Loop through the string table and write all the strings."
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

    // Generate read only data
    codegen_generate_rod();
    return 0;
}