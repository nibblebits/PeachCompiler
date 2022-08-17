#include "compiler.h"
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
struct resolver_default_entity_data* resolver_default_entity_private(struct resolver_entity* entity)
{
    return entity->private;
}

struct resolver_default_scope_data* resolver_default_scope_private(struct resolver_scope* scope)
{
    return scope->private;
}

char* resolver_default_stack_asm_address(int stack_offset, char* out)
{
    if (stack_offset < 0)
    {
        sprintf(out, "ebp%i", stack_offset);
        return out;
    }

    sprintf(out, "ebp+%i", stack_offset);
    return out;
}

struct resolver_default_entity_data* resolver_default_new_entity_data()
{
    struct resolver_default_entity_data* entity_data = calloc(sizeof(struct resolver_default_entity_data), 1);
    return entity_data;
}

void resolver_default_global_asm_address(const char* name, int offset, char* address_out)
{
    if (offset == 0)
    {
        sprintf(address_out, "%s", name);
        return;
    }

    sprintf(address_out, "%s+%i", name, offset);
}

void resolver_default_entity_data_set_address(struct resolver_default_entity_data* entity_data, struct node* var_node, int offset, int flags)
{
    if (!var_node)
    {
        return;
    }
    if (!variable_node(var_node)->var.name)
    {
        return;
    }

    entity_data->offset = offset;
    if (flags & RESOLVER_DEFAULT_ENTITY_FLAG_IS_LOCAL_STACK)
    {
        resolver_default_stack_asm_address(offset, entity_data->address);
        sprintf(entity_data->base_address, "ebp");
    }
    else
    {
        resolver_default_global_asm_address(variable_node(var_node)->var.name, offset, entity_data->address);
        sprintf(entity_data->base_address, "%s", variable_node(var_node)->var.name);
    }
}

void* resolver_default_make_private(struct resolver_entity* entity, struct node* node, int offset, struct resolver_scope* scope)
{
    struct resolver_default_entity_data* entity_data = resolver_default_new_entity_data();
    int entity_flags = 0x00;
    if (entity->flags & RESOLVER_ENTITY_FLAG_IS_STACK)
    {
        entity_flags |= RESOLVER_DEFAULT_ENTITY_FLAG_IS_LOCAL_STACK;
    }
    entity_data->offset = offset;
    entity_data->flags = entity_flags;
    entity_data->type = entity->type;
    if (variable_node(node))
    {
        resolver_default_entity_data_set_address(entity_data, variable_node(node), offset, entity_flags);
    }

    return entity_data;
}

void resolver_default_set_result_base(struct resolver_result* result, struct resolver_entity* base_entity)
{
    struct resolver_default_entity_data* data = resolver_default_entity_private(base_entity);
    if (!data)
    {
        return;
    }

    strncpy(result->base.base_address, data->base_address, sizeof(result->base.base_address));
    strncpy(result->base.address, data->address, sizeof(result->base.address));
    result->base.offset = data->offset;
}

struct resolver_default_entity_data* resolver_default_new_entity_data_for_var_node(struct node* var_node, int offset, int flags)
{
    struct resolver_default_entity_data* entity_data = resolver_default_new_entity_data();
    assert(variable_node(var_node));
    entity_data->offset = offset;
    entity_data->flags = flags;
    entity_data->type = RESOLVER_DEFAULT_ENTITY_DATA_TYPE_VARIABLE;
    resolver_default_entity_data_set_address(entity_data, variable_node(var_node), offset,flags);
    return entity_data;
}

struct resolver_default_entity_data* resolver_default_new_entity_data_for_array_bracket(struct node* breacket_node)
{
    struct resolver_default_entity_data* entity_data = resolver_default_new_entity_data();
    entity_data->type = RESOLVER_DEFAULT_ENTITY_DATA_TYPE_ARRAY_BRACKET;
    return entity_data;
}

struct resolver_default_entity_data* resolver_default_new_entity_data_for_function(struct node* func_node, int flags)
{
    struct resolver_default_entity_data* entity_data = resolver_default_new_entity_data();
    entity_data->flags = flags;
    entity_data->type = RESOLVER_DEFAULT_ENTITY_DATA_TYPE_FUNCTION;
    resolver_default_global_asm_address(func_node->func.name, 0, entity_data->address);
    return entity_data;
}

struct resolver_entity* resolver_default_new_scope_entity(struct resolver_process* resolver, struct node* var_node, int offset, int flags)
{
    assert(var_node->type == NODE_TYPE_VARIABLE);
    struct resolver_default_entity_data* entity_data = resolver_default_new_entity_data_for_var_node(variable_node(var_node), offset, flags);
    return resolver_new_entity_for_var_node(resolver, variable_node(var_node), entity_data, offset);
}

struct resolver_entity* resolver_default_register_function(struct resolver_process* resolver, struct node* func_node, int flags)
{
    struct resolver_default_entity_data* private = resolver_default_new_entity_data_for_function(func_node, flags);
    return resolver_register_function(resolver, func_node, private);
}

void resolver_default_new_scope(struct resolver_process* resolver, int flags)
{
    struct resolver_default_scope_data* scope_data = calloc(sizeof(struct resolver_default_scope_data), 1);
    scope_data->flags |= flags;
    resolver_new_scope(resolver, scope_data, flags);
}

void resolver_default_finish_scope(struct resolver_process* resolver)
{
    resolver_finish_scope(resolver);
}

void* resolver_default_new_array_entity(struct resolver_result* result, struct node* array_entity_node)
{
    return resolver_default_new_entity_data_for_array_bracket(array_entity_node);
}

void resolver_default_delete_entity(struct resolver_entity*  entity)
{
    free(entity->private);
}

void resolver_default_delete_scope(struct resolver_scope* scope)
{
    free(scope->private);
}

static void resolver_default_merge_array_calculate_out_offset(struct datatype* dtype, struct resolver_entity* entity, int* out_offset)
{
    assert(entity->array.array_index_node->type == NODE_TYPE_NUMBER);
    int index_val = entity->array.array_index_node->llnum;
    *(out_offset) += array_offset(dtype, entity->array.index, index_val);
}

struct resolver_entity* resolver_default_merge_entities(struct resolver_process* process, struct resolver_result* result, struct resolver_entity* left_entity, struct resolver_entity* right_entity)
{
    int new_pos = left_entity->offset + right_entity->offset;
    return resolver_make_entity(process, result, &right_entity->dtype, left_entity->node, &(struct resolver_entity){.type=right_entity->type, .flags=left_entity->flags,.offset=new_pos,.array=right_entity->array}, left_entity->scope);
}

struct resolver_process* resolver_default_new_process(struct compile_process* compiler)
{
    return resolver_new_process(compiler, &(struct resolver_callbacks){.new_array_entity=resolver_default_new_array_entity, .delete_entity=resolver_default_delete_entity,.delete_scope=resolver_default_delete_scope,.merge_entities=resolver_default_merge_entities,.make_private=resolver_default_make_private,.set_result_base=resolver_default_set_result_base });

}