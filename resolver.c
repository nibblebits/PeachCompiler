#include "compiler.h"
#include "helpers/vector.h"
#include <stdlib.h>
#include <assert.h>
void resolver_follow_part(struct resolver_process *resolver, struct node *node, struct resolver_result *result);

bool resolver_result_failed(struct resolver_result *result)
{
    return result->flags & RESOLVER_RESULT_FLAG_FAILED;
}

bool resolver_result_ok(struct resolver_result *result)
{
    return !resolver_result_failed(result);
}

bool resolver_result_finished(struct resolver_result *result)
{
    return result->flags & RESOLVER_RESULT_FLAG_RUNTIME_NEEDED_TO_FINISH_PATH;
}

struct resolver_entity *resolver_result_entity_root(struct resolver_result *result)
{
    return result->entity;
}

struct resolver_entity *resolver_result_entity_next(struct resolver_entity *entity)
{
    return entity->next;
}

struct resolver_entity *resolver_entity_clone(struct resolver_entity *entity)
{
    if (!entity)
    {
        return NULL;
    }

    struct resolver_entity *new_entity = calloc(1, sizeof(struct resolver_entity));
    memcpy(new_entity, entity, sizeof(struct resolver_entity));
    return new_entity;
}

struct resolver_entity *resolver_result_entity(struct resolver_result *result)
{
    if (resolver_result_failed(result))
    {
        return NULL;
    }

    return result->entity;
}

struct resolver_result *resolver_new_result(struct resolver_process *process)
{
    struct resolver_result *result = calloc(1, sizeof(struct resolver_result));
    result->array_data.array_entities = vector_create(sizeof(struct resolver_entity *));
    return result;
}

void resolver_result_free(struct resolver_result *result)
{
    vector_free(result->array_data.array_entities);
    free(result);
}

struct resolver_scope *resolver_process_scope_current(struct resolver_process *process)
{
    return process->scope.current;
}

void resolver_runtime_needed(struct resolver_result *result, struct resolver_entity *last_entity)
{
    result->entity = last_entity;
    result->flags &= ~RESOLVER_RESULT_FLAG_RUNTIME_NEEDED_TO_FINISH_PATH;
}

void resolver_result_entity_push(struct resolver_result *result, struct resolver_entity *entity)
{
    if (!result->first_entity_const)
    {
        result->first_entity_const = entity;
    }

    if (!result->last_entity)
    {
        result->entity = entity;
        result->last_entity = entity;
        result->count++;
        return;
    }

    result->last_entity->next = entity;
    entity->prev = result->last_entity;
    result->last_entity = entity;
    result->count++;
}

struct resolver_entity *resolver_result_peek(struct resolver_result *result)
{
    return result->last_entity;
}

struct resolver_entity *resolver_result_peek_ignore_rule_entity(struct resolver_result *result)
{
    struct resolver_entity *entity = resolver_result_peek(result);
    while (entity && entity->type == RESOLVER_ENTITY_TYPE_RULE)
    {
        entity = entity->prev;
    }

    return entity;
}

struct resolver_entity *resolver_result_pop(struct resolver_result *result)
{
    struct resolver_entity *entity = result->last_entity;
    if (!result->entity)
    {
        return NULL;
    }

    if (result->entity == result->last_entity)
    {
        result->entity = result->last_entity->prev;
        result->last_entity = result->last_entity->prev;
        result->count--;
        goto out;
    }

    result->last_entity = result->last_entity->prev;
    result->count--;
out:
    if (result->count == 0)
    {
        result->first_entity_const = NULL;
        result->last_entity = NULL;
        result->entity = NULL;
    }
    entity->prev = NULL;
    entity->next = NULL;
    return entity;
}

struct vector *resolver_array_data_vec(struct resolver_result *result)
{
    return result->array_data.array_entities;
}

struct compile_process *resolver_compiler(struct resolver_process *process)
{
    return process->compiler;
}

struct resolver_scope *resolver_scope_current(struct resolver_process *process)
{
    return process->scope.current;
}

struct resolver_scope *resolver_scope_root(struct resolver_process *process)
{
    return process->scope.root;
}

struct resolver_scope *resolver_new_scope_create()
{
    struct resolver_scope *scope = calloc(1, sizeof(struct resolver_scope));
    scope->entities = vector_create(sizeof(struct resolver_entity *));
    return scope;
}

struct resolver_scope *resolver_new_scope(struct resolver_process *resolver, void *private, int flags)
{
    struct resolver_scope *scope = resolver_new_scope_create();
    if (!scope)
    {
        return NULL;
    }

    resolver->scope.current->next = scope;
    scope->prev = resolver->scope.current;
    resolver->scope.current = scope;
    scope->private = private;
    scope->flags = flags;
    return scope;
}

void resolver_finish_scope(struct resolver_process *resolver)
{
    struct resolver_scope *scope = resolver->scope.current;
    resolver->scope.current = scope->prev;
    resolver->callbacks.delete_scope(scope);
    free(scope);
}

struct resolver_process *resolver_new_process(struct compile_process *compiler, struct resolver_callbacks *callbacks)
{
    struct resolver_process *process = calloc(1, sizeof(struct resolver_process));
    process->compiler = compiler;
    memcpy(&process->callbacks, callbacks, sizeof(process->callbacks));
    process->scope.root = resolver_new_scope_create();
    process->scope.current = process->scope.root;
    return process;
}

struct resolver_entity *resolver_create_new_entity(struct resolver_result *result, int type, void *private)
{
    struct resolver_entity *entity = calloc(1, sizeof(struct resolver_entity));
    if (!entity)
    {
        return NULL;
    }

    entity->type = type;
    entity->private = private;
    return entity;
}

struct resolver_entity *resolver_create_new_entity_for_unsupported_node(struct resolver_result *result, struct node *node)
{
    struct resolver_entity *entity = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_UNSUPPORTED, NULL);
    if (!entity)
    {
        return NULL;
    }

    entity->node = node;
    entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY | RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
    return entity;
}

struct resolver_entity *resolver_create_new_entity_for_array_bracket(struct resolver_result *result, struct resolver_process *process, struct node *node, struct node *array_index_node, int index, struct datatype *dtype, void *private, struct resolver_scope *scope)
{
    struct resolver_entity *entity = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_ARRAY_BRACKET, private);
    if (!entity)
    {
        return NULL;
    }

    entity->scope = scope;
    assert(entity->scope);
    entity->name = NULL;
    entity->dtype = *dtype;
    entity->node = node;
    entity->array.index = index;
    entity->array.dtype = *dtype;
    entity->array.array_index_node = array_index_node;
    int array_index_val = 1;
    if (array_index_node->type == NODE_TYPE_NUMBER)
    {
        array_index_val = array_index_node->llnum;
    }
    entity->offset = array_offset(dtype, index, array_index_val);
    return entity;
}

struct resolver_entity *resolver_create_new_entity_for_merged_array_bracket(struct resolver_result *result, struct resolver_process *process, struct node *node, struct node *array_index_node, int index, struct datatype *dtype, void *private, struct resolver_scope *scope)
{
    struct resolver_entity *entity = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_ARRAY_BRACKET, private);
    if (!entity)
    {
        return NULL;
    }

    entity->scope = scope;
    assert(entity->scope);
    entity->name = NULL;
    entity->dtype = *dtype;
    entity->node = node;
    entity->array.index = index;
    entity->array.dtype = *dtype;
    entity->array.array_index_node = array_index_node;
    return entity;
}

struct resolver_entity *resolver_create_new_unknown_entity(struct resolver_process *process, struct resolver_result *result, struct datatype *dtype, struct node *node, struct resolver_scope *scope, int offset)
{
    struct resolver_entity *entity = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_GENERAL, NULL);
    if (!entity)
    {
        return NULL;
    }

    entity->flags |= RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY | RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY;
    entity->scope = scope;
    entity->dtype = *dtype;
    entity->node = node;
    entity->offset = offset;
    return entity;
}

struct resolver_entity *resolver_create_new_unary_indirection_entity(struct resolver_process *process, struct resolver_result *result, struct node *node, int indirection_depth)
{
    struct resolver_entity *entity = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION, NULL);
    if (!entity)
    {
        return NULL;
    }

    entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY | RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
    entity->node = node;
    entity->indirection.depth = indirection_depth;
    return entity;
}

/**
 * @brief &a.b.c
 *
 * @param process
 * @param result
 * @param dtype
 * @param node
 * @param scope
 * @param offset
 * @return struct resolver_entity*
 */
struct resolver_entity *resolver_create_new_unary_get_address_entity(struct resolver_process *process, struct resolver_result *result, struct datatype *dtype, struct node *node, struct resolver_scope *scope, int offset)
{
    struct resolver_entity *entity = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS, NULL);
    if (!entity)
    {
        return NULL;
    }

    entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY | RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
    entity->node = node;
    entity->scope = scope;

    entity->dtype = *dtype;
    entity->dtype.flags |= DATATYPE_FLAG_IS_POINTER;
    entity->dtype.pointer_depth++;
    return entity;
}

struct resolver_entity *resolver_create_new_cast_entity(struct resolver_process *process, struct resolver_scope *scope, struct datatype *cast_dtype)
{
    struct resolver_entity *entity = resolver_create_new_entity(NULL, RESOLVER_ENTITY_TYPE_CAST, NULL);
    if (!entity)
    {
        return NULL;
    }

    entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY | RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
    entity->scope = scope;
    entity->dtype = *cast_dtype;
    return entity;
}

struct resolver_entity *resolver_create_new_entity_for_var_node_custom_scope(struct resolver_process *process, struct node *var_node, void *private, struct resolver_scope *scope, int offset)
{
    assert(var_node->type == NODE_TYPE_VARIABLE);
    struct resolver_entity *entity = resolver_create_new_entity(NULL, NODE_TYPE_VARIABLE, private);
    if (!entity)
    {
        return NULL;
    }

    entity->scope = scope;
    assert(entity->scope);
    entity->dtype = var_node->var.type;
    entity->node = var_node;
    entity->name = var_node->var.name;
    entity->offset = offset;
    return entity;
}

struct resolver_entity *resolver_create_new_entity_for_var_node(struct resolver_process *process, struct node *var_node, void *private, int offset)
{
    return resolver_create_new_entity_for_var_node_custom_scope(process, var_node, private, resolver_scope_current(process), offset);
}

struct resolver_entity *resolver_new_entity_for_var_node_no_push(struct resolver_process *process, struct node *var_node, void *private, int offset, struct resolver_scope *scope)
{
    struct resolver_entity *entity = resolver_create_new_entity_for_var_node_custom_scope(process, var_node, private, scope, offset);
    if (!entity)
    {
        return NULL;
    }

    if (scope->flags & RESOLVER_SCOPE_FLAG_IS_STACK)
    {
        entity->flags |= RESOLVER_ENTITY_FLAG_IS_STACK;
    }

    return entity;
}

struct resolver_entity *resolver_new_entity_for_var_node(struct resolver_process *process, struct node *var_node, void *private, int offset)
{
    struct resolver_entity *entity = resolver_new_entity_for_var_node_no_push(process, var_node, private, offset, resolver_process_scope_current(process));
    if (!entity)
    {
        return NULL;
    }

    vector_push(process->scope.current->entities, &entity);
    return entity;
}

void resolver_new_entity_for_rule(struct resolver_process *process, struct resolver_result *result, struct resolver_entity_rule *rule)
{
    struct resolver_entity *entity_rule = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_RULE, NULL);
    entity_rule->rule = *rule;
    resolver_result_entity_push(result, entity_rule);
}

struct resolver_entity *resolver_make_entity(struct resolver_process *process, struct resolver_result *result, struct datatype *custom_dtype, struct node *node, struct resolver_entity *guided_entity, struct resolver_scope *scope)
{
    struct resolver_entity *entity = NULL;
    int offset = guided_entity->offset;
    int flags = guided_entity->flags;
    switch (node->type)
    {
    case NODE_TYPE_VARIABLE:
        entity = resolver_new_entity_for_var_node_no_push(process, node, NULL, offset, scope);
        break;

    default:
        entity = resolver_create_new_unknown_entity(process, result, custom_dtype, node, scope, offset);
    }

    if (entity)
    {
        entity->flags |= flags;
        if (custom_dtype)
        {
            entity->dtype = *custom_dtype;
        }

        entity->private = process->callbacks.make_private(entity, node, offset, scope);
    }
    return entity;
}

struct resolver_entity *resolver_create_new_entity_for_function_call(struct resolver_result *result, struct resolver_process *process, struct resolver_entity *left_operand_entity, void *private)
{
    struct resolver_entity *entity = resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_FUNCTION_CALL, private);
    if (!entity)
    {
        return NULL;
    }

    entity->dtype = left_operand_entity->dtype;
    entity->func_call_data.arguments = vector_create(sizeof(struct node *));
    return entity;
}

struct resolver_entity *resolver_regster_function(struct resolver_process *process, struct node *func_node, void *private)
{
    struct resolver_entity *entity = resolver_create_new_entity(NULL, RESOLVER_ENTITY_TYPE_FUNCTION, private);
    if (!entity)
    {
        return NULL;
    }

    entity->name = func_node->func.name;
    entity->node = func_node;
    entity->dtype = func_node->func.rtype;
    entity->scope = resolver_process_scope_current(process);
    vector_push(process->scope.root->entities, &entity);
    return entity;
}

struct resolver_entity *resolver_get_entity_in_scope_with_entity_type(struct resolver_result *result, struct resolver_process *resolver, struct resolver_scope *scope, const char *entity_name, int entity_type)
{

    /*
     * Are we accessing a structure or union
     */
    if (result && result->last_struct_union_entity)
    {
        struct resolver_scope *scope = result->last_struct_union_entity->scope;
        struct node *out_node = NULL;
        struct datatype *node_var_datatype = &result->last_struct_union_entity->dtype;
        int offset = struct_offset(resolver_compiler(resolver), node_var_datatype->type_str, entity_name, &out_node, 0, 0);
        if (node_var_datatype->type == DATA_TYPE_UNION)
        {
            offset = 0;
        }
        return resolver_make_entity(resolver, result, NULL, out_node, &(struct resolver_entity){.type = RESOLVER_ENTITY_TYPE_VARIABLE, .offset = offset}, scope);
    }

    // Dealing with a primtiive type
    vector_set_peek_pointer_end(scope->entities);
    vector_set_flag(scope->entities, VECTOR_FLAG_PEEK_DECREMENT);
    struct resolver_entity *current = vector_peek_ptr(scope->entities);
    while (current)
    {
        if (entity_type != -1 && current->type != entity_type)
        {
            current = vector_peek_ptr(scope->entities);
            continue;
        }

        if (S_EQ(current->name, entity_name))
        {
            break;
        }

        current = vector_peek_ptr(scope->entities);
    }

    return current;
}

struct resolver_entity *resolver_get_entity_for_type(struct resolver_result *result, struct resolver_process *resolver, const char *entity_name, int entity_type)
{
    struct resolver_scope *scope = resolver->scope.current;
    struct resolver_entity *entity = NULL;
    while (scope)
    {
        entity = resolver_get_entity_in_scope_with_entity_type(result, resolver, scope, entity_name, entity_type);
        if (entity)
        {
            break;
        }

        scope = scope->prev;
    }

    if (entity)
    {
        memset(&entity->last_resolve, 0, sizeof(entity->last_resolve));
    }
    return entity;
}

struct resolver_entity *resolver_get_entity(struct resolver_result *result, struct resolver_process *resolver, const char *entity_name)
{
    return resolver_get_entity_for_type(result, resolver, entity_name, -1);
}

struct resolver_entity *resolver_get_entity_in_scope(struct resolver_result *result, struct resolver_process *resolver, struct resolver_scope *scope, const char *entity_name)
{
    return resolver_get_entity_in_scope_with_entity_type(result, resolver, scope, entity_name, -1);
}

struct resolver_entity *resolver_get_variable(struct resolver_result *result, struct resolver_process *resolver, const char *var_name)
{
    return resolver_get_entity_for_type(result, resolver, var_name, RESOLVER_ENTITY_TYPE_VARIABLE);
}

struct resolver_entity *resolver_get_function_in_scope(struct resolver_result *result, struct resolver_process *resolver, const char *func_name, struct resolver_scope *scope)
{
    return resolver_get_entity_for_type(result, resolver, func_name, RESOLVER_ENTITY_TYPE_FUNCTION);
}

struct resolver_entity *resolver_get_function(struct resolver_result *result, struct resolver_process *resolver, const char *func_name)
{
    struct resolver_entity *entity = NULL;
    struct resolver_scope *scope = resolver->scope.root;
    entity = resolver_get_function_in_scope(result, resolver, func_name, scope);
    return entity;
}

struct resolver_entity *resolver_follow_for_name(struct resolver_process *resolver, const char *name, struct resolver_result *result)
{
    struct resolver_entity *entity = resolver_entity_clone(resolver_get_entity(result, resolver, name));
    if (!entity)
    {
        return NULL;
    }

    resolver_result_entity_push(result, entity);

    // The first found identifier
    if (!result->identifier)
    {
        result->identifier = entity;
    }

    if (entity->type == RESOLVER_ENTITY_TYPE_VARIABLE && datatype_is_struct_or_union(&entity->var_data.dtype) ||
                                                             (entity->type == RESOLVER_ENTITY_TYPE_FUNCTION && datatype_is_struct_or_union(&entity->dtype)))
    {
        result->last_struct_union_entity = entity;
    }

    return entity;
}

struct resolver_entity *resolver_follow_identifier(struct resolver_process *resolver, struct node *node, struct resolver_result *result)
{
    struct resolver_entity *entity = resolver_follow_for_name(resolver, node->sval, result);
    if (entity)
    {
        entity->last_resolve.referencing_node = node;
    }
    return entity;
}

struct resolver_entity* resolver_follow_variable(struct resolver_process* resolver, struct node* var_node, struct resolver_result* result)
{
    struct resolver_entity* entity = resolver_follow_for_name(resolver, var_node->var.name, result);
    return entity;
}

struct resolver_entity* resolver_follow_struct_exp(struct resolver_process* resolver, struct node* node, struct resolver_result* result)
{
    // a.b
    resolver_follow_part(resolver, node->exp.left, result);
    struct resolver_entity* left_entity = resolver_result_peek(result);
    struct resolver_entity_rule rule = {};
    if (is_access_node_with_op(node, "->"))
    {
        //a->b (do not merge)
        rule.left.flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
        if (left_entity->type != RESOLVER_ENTITY_TYPE_FUNCTION_CALL)
        {
//            int* a;
  //          *a = 50;
            rule.right.flags = RESOLVER_ENTITY_FLAG_DO_INDIRECTION;
        }
    }

    resolver_new_entity_for_rule(resolver, result, &rule);
    resolver_follow_part(resolver, node->exp.right, result);
    return NULL;
}

struct resolver_entity* resolver_follow_exp(struct resolver_process* resolver, struct node* node, struct resolver_result* result)
{
    struct resolver_entity* entity = NULL;
    if (is_access_node(node))
    {
        entity = resolver_follow_struct_exp(resolver, node, result);
    }
}
struct resolver_entity *resolver_follow_part_return_entity(struct resolver_process *resolver, struct node *node, struct resolver_result *result)
{
    struct resolver_entity *entity = NULL;
    switch (node->type)
    {
    case NODE_TYPE_IDENTIFIER:
        entity = resolver_follow_identifier(resolver, node, result);
        break;
    case NODE_TYPE_VARIABLE:
        entity = resolver_follow_variable(resolver, node, result);
        break;

    case NODE_TYPE_EXPRESSION:
        entity = resolver_follow_exp(resolver, node, result);
        break;
    }
}

void resolver_follow_part(struct resolver_process *resolver, struct node *node, struct resolver_result *result)
{
    resolver_follow_part_return_entity(resolver, node, result);
}

void resolver_execute_rules(struct resolver_process *resolver, struct resolver_result *result)
{
}

void resolver_merge_compile_times(struct resolver_process *resolver, struct resolver_result *result)
{
}

void resolver_finalize_result(struct resolver_process *resolver, struct resolver_result *result)
{
}
struct resolver_result *resolver_follow(struct resolver_process *resolver, struct node *node)
{
    assert(resolver);
    assert(node);
    struct resolver_result *result = resolver_new_result(resolver);
    resolver_follow_part(resolver, node, result);
    if (!resolver_result_entity_root(result))
    {
        result->flags |= RESOLVER_RESULT_FLAG_FAILED;
    }
    resolver_execute_rules(resolver, result);
    resolver_merge_compile_times(resolver, result);
    resolver_finalize_result(resolver, result);
    return result;
}
