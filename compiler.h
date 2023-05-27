#ifndef PEACHCOMPILER_H
#define PEACHCOMPILER_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <linux/limits.h>
#include <assert.h>

#define FAIL_ERR(message) assert(0 == 1 && message)

#define S_EQ(str, str2) \
    (str && str2 && (strcmp(str, str2) == 0))

struct pos
{
    int line;
    int col;
    const char *filename;
};

// In C We have a stack alignment of 16 bytes.
#define C_STACK_ALIGNMENT 16
#define STACK_PUSH_SIZE 4
#define C_ALIGN(size) (size % C_STACK_ALIGNMENT) ? size + (C_STACK_ALIGNMENT - (size % C_STACK_ALIGNMENT)) : size

#define NUMERIC_CASE \
    case '0':        \
    case '1':        \
    case '2':        \
    case '3':        \
    case '4':        \
    case '5':        \
    case '6':        \
    case '7':        \
    case '8':        \
    case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+':                            \
    case '-':                            \
    case '*':                            \
    case '>':                            \
    case '<':                            \
    case '^':                            \
    case '%':                            \
    case '!':                            \
    case '=':                            \
    case '~':                            \
    case '|':                            \
    case '&':                            \
    case '(':                            \
    case '[':                            \
    case ',':                            \
    case '.':                            \
    case '?'

#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case ':':       \
    case ';':       \
    case '#':       \
    case '\\':      \
    case ')':       \
    case ']'
enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_INPUT_ERROR
};

enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

enum
{
    NUMBER_TYPE_NORMAL,
    NUMBER_TYPE_LONG,
    NUMBER_TYPE_FLOAT,
    NUMBER_TYPE_DOUBLE
};

enum
{
    TOKEN_FLAG_IS_CUSTOM_OPERATOR = 0b00000001
};

struct token
{
    int type;
    int flags;
    struct pos pos;
    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void *any;
    };

    struct token_number
    {
        int type;
    } num;

    // True if their is whitespace between the token and the next token
    // i.e * a for operator token * would mean whitespace would be set for token "a"
    bool whitespace;

    // (5+10+20)
    const char *between_brackets;

    // Anything between function call arguments. ABC(hello world)
    const char* between_arguments;
};

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *process, char c);

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process
{
    struct pos pos;
    struct vector *token_vec;
    struct compile_process *compiler;

    /**
     *
     * ((50))
     */
    int current_expression_count;
    struct buffer *parentheses_buffer;

    struct buffer* argument_string_buffer;
    struct lex_process_functions *function;

    // This will be private data that the lexer does not understand
    // but the person using the lexer does understand.
    void *private;
};

enum
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

enum
{
    COMPILE_PROCESS_EXECUTE_NASM = 0b00000001,
    COMPILE_PROCESS_EXPORT_AS_OBJECT = 0b00000010,
};

struct scope
{
    int flags;

    // void*
    struct vector *entities;

    // The total number of bytes this scope uses. Aligned to 16 bytes.
    size_t size;

    // NULL if no parent.
    struct scope *parent;
};

enum
{
    SYMBOL_TYPE_NODE,
    SYMBOL_TYPE_NATIVE_FUNCTION,
    SYMBOL_TYPE_UNKNOWN
};

struct symbol
{
    const char *name;
    int type;
    void *data;
};

struct codegen_entry_point
{
    // ID of the entry point
    int id;
};

struct codegen_exit_point
{
    // ID of the exit point
    int id;
};

struct string_table_element
{
    // This is the string that the element is related too. "Hello world"
    const char *str;

    // This is the assembly label that points to the memory
    // where the string can be found.
    const char label[50];
};

struct code_generator
{

    struct generator_switch_stmt
    {
        struct generator_switch_stmt_entity
        {
            int id;
        } current;

        // Vector of generatr_switch_stmt_entity
        struct vector* swtiches;
    } _switch;

    // A vector of struct string_table_element*
    struct vector *string_table;

    // vector of struct codegen_entry_point*
    struct vector *entry_points;
    // vector of struct codegen_exit_point*
    struct vector *exit_points;

    // Vector of const char* that will go in the data section
    struct vector* custom_data_section;

    // vector of struct response*
    struct vector *responses;
};

struct resolver_process;

struct generator;
struct native_function;
struct node;
struct resolver_entity;
struct datatype;

struct generator_entity_address
{
    bool is_stack;
    long offset;
    const char* address;
    const char* base_address;
};

#define GENERATOR_BEGIN_EXPRESSION(gen)
#define GENERATOR_END_EXPRESSION(gen) gen->end_exp(gen)


typedef void(*ASM_PUSH_PROTOTYPE)(const char* ins, ...);
typedef void (*NATIVE_FUNCTION_CALL)(struct generator* generator, struct native_function* func, struct vector* arguments);
typedef void(*GENERATOR_GENERATE_EXPRESSION)(struct generator* generator, struct node* node, int flags);
typedef void (*GENERATOR_ENTITY_ADDRESS)(
    struct generator* generator, struct resolver_entity* entity, 
    struct generator_entity_address* address_out);
typedef void(*GENERATOR_END_EXPRESSION)(struct generator* generator);

typedef void(*GENERATOR_FUNCTION_RETURN)(struct datatype* dtype, const char* fmt, ...);

struct generator
{
    ASM_PUSH_PROTOTYPE asm_push;
    GENERATOR_GENERATE_EXPRESSION gen_exp;
    GENERATOR_END_EXPRESSION end_exp;
    GENERATOR_ENTITY_ADDRESS entity_address;
    GENERATOR_FUNCTION_RETURN ret;

    struct compile_process* compiler;


    // Private data for the generator
    void* private;
};

struct native_function_callbacks
{
    NATIVE_FUNCTION_CALL call;
};

struct native_function
{
    const char* name;
    struct native_function_callbacks callbacks;
};

struct symbol* native_create_function(struct compile_process* compiler, const char* name,
 struct native_function_callbacks* callbacks);

struct native_function* native_function_get(struct compile_process* compiler, const char* name);

struct preprocessor;
struct preprocessor_definition;
struct preprocessor_function_argument;
struct preprocessor_included_file;


typedef void (*PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION)(struct preprocessor* preprocessor, struct preprocessor_included_file* included_file);

enum
{
    PREPROCESSOR_DEFINITION_STANDARD,
    PREPROCESSOR_DEFINITION_MACRO_FUNCTION,
    PREPROCESSOR_DEFINITION_NATIVE_CALLBACK,
    PREPROCESSOR_DEFINITION_TYPEDEF
};

struct preprocessor;
struct preprocessor_definition;
struct preprocessor_function_argument
{
    // Tokens for this argument struct token
    struct vector* tokens;
};

struct preprocessor_function_arguments
{
    // Vector of struct preprocessor_function_argument
    struct vector* arguments;
};

typedef int (*PREPROCESSOR_DEFINITION_NATIVE_CALL_EVALUATE)(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments);
typedef struct vector* (*PREPROCESSOR_DEFINITION_NATIVE_CALL_VALUE)(struct preprocessor_definition* definition, struct preprocessor_function_arguments* arguments);

struct preprocessor_definition
{
    // I.e standard or macro function
    int type;

    // The name i.e #define ABC ABC is the name
    const char* name;
    union 
    {
        struct standard_preprocessor_definition
        {
            // A vecor of definition value tokens. Values can be multiple lines
            // vector of struct token
            struct vector* value;

            // A vector of const char* representing function arguments in order
            // for example: ABC(a, b, c) 
            struct vector* arguments;
        } standard;

        struct typedef_preprocessor_definition
        {
            struct vector* value;
        } _typedef;

        struct native_callback_preprocessor_definition
        {
            PREPROCESSOR_DEFINITION_NATIVE_CALL_EVALUATE evaluate;
            PREPROCESSOR_DEFINITION_NATIVE_CALL_VALUE value;
        } native;;
    };
    
    struct preprocessor* preprocessor;
};

struct preprocessor_included_file
{
    char filename[PATH_MAX];
};


// Function pointers
typedef void(*PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION)(struct preprocessor* preprocessor, struct preprocessor_included_file* included_file);
PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION preprocessor_static_include_handler_for(const char* filename);

struct preprocessor
{
    // A vector of struct preprocessor_definition*
    struct vector* definitions;
    // Vector of struct preprocessor_node*
    struct vector* exp_vector;

    struct expressionable* expressionable;

    struct compile_process* compiler;

    // A vector of included files struct preprocessor_included_file*
    struct vector* includes;
};

struct preprocessor* preprocessor_create(struct compile_process* compiler);
int preprocessor_run(struct compile_process* compiler);


struct compile_process
{
    // The flags in regards to how this file should be compiled
    int flags;

    struct pos pos;
    struct compile_process_input_file
    {
        FILE *fp;
        const char *abs_path;
    } cfile;

    // Untampered token vector, contains definitions, and source code tokens, the preprocessor
    // will go through this vector and populate the "token_vec" vector after it is done.
    struct vector* token_vec_original;

    // A vector of tokens from lexical analysis.
    struct vector *token_vec;

    struct vector *node_vec;
    struct vector *node_tree_vec;
    FILE *ofile;

    struct
    {
        struct scope *root;
        struct scope *current;
    } scope;

    struct
    {
        // Current active symbol table. struct symbol*
        struct vector *table;

        // struct vector* multiple symbol tables stored in here..
        struct vector *tables;
    } symbols;

    // Pointer to our codegenerator.
    struct code_generator *generator;
    struct resolver_process *resolver;

    // A vector of const char* that represents include directories.
    struct vector* include_dirs;
    struct preprocessor* preprocessor;
};

enum
{
    PARSE_ALL_OK,
    PARSE_GENERAL_ERROR
};

enum
{
    VALIDATION_ALL_OK,
    VALIDATION_GENERAL_ERROR
};

enum
{
    CODEGEN_ALL_OK,
    CODEGEN_GENERAL_ERROR
};

enum
{
    NODE_TYPE_EXPRESSION,
    NODE_TYPE_EXPRESSION_PARENTHESES,
    NODE_TYPE_NUMBER,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_STRING,
    NODE_TYPE_VARIABLE,
    NODE_TYPE_VARIABLE_LIST,
    NODE_TYPE_FUNCTION,
    NODE_TYPE_BODY,
    NODE_TYPE_STATEMENT_RETURN,
    NODE_TYPE_STATEMENT_IF,
    NODE_TYPE_STATEMENT_ELSE,
    NODE_TYPE_STATEMENT_WHILE,
    NODE_TYPE_STATEMENT_DO_WHILE,
    NODE_TYPE_STATEMENT_FOR,
    NODE_TYPE_STATEMENT_BREAK,
    NODE_TYPE_STATEMENT_CONTINUE,
    NODE_TYPE_STATEMENT_SWITCH,
    NODE_TYPE_STATEMENT_CASE,
    NODE_TYPE_STATEMENT_DEFAULT,
    NODE_TYPE_STATEMENT_GOTO,

    NODE_TYPE_UNARY,
    NODE_TYPE_TENARY,
    NODE_TYPE_LABEL,
    NODE_TYPE_STRUCT,
    NODE_TYPE_UNION,
    NODE_TYPE_BRACKET,
    NODE_TYPE_CAST,
    NODE_TYPE_BLANK
};

enum
{
    NODE_FLAG_INSIDE_EXPRESSION = 0b00000001,
    NODE_FLAG_IS_FORWARD_DECLARATION = 0b00000010,
    NODE_FLAG_HAS_VARIABLE_COMBINED = 0b00000100
};

struct array_brackets
{
    // Vector of struct node*
    struct vector *n_brackets;
};

struct node;
struct datatype
{
    int flags;
    // i.e type of long, int, float ect..
    int type;

    // i.e long int. int being the secondary.
    struct datatype *secondary;
    // long
    const char *type_str;
    // The sizeof the datatype.
    size_t size;
    int pointer_depth;

    union
    {
        struct node *struct_node;
        struct node *union_node;
    };

    struct array
    {
        struct array_brackets *brackets;
        /**
         *
         * The total array size: Equation = DATATYPE_SIZE * EACH_INDEX
         */
        size_t size;
    } array;
};

struct parsed_switch_case
{
    // Index of the parsed case
    int index;
};

struct stack_frame_data
{
    /*
     * The datatype that was pushed to the stack
     */
    struct datatype dtype;
};

struct stack_frame_element
{
    // Stack frame element flags
    int flags;
    // The type of frame element
    int type;
    /**
     * The name of the frame element, not a variable name. I.e result_value
     */
    const char *name;

    // The offset this element is on the base pointer
    int offset_from_bp;

    struct stack_frame_data data;
};

#define STACK_PUSH_SIZE 4
enum
{
    STACK_FRAME_ELEMENT_TYPE_LOCAL_VARIABLE,
    STACK_FRAME_ELEMENT_TYPE_SAVED_REGISTER,
    STACK_FRAME_ELEMENT_TYPE_SAVED_BP,
    STACK_FRAME_ELEMENT_TYPE_PUSHED_VALUE,
    STACK_FRAME_ELEMENT_TYPE_UNKNOWN,
};

enum
{
    STACK_FRAME_ELEMENT_FLAG_IS_PUSHED_ADDRESS = 0b00000001,
    STACK_FRAME_ELEMENT_FLAG_ELEMENT_NOT_FOUND = 0b00000010,
    STACK_FRAME_ELEMENT_FLAG_IS_NUMERICAL = 0b00000100,
    STACK_FRAME_ELEMENT_FLAG_HAS_DATATYPE = 0b00001000,
};

void stackframe_pop(struct node *func_node);
struct stack_frame_element *stackframe_back(struct node *func_node);
struct stack_frame_element *stackframe_back_expect(struct node *func_node, int expecting_type, const char *expecting_name);
void stackframe_pop_expecting(struct node *func_node, int expecting_type, const char *expecting_name);
void stackframe_peek_start(struct node *func_node);
struct stack_frame_element *stackframe_peek(struct node *func_node);
void stackframe_push(struct node *func_node, struct stack_frame_element *element);
void stackframe_sub(struct node *func_node, int type, const char *name, size_t amount);
void stackframe_add(struct node *func_node, int type, const char *name, size_t amount);
void stackframe_assert_empty(struct node *func_node);

enum
{
    UNARY_FLAG_IS_LEFT_OPERANDED_UNARY = 0b00000001,
};

struct node;
struct unary
{
    int flags;
    // "*" for pointer access.. **** even for multiple pointer access only the first operator
    // is here
    const char *op;
    struct node *operand;
    union
    {
        struct indirection
        {
            // The pointer depth
            int depth;
        } indirection;
    };
};
struct node
{
    int type;
    int flags;

    struct pos pos;

    struct node_binded
    {
        // Pointer to our body node
        struct node *owner;

        // Pointer to the function this node is in.
        struct node *function;
    } binded;

    union
    {
        struct exp
        {
            struct node *left;
            struct node *right;
            const char *op;
        } exp;

        struct parenthesis
        {
            // The expression inside the parenthesis node.
            struct node *exp;
        } parenthesis;

        struct var
        {
            struct datatype type;
            int padding;
            // Aligned offset
            int aoffset;
            const char *name;
            struct node *val;
        } var;

        struct node_tenary
        {
            struct node *true_node;
            struct node *false_node;
        } tenary;

        struct varlist
        {
            // A list of struct node* variables.
            struct vector *list;
        } var_list;

        struct bracket
        {
            // int x[50]; [50] would be our bracket node. The inner would NODE_TYPE_NUMBER value of 50
            struct node *inner;
        } bracket;

        struct _struct
        {
            const char *name;
            struct node *body_n;

            /**
             * struct abc
             * {
             *
             * } var_name;
             *
             * NULL if no variable attached to structure.
             *
             */
            struct node *var;
        } _struct;

        struct _union
        {
            const char *name;
            struct node *body_n;

            /**
             * struct abc
             * {
             *
             * } var_name;
             *
             * NULL if no variable attached to structure.
             *
             */
            struct node *var;
        } _union;

        struct body
        {
            /**
             * struct node* vector of statements
             */
            struct vector *statements;

            // The size of combined variables inside this body.
            size_t size;

            // True if the variable size had to be increased due to padding in the body.
            bool padded;

            // A pointer to the largest variable node in the statements vector.
            struct node *largest_var_node;
        } body;

        struct function
        {
            // Special flags
            int flags;
            // Return type i.e void, int, long ect...
            struct datatype rtype;

            // I.e function name "main"
            const char *name;

            struct function_arguments
            {
                // Vector of struct node* . Must be type NODE_TYPE_VARIABLE
                struct vector *vector;

                // How much to add to the EBP to find the first argument.
                size_t stack_addition;
            } args;

            // Pointer to the function body node, NULL if this is a function prototype
            struct node *body_n;

            struct stack_frame
            {
                // A vector of stack_frame_element
                struct vector *elements;
            } frame;

            // The stack size for all variables inside this function.
            size_t stack_size;
        } func;

        struct statement
        {

            struct return_stmt
            {
                // The expression of the return
                struct node *exp;
            } return_stmt;

            struct if_stmt
            {
                // if(COND) {// body }
                struct node *cond_node;
                struct node *body_node;

                // if(COND) {} else {}
                struct node *next;
            } if_stmt;

            struct else_stmt
            {
                struct node *body_node;
            } else_stmt;

            struct for_stmt
            {
                struct node *init_node;
                struct node *cond_node;
                struct node *loop_node;
                struct node *body_node;
            } for_stmt;

            struct while_stmt
            {
                struct node *exp_node;
                struct node *body_node;
            } while_stmt;

            struct do_while_stmt
            {
                struct node *exp_node;
                struct node *body_node;
            } do_while_stmt;

            struct switch_stmt
            {
                struct node *exp;
                struct node *body;
                struct vector *cases;
                bool has_default_case;
            } switch_stmt;

            struct _case_stmt
            {
                struct node *exp;
            } _case;

            struct _goto_stmt
            {
                struct node *label;
            } _goto;
        } stmt;

        struct node_label
        {
            struct node *name;
        } label;

        struct cast
        {
            struct datatype dtype;
            struct node *operand;
        } cast;

        struct unary unary;
    };

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
    };
};

enum
{
    RESOLVER_ENTITY_FLAG_IS_STACK = 0b00000001,
    RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY = 0b00000010,
    RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY = 0b00000100,
    RESOLVER_ENTITY_FLAG_DO_INDIRECTION = 0b00001000,
    RESOLVER_ENTITY_FLAG_JUST_USE_OFFSET = 0b00010000,
    RESOLVER_ENTITY_FLAG_IS_POINTER_ARRAY_ENTITY = 0b00100000,
    RESOLVER_ENTITY_FLAG_WAS_CASTED = 0b01000000,
    RESOLVER_ENTITY_FLAG_USES_ARRAY_BRACKETS = 0b10000000
};

enum
{
    RESOLVER_ENTITY_TYPE_VARIABLE,
    RESOLVER_ENTITY_TYPE_FUNCTION,
    RESOLVER_ENTITY_TYPE_NATIVE_FUNCTION,
    RESOLVER_ENTITY_TYPE_STRUCTURE,
    RESOLVER_ENTITY_TYPE_FUNCTION_CALL,
    RESOLVER_ENTITY_TYPE_ARRAY_BRACKET,
    RESOLVER_ENTITY_TYPE_RULE,
    RESOLVER_ENTITY_TYPE_GENERAL,
    RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS,
    RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION,
    RESOLVER_ENTITY_TYPE_UNSUPPORTED,
    RESOLVER_ENTITY_TYPE_CAST
};

enum
{
    RESOLVER_SCOPE_FLAG_IS_STACK = 0b00000001
};

struct resolver_result;
struct resolver_process;
struct resolver_scope;
struct resolver_entity;

typedef void *(*RESOLVER_NEW_ARRAY_BRACKET_ENTITY)(struct resolver_result *result, struct node *array_entity_node);
typedef void (*RESOLVER_DELETE_SCOPE)(struct resolver_scope *scope);
typedef void (*RESOLVER_DELETE_ENTITY)(struct resolver_entity *entity);
typedef struct resolver_entity *(*RESOLVER_MERGE_ENTITIES)(struct resolver_process *process, struct resolver_result *result, struct resolver_entity *left_entity, struct resolver_entity *right_entity);
typedef void *(*RESOLVER_MAKE_PRIVATE)(struct resolver_entity *entity, struct node *node, int offset, struct resolver_scope *scope);
typedef void (*RESOLVER_SET_RESULT_BASE)(struct resolver_result *result, struct resolver_entity *base_entity);
struct resolver_callbacks
{
    RESOLVER_NEW_ARRAY_BRACKET_ENTITY new_array_entity;
    RESOLVER_DELETE_SCOPE delete_scope;
    RESOLVER_DELETE_ENTITY delete_entity;
    RESOLVER_MERGE_ENTITIES merge_entities;
    RESOLVER_MAKE_PRIVATE make_private;
    RESOLVER_SET_RESULT_BASE set_result_base;
};

struct compile_process;
struct resolver_process
{
    struct resolver_scopes
    {
        struct resolver_scope *root;
        struct resolver_scope *current;
    } scope;

    struct compile_process *compiler;
    struct resolver_callbacks callbacks;
};
struct resolver_array_data
{
    // Holds nodes of type resolver_entity
    struct vector *array_entities;
};

enum
{
    RESOLVER_DEFAULT_ENTITY_TYPE_STACK,
    RESOLVER_DEFAULT_ENTITY_TYPE_SYMBOL
};

enum
{
    RESOLVER_DEFAULT_ENTITY_FLAG_IS_LOCAL_STACK = 0b00000001
};

enum
{
    RESOLVER_DEFAULT_ENTITY_DATA_TYPE_VARIABLE,
    RESOLVER_DEFAULT_ENTITY_DATA_TYPE_FUNCTION,
    RESOLVER_DEFAULT_ENTITY_DATA_TYPE_ARRAY_BRACKET,
};
struct resolver_default_entity_data
{
    // i.e variable, function, structure
    int type;
    // This is the address [ebp-4], [var_name+4]
    char address[60];
    // ebp, var_name
    char base_address[60];
    // -4
    int offset;
    // Flags relating to the entity data
    int flags;
};

struct resolver_default_scope_data
{
    int flags;
};

enum
{
    RESOLVER_RESULT_FLAG_FAILED = 0b00000001,
    RESOLVER_RESULT_FLAG_RUNTIME_NEEDED_TO_FINISH_PATH = 0b00000010,
    RESOLVER_RESULT_FLAG_PROCESSING_ARRAY_ENTITIES = 0b00000100,
    RESOLVER_RESULT_FLAG_HAS_POINTER_ARRAY_ACCESS = 0b00001000,
    RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX = 0b00010000,
    RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE = 0b00100000,
    RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE = 0b01000000,
    RESOLVER_RESULT_FLAG_DOES_GET_ADDRESS = 0b10000000
};

struct resolver_result
{
    // This is the first entity in our resolver result
    struct resolver_entity *first_entity_const;

    // This entity represents the variable at the start of this expression.
    struct resolver_entity *identifier;

    // Equal to the last structure or union entity discovered.
    struct resolver_entity *last_struct_union_entity;

    struct resolver_array_data array_data;

    // The root entity of our result
    struct resolver_entity *entity;
    // The last entity of our result.
    struct resolver_entity *last_entity;
    int flags;
    // The total number of entities
    size_t count;

    struct resolver_result_base
    {
        // [ebp-4], [name+4]
        char address[60];
        // EBP, global_variable_name
        char base_address[60];
        // -4
        int offset;
    } base;
};
struct resolver_scope
{
    // Resolver scope flags
    int flags;
    struct vector *entities;
    struct resolver_scope *next;
    struct resolver_scope *prev;

    // Private data for the resolver scope.
    void *private;
};

struct resolver_entity
{
    int type;
    int flags;

    // The name of the resolved entity
    // i.e function name, variable name ect ect
    const char *name;

    // The offset from the stack EBP+(offset)
    int offset;

    // This is the node that the entity is relating to.
    struct node *node;

    union
    {
        struct resolver_entity_var_data
        {
            struct datatype dtype;
            struct resolver_array_runtime_
            {
                struct datatype dtype;
                struct node *index_node;
                int multiplier;
            } array_runtime;
        } var_data;

        struct resolver_array
        {
            struct datatype dtype;
            struct node *array_index_node;
            int index;
        } array;

        struct resolver_entity_function_call_data
        {
            // struct node* vector
            struct vector *arguments;
            // The total bytes used by the function call.
            size_t stack_size;
        } func_call_data;

        struct resolver_entity_rule
        {
            struct resolver_entity_rule_left
            {
                int flags;
            } left;

            struct resolver_entity_rule_right
            {
                int flags;
            } right;
        } rule;

        struct resolver_indirection
        {
            // How much is the depth we need to find the value.
            int depth;
        } indirection;

        struct resolver_native_function
        {
            struct symbol* symbol;
        } native_func;
    };

    struct entity_last_resolve
    {

        struct node *referencing_node;
    } last_resolve;

    // The datatype of the resolver entity
    struct datatype dtype;

    // The scope that this entity belongs to
    struct resolver_scope *scope;

    // The result of the resolution
    struct resolver_result *result;

    // The resolver process
    struct resolver_process *resolver;

    // Private data that only the resolver entity creator knows about.
    void *private;

    // The next entity
    struct resolver_entity *next;
    // The previous entity
    struct resolver_entity *prev;
};
enum
{
    DATATYPE_FLAG_IS_SIGNED = 0b00000001,
    DATATYPE_FLAG_IS_STATIC = 0b00000010,
    DATATYPE_FLAG_IS_CONST = 0b00000100,
    DATATYPE_FLAG_IS_POINTER = 0b00001000,
    DATATYPE_FLAG_IS_ARRAY = 0b00010000,
    DATATYPE_FLAG_IS_EXTERN = 0b00100000,
    DATATYPE_FLAG_IS_RESTRICT = 0b01000000,
    DATATYPE_FLAG_IGNORE_TYPE_CHECKING = 0b10000000,
    DATATYPE_FLAG_IS_SECONDARY = 0b100000000,
    DATATYPE_FLAG_STRUCT_UNION_NO_NAME = 0b1000000000,
    DATATYPE_FLAG_IS_LITERAL = 0b10000000000,
};
enum
{
    DATA_TYPE_VOID,
    DATA_TYPE_CHAR,
    DATA_TYPE_SHORT,
    DATA_TYPE_INTEGER,
    DATA_TYPE_LONG,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_STRUCT,
    DATA_TYPE_UNION,
    DATA_TYPE_UNKNOWN
};

enum
{
    DATA_TYPE_EXPECT_PRIMITIVE,
    DATA_TYPE_EXPECT_UNION,
    DATA_TYPE_EXPECT_STRUCT
};

enum
{
    DATA_SIZE_ZERO = 0,
    DATA_SIZE_BYTE = 1,
    DATA_SIZE_WORD = 2,
    DATA_SIZE_DWORD = 4,
    DATA_SIZE_DDWORD = 8
};

enum
{
    EXPRESSION_FLAG_RIGHT_NODE = 0b0000000000000001,
    EXPRESSION_IN_FUNCTION_CALL_ARGUMENTS = 0b0000000000000010,
    EXPRESSION_IN_FUNCTION_CALL_LEFT_OPERAND = 0b0000000000000100,
    EXPRESSION_IS_ADDITION = 0b0000000000001000,
    EXPRESSION_IS_SUBTRACTION = 0b0000000000010000,
    EXPRESSION_IS_MULTIPLICATION = 0b0000000000100000,
    EXPRESSION_IS_DIVISION = 0b0000000001000000,
    EXPRESSION_IS_FUNCTION_CALL = 0b0000000010000000,
    EXPRESSION_INDIRECTION = 0b0000000100000000,
    EXPRESSION_GET_ADDRESS = 0b0000001000000000,
    EXPRESSION_IS_ABOVE = 0b0000010000000000,
    EXPRESSION_IS_ABOVE_OR_EQUAL = 0b0000100000000000,
    EXPRESSION_IS_BELOW = 0b0001000000000000,
    EXPRESSION_IS_BELOW_OR_EQUAL = 0b0010000000000000,
    EXPRESSION_IS_EQUAL = 0b0100000000000000,
    EXPRESSION_IS_NOT_EQUAL = 0b1000000000000000,
    EXPRESSION_LOGICAL_AND = 0b10000000000000000,
    EXPRESSION_LOGICAL_OR = 0b100000000000000000,
    EXPRESSION_IN_LOGICAL_EXPRESSION = 0b1000000000000000000,
    EXPRESSION_IS_BITSHIFT_LEFT = 0b10000000000000000000,
    EXPRESSION_IS_BITSHIFT_RIGHT = 0b100000000000000000000,
    EXPRESSION_IS_BITWISE_OR = 0b1000000000000000000000,
    EXPRESSION_IS_BITWISE_AND = 0b10000000000000000000000,
    EXPRESSION_IS_BITWISE_XOR = 0b100000000000000000000000,
    EXPRESSION_IS_NOT_ROOT_NODE = 0b1000000000000000000000000,
    EXPRESSION_IS_ASSIGNMENT = 0b10000000000000000000000000,
    IS_ALONE_STATEMENT = 0b100000000000000000000000000,
    EXPRESSION_IS_UNARY = 0b1000000000000000000000000000,
    IS_STATEMENT_RETURN = 0b10000000000000000000000000000,
    IS_RIGHT_OPERAND_OF_ASSIGNMENT = 0b100000000000000000000000000000,
    IS_LEFT_OPERAND_OF_ASSIGNMENT = 0b1000000000000000000000000000000,
    EXPRESSION_IS_MODULAS = 0b10000000000000000000000000000000,
};

#define EXPRESSION_GEN_MATHABLE (       \
    EXPRESSION_IS_ADDITION |           \
    EXPRESSION_IS_SUBTRACTION |        \
    EXPRESSION_IS_MULTIPLICATION |     \
    EXPRESSION_IS_DIVISION |           \
    EXPRESSION_IS_MODULAS |            \
    EXPRESSION_IS_FUNCTION_CALL |      \
    EXPRESSION_INDIRECTION |           \
    EXPRESSION_GET_ADDRESS |           \
    EXPRESSION_IS_ABOVE |              \
    EXPRESSION_IS_ABOVE_OR_EQUAL |     \
    EXPRESSION_IS_BELOW |              \
    EXPRESSION_IS_BELOW_OR_EQUAL |     \
    EXPRESSION_IS_EQUAL |              \
    EXPRESSION_IS_NOT_EQUAL |          \
    EXPRESSION_LOGICAL_AND |           \
    EXPRESSION_LOGICAL_OR |            \
    EXPRESSION_IN_LOGICAL_EXPRESSION | \
    EXPRESSION_IS_BITSHIFT_LEFT |      \
    EXPRESSION_IS_BITSHIFT_RIGHT |     \
    EXPRESSION_IS_BITWISE_OR |         \
    EXPRESSION_IS_BITWISE_AND |        \
    EXPRESSION_IS_BITWISE_XOR)


#define EXPRESSION_UNINHERITABLE_FLAGS (            \
      EXPRESSION_FLAG_RIGHT_NODE | EXPRESSION_IN_FUNCTION_CALL_ARGUMENTS | \
      EXPRESSION_IS_ADDITION | EXPRESSION_IS_MODULAS | EXPRESSION_IS_SUBTRACTION | EXPRESSION_IS_MULTIPLICATION | \
      EXPRESSION_IS_DIVISION | EXPRESSION_IS_ABOVE | EXPRESSION_IS_ABOVE_OR_EQUAL | \
      EXPRESSION_IS_BELOW | EXPRESSION_IS_BELOW_OR_EQUAL | EXPRESSION_IS_EQUAL |    \
      EXPRESSION_IS_NOT_EQUAL | EXPRESSION_LOGICAL_AND | \
      EXPRESSION_IS_BITSHIFT_LEFT | EXPRESSION_IS_BITSHIFT_RIGHT | \
      EXPRESSION_IS_BITWISE_OR | EXPRESSION_IS_BITWISE_AND | EXPRESSION_IS_BITWISE_XOR | EXPRESSION_IS_ASSIGNMENT | IS_ALONE_STATEMENT)

enum
{
    STRUCT_ACCESS_BACKWARDS = 0b00000001,
    STRUCT_STOP_AT_POINTER_ACCESS = 0b00000010
};

enum
{
    // The flag is set for native functions.
    FUNCTION_NODE_FLAG_IS_NATIVE = 0b00000001,
};

int compile_file(const char *filename, const char *out_filename, int flags);
struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags, struct compile_process* parent_process);
const char* compiler_include_dir_begin(struct compile_process* process);
const char* compiler_include_dir_next(struct compile_process* process);
struct compile_process* compile_include(const char* filename, struct compile_process* parent_process);

void compiler_setup_default_include_directories(struct vector* include_vec);

char compile_process_next_char(struct lex_process *lex_process);
char compile_process_peek_char(struct lex_process *lex_process);
void compile_process_push_char(struct lex_process *lex_process, char c);

void compiler_node_error(struct node* node, const char* msg, ...);
void compiler_error(struct compile_process *compiler, const char *msg, ...);
void compiler_warning(struct compile_process *compiler, const char *msg, ...);

struct lex_process *lex_process_create(struct compile_process *compiler, struct lex_process_functions *functions, void *private);
void lex_process_free(struct lex_process *process);
void *lex_process_private(struct lex_process *process);
struct vector *lex_process_tokens(struct lex_process *process);
int lex(struct lex_process *process);
int parse(struct compile_process *process);
int codegen(struct compile_process *process);
struct code_generator *codegenerator_new(struct compile_process *process);


// Validator

int validate(struct compile_process* process);

/**
 * @brief Builds tokens for the input string.
 *
 * @param compiler
 * @param str
 * @return struct lex_process*
 */
struct lex_process *tokens_build_for_string(struct compile_process *compiler, const char *str);

bool token_is_keyword(struct token *token, const char *value);
bool token_is_identifier(struct token *token);
bool token_is_symbol(struct token *token, char c);
struct vector* tokens_join_vector(struct compile_process* compiler, struct vector* token_vec);

bool token_is_nl_or_comment_or_newline_seperator(struct token *token);
bool keyword_is_datatype(const char *str);
bool token_is_primitive_keyword(struct token *token);

bool datatype_is_void_no_ptr(struct datatype* dtype);
void datatype_set_void(struct datatype* dtype);
bool datatype_is_struct_or_union_for_name(const char *name);
size_t datatype_size_for_array_access(struct datatype *dtype);
size_t datatype_element_size(struct datatype *dtype);
size_t datatype_size_no_ptr(struct datatype *dtype);
size_t datatype_size(struct datatype *dtype);
bool datatype_is_primitive(struct datatype *dtype);
bool datatype_is_struct_or_union_non_pointer(struct datatype *dtype);
struct datatype datatype_for_numeric();
struct datatype datatype_for_string();
struct datatype* datatype_thats_a_pointer(struct datatype* d1, struct datatype* d2);
struct datatype* datatype_pointer_reduce(struct datatype* datatype, int by);
bool is_logical_operator(const char* op);
bool is_logical_node(struct node* node);

bool token_is_operator(struct token *token, const char *val);
bool is_operator_token(struct token* token);

struct node *node_create(struct node *_node);
struct node *node_from_sym(struct symbol *sym);
struct node *node_from_symbol(struct compile_process *current_process, const char *name);
bool node_is_expression_or_parentheses(struct node *node);
bool node_is_value_type(struct node *node);
bool node_is_expression(struct node *node, const char *op);
bool node_is_struct_or_union(struct node *node);
bool is_array_node(struct node *node);
bool is_node_assignment(struct node *node);
bool is_unary_operator(const char *op);
bool op_is_indirection(const char *op);
bool op_is_address(const char *op);

struct node *struct_node_for_name(struct compile_process *current_process, const char *name);
struct node *union_node_for_name(struct compile_process *current_process, const char *name);

void make_tenary_node(struct node *true_node, struct node *false_node);

void make_default_node();
void make_case_node(struct node *exp_node);
void make_goto_node(struct node *label_node);
void make_label_node(struct node *name_node);

void make_continue_node();
void make_break_node();

void make_cast_node(struct datatype *dtype, struct node *operand_node);
void make_exp_node(struct node *left_node, struct node *right_node, const char *op);
void make_exp_parentheses_node(struct node *exp_node);

void make_bracket_node(struct node *node);
void make_body_node(struct vector *body_vec, size_t size, bool padded, struct node *largest_var_node);
void make_struct_node(const char *name, struct node *body_node);
void make_union_node(const char *name, struct node *body_node);
void make_switch_node(struct node *exp_node, struct node *body_node, struct vector *cases, bool has_default_case);
void make_function_node(struct datatype *ret_type, const char *name, struct vector *arguments, struct node *body_node);
void make_while_node(struct node *exp_node, struct node *body_node);
void make_do_while_node(struct node *body_node, struct node *exp_node);
void make_for_node(struct node *init_node, struct node *cond_node, struct node *loop_node, struct node *body_node);
void make_return_node(struct node *exp_node);
void make_if_node(struct node *cond_node, struct node *body_node, struct node *next_node);
void make_else_node(struct node *body_node);
void make_unary_node(const char* op, struct node* operand_node, int flags);
bool is_left_operanded_unary_operator(const char* op);
bool is_parentheses(const char* op);
bool unary_operand_compatible(struct token* token);
struct node *node_pop();
struct node *node_peek();
struct node *node_peek_or_null();
void node_push(struct node *node);
void node_set_vector(struct vector *vec, struct vector *root_vec);

bool is_access_operator(const char *op);
bool is_access_node(struct node *node);
bool is_array_operator(const char *op);
bool is_array_node(struct node *node);
bool is_parentheses_operator(const char *op);
bool is_parentheses_node(struct node *node);
bool is_access_node_with_op(struct node *node, const char *op);
bool is_argument_operator(const char *op);
bool is_argument_node(struct node *node);
void datatype_decrement_pointer(struct datatype *dtype);
size_t array_brackets_count(struct datatype *dtype);

bool node_is_expressionable(struct node *node);
bool node_valid(struct node *node);

struct node *node_peek_expressionable_or_null();
bool node_is_struct_or_union_variable(struct node *node);

struct array_brackets *array_brackets_new();
void array_brackets_free(struct array_brackets *brackets);
void array_brackets_add(struct array_brackets *brackets, struct node *bracket_node);
struct vector *array_brackets_node_vector(struct array_brackets *brackets);
size_t array_brackets_calculate_size_from_index(struct datatype *dtype, struct array_brackets *brackets, int index);
size_t array_brackets_calculate_size(struct datatype *dtype, struct array_brackets *brackets);
int array_total_indexes(struct datatype *dtype);
bool datatype_is_struct_or_union(struct datatype *dtype);
struct node *variable_struct_or_union_body_node(struct node *node);
struct node *variable_node_or_list(struct node *node);

int array_multiplier(struct datatype *dtype, int index, int index_value);
int array_offset(struct datatype *dtype, int index, int index_value);
int struct_offset(struct compile_process *compile_proc, const char *struct_name, const char *var_name, struct node **var_node_out, int last_pos, int flags);
struct node *variable_struct_or_union_largest_variable_node(struct node *var_node);
struct node *body_largest_variable_node(struct node *body_node);

struct resolver_entity *resolver_make_entity(struct resolver_process *process, struct resolver_result *result, struct datatype *custom_dtype, struct node *node, struct resolver_entity *guided_entity, struct resolver_scope *scope);
struct resolver_process *resolver_new_process(struct compile_process *compiler, struct resolver_callbacks *callbacks);
struct resolver_entity *resolver_new_entity_for_var_node(struct resolver_process *process, struct node *var_node, void *private, int offset);
struct resolver_entity *resolver_register_function(struct resolver_process *process, struct node *func_node, void *private);
struct resolver_scope *resolver_new_scope(struct resolver_process *resolver, void *private, int flags);
struct resolver_entity* resolver_get_variable_from_local_scope(struct resolver_process* resolver, const char* var_name);

void resolver_finish_scope(struct resolver_process *resolver);
struct resolver_result *resolver_follow(struct resolver_process *resolver, struct node *node);
bool resolver_result_ok(struct resolver_result *result);
struct resolver_entity *resolver_result_entity_root(struct resolver_result *result);
struct resolver_entity *resolver_result_entity_next(struct resolver_entity *entity);
struct resolver_entity *resolver_result_entity(struct resolver_result *result);

bool function_node_is_prototype(struct node *node);
size_t function_node_stack_size(struct node *node);
struct vector *function_node_argument_vec(struct node *node);

struct resolver_default_entity_data *resolver_default_entity_private(struct resolver_entity *entity);
struct resolver_default_scope_data *resolver_default_scope_private(struct resolver_scope *scope);
char *resolver_default_stack_asm_address(int stack_offset, char *out);
struct resolver_default_entity_data *resolver_default_new_entity_data();
void resolver_default_global_asm_address(const char *name, int offset, char *address_out);

void resolver_default_entity_data_set_address(struct resolver_default_entity_data *entity_data, struct node *var_node, int offset, int flags);
void *resolver_default_make_private(struct resolver_entity *entity, struct node *node, int offset, struct resolver_scope *scope);
void resolver_default_set_result_base(struct resolver_result *result, struct resolver_entity *base_entity);
struct resolver_default_entity_data *resolver_default_new_entity_data_for_var_node(struct node *var_node, int offset, int flags);
struct resolver_default_entity_data *resolver_default_new_entity_data_for_array_bracket(struct node *breacket_node);
struct resolver_default_entity_data *resolver_default_new_entity_data_for_function(struct node *func_node, int flags);
struct resolver_entity *resolver_default_new_scope_entity(struct resolver_process *resolver, struct node *var_node, int offset, int flags);

struct resolver_entity *resolver_default_register_function(struct resolver_process *resolver, struct node *func_node, int flags);

void resolver_default_new_scope(struct resolver_process *resolver, int flags);
void resolver_default_finish_scope(struct resolver_process *resolver);
void *resolver_default_new_array_entity(struct resolver_result *result, struct node *array_entity_node);
void resolver_default_delete_entity(struct resolver_entity *entity);
void resolver_default_delete_scope(struct resolver_scope *scope);
struct resolver_entity *resolver_default_merge_entities(struct resolver_process *process, struct resolver_result *result, struct resolver_entity *left_entity, struct resolver_entity *right_entity);
struct resolver_process *resolver_default_new_process(struct compile_process *compiler);

/**
 * @brief Gets the variable size from the given variable node
 *
 * @param var_node
 * @return size_t
 */
size_t variable_size(struct node *var_node);
/**
 * @brief Sums the variable size of all variable nodes inside the variable list node
 * Returns the result
 *
 * @param var_list_node
 * @return size_t The sum of all variable node sizes in the list.
 */
size_t variable_size_for_list(struct node *var_list_node);
struct node *variable_node(struct node *node);
bool variable_node_is_primitive(struct node *node);

int padding(int val, int to);
int align_value(int val, int to);
int align_value_treat_positive(int val, int to);
int compute_sum_padding(struct vector *vec);


// Preprocessor
void preprocessor_create_definitions(struct preprocessor* preprocessor);
struct token *preprocessor_previous_token(struct compile_process *compiler);
struct vector *preprocessor_build_value_vector_for_integer(int value);
struct preprocessor_definition *preprocessor_definition_create_native(const char *name,
                                                                      PREPROCESSOR_DEFINITION_NATIVE_CALL_EVALUATE evaluate,
                                                                      PREPROCESSOR_DEFINITION_NATIVE_CALL_VALUE value,
                                                                      struct preprocessor *preprocessor);


struct scope *scope_new(struct compile_process *process, int flags);
struct scope *scope_create_root(struct compile_process *process);
void scope_free_root(struct compile_process *process);
void scope_iteration_start(struct scope *scope);
void scope_iteration_end(struct scope *scope);
void *scope_iterate_back(struct scope *scope);
void *scope_last_entity_at_scope(struct scope *scope);
void *scope_last_entity_from_scope_stop_at(struct scope *scope, struct scope *stop_scope);
void *scope_last_entity_stop_at(struct compile_process *process, struct scope *stop_scope);
void *scope_last_entity(struct compile_process *process);
void scope_push(struct compile_process *process, void *ptr, size_t elem_size);
void scope_finish(struct compile_process *process);
struct scope *scope_current(struct compile_process *process);

void symresolver_initialize(struct compile_process *process);
void symresolver_new_table(struct compile_process *process);
void symresolver_end_table(struct compile_process *process);
void symresolver_build_for_node(struct compile_process *process, struct node *node);
struct symbol* symresolver_register_symbol(struct compile_process* process, const char* sym_name, int type, void* data);

struct symbol *symresolver_get_symbol(struct compile_process *process, const char *name);
struct symbol *symresolver_get_symbol_for_native_function(struct compile_process *process, const char *name);


struct expressionable;
struct expressionable_callbacks;
struct expressionable_config;

struct expressionable_callbacks *expressionable_callbacks(struct expressionable *expressionable);
void *expressionable_node_pop(struct expressionable *expressionable);
struct token *expressionable_token_next(struct expressionable *expressionable);

void expressionable_node_push(struct expressionable *expressionable, void *node_ptr);
void *expressionable_node_peek_or_null(struct expressionable *expressionable);
void expressionable_ignore_nl(struct expressionable *expressionable, struct token *next_token);
struct token *expressionable_peek_next(struct expressionable *expressionable);
bool expressionable_token_next_is_operator(struct expressionable *expressionable, const char *op);
void expressionable_init(struct expressionable* expressionable, struct vector* token_vector, struct vector* node_vector, struct expressionable_config* config, int flags);
struct expressionable* expressionable_create(struct expressionable_config* config, struct vector* token_vector, struct vector* node_vector, int flags);
int expressionable_parse_number(struct expressionable *expressionable);
int expressionable_parse_identifier(struct expressionable *expressionable);

bool expressionable_parser_left_op_has_priority(const char *op_left, const char *op_right);
void expressionable_parser_node_shift_children_left(struct expressionable *expressionable, void *node);

void expressionable_parser_reorder_expression(struct expressionable *expressionable, void **node_out);

bool expressionable_generic_type_is_value_expressionable(int type);
void expressionable_expect_op(struct expressionable *expressionable, const char *op);

void expressionable_expect_sym(struct expressionable *expressionable, char c);
void expressionable_deal_with_additional_expression(struct expressionable *expressionable);
void expressionable_parse_parentheses(struct expressionable *expressionable);
int expressionable_get_pointer_depth(struct expressionable *expressionable);
void expressionable_parse_for_indirection_unary(struct expressionable *expressionable);

void expressionable_parse_for_normal_unary(struct expressionable *expressionable);
void expressionable_parse_unary(struct expressionable *expressionable);
void expressionable_parse_for_operator(struct expressionable *expressionable);
void expressionable_parse_tenary(struct expressionable* expressionable);
int expressionable_parse_exp(struct expressionable *expressionable, struct token *token);
int expressionable_parse_token(struct expressionable *expressionable, struct token *token, int flags);
int expressionable_parse_single_with_flags(struct expressionable *expressionable, int flags);
int expressionable_parse_single(struct expressionable *expressionable);
void expressionable_parse(struct expressionable *expressionable);

size_t function_node_argument_stack_addition(struct node *node);
long arithmetic(struct compile_process* compiler, long left_operand, long right_operand, const char* op, bool* success);

#define TOTAL_OPERATOR_GROUPS 14
#define MAX_OPERATORS_IN_GROUP 12

enum
{
    ASSOCIATIVITY_LEFT_TO_RIGHT,
    ASSOCIATIVITY_RIGHT_TO_LEFT
};

struct expressionable_op_precedence_group
{
    char *operators[MAX_OPERATORS_IN_GROUP];
    int associtivity;
};


enum
{
    EXPRESSIONABLE_GENERIC_TYPE_NUMBER,
    EXPRESSIONABLE_GENERIC_TYPE_IDENTIFIER,
    EXPRESSIONABLE_GENERIC_TYPE_UNARY,
    EXPRESSIONABLE_GENERIC_TYPE_PARENTHESES,
    EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION,
    EXPRESSIONABLE_GENERIC_TYPE_NON_GENERIC
};

enum
{
    EXPRESSIONABLE_IS_SINGLE,
    EXPRESSIONABLE_IS_PARENTHESES
};

struct expressionable;

typedef void*(*EXPRESSIONABLE_HANDLE_NUMBER)(struct expressionable* expressionable);
typedef void*(*EXPRESSIONABLE_HANDLE_IDENTIFIER)(struct expressionable* expressionable);
typedef void (*EXPRESSIONABLE_MAKE_EXPRESSION_NODE)(struct expressionable* expressionable, void* left_node_ptr, void* right_node_ptr, const char* op);
typedef void (*EXPRESSIONABLE_MAKE_TENARY_NODE)(struct expressionable* expressionable, void* true_result_node, void* false_result_node);
typedef void (*EXPRESSIONABLE_MAKE_PARENTHESES_NODE)(struct expressionable* expressionable, void* node_ptr);
typedef void (*EXPRESSIONABLE_MAKE_UNARY_NODE)(struct expressionable* expressionable, const char* op, void* right_operand_node_ptr);
typedef void (*EXPRESSIONABLE_MAKE_UNARY_INDIRECTION_NODE)(struct expressionable* expressionable, int ptr_depth, void* right_operand_node_ptr);
typedef int (*EXPRESSIONABLE_GET_NODE_TYPE)(struct expressionable* expressionable, void* node);
typedef void* (*EXPRESSIONABLE_GET_LEFT_NODE)(struct expressionable* expressionable, void* target_node);
typedef void* (*EXPRESSIONABLE_GET_RIGHT_NODE)(struct expressionable* expressionable, void* target_node);
typedef const char* (*EXPRESSIONABLE_GET_NODE_OPERATOR)(struct expressionable* expressionable, void* target_node);
typedef void** (*EXPRESSIONABLE_GET_NODE_ADDRESS)(struct expressionable* expressionable, void* target_node);
typedef void (*EXPRESSIONABLE_SET_EXPRESSION_NODE)(struct expressionable* expressionable, void* node, void* left_node, void* right_node, const char* op );

typedef bool (*EXPRESSIONABLE_SHOULD_JOIN_NODES)(struct expressionable* expressionable, void* previous_node, void* node);
typedef void* (*EXPRESSIONABLE_JOIN_NODES)(struct expressionable* expressionable, void* previous_node, void* node);
typedef bool (*EXPRESSIONABLE_EXPECTING_ADDITIONAL_NODE)(struct expressionable* expressionable, void* node);
typedef bool (*EXPRESSIONABLE_IS_CUSTOM_OPERATOR)(struct expressionable* expressionable, struct token* token);


struct expressionable_config
{
    struct expressionable_callbacks
    {
        // Function pointers
        EXPRESSIONABLE_HANDLE_NUMBER handle_number_callback;
        EXPRESSIONABLE_HANDLE_IDENTIFIER handle_identifier_callback;
        EXPRESSIONABLE_MAKE_EXPRESSION_NODE make_expression_node;
        EXPRESSIONABLE_MAKE_PARENTHESES_NODE make_parentheses_node;
        EXPRESSIONABLE_MAKE_UNARY_NODE make_unary_node;
        EXPRESSIONABLE_MAKE_UNARY_INDIRECTION_NODE make_unary_indirection_node;
        EXPRESSIONABLE_MAKE_TENARY_NODE make_tenary_node;
        EXPRESSIONABLE_GET_NODE_TYPE get_node_type;
        EXPRESSIONABLE_GET_LEFT_NODE get_left_node;
        EXPRESSIONABLE_GET_RIGHT_NODE get_right_node;
        EXPRESSIONABLE_GET_NODE_OPERATOR get_node_operator;
        EXPRESSIONABLE_GET_NODE_ADDRESS get_left_node_address;
        EXPRESSIONABLE_GET_NODE_ADDRESS get_right_node_address;
        EXPRESSIONABLE_SET_EXPRESSION_NODE set_exp_node;
        EXPRESSIONABLE_SHOULD_JOIN_NODES should_join_nodes;
        EXPRESSIONABLE_JOIN_NODES join_nodes;
        EXPRESSIONABLE_EXPECTING_ADDITIONAL_NODE expecting_additional_node;
        EXPRESSIONABLE_IS_CUSTOM_OPERATOR is_custom_operator;
    } callbacks;
};
enum
{
    EXPRESSIONABLE_FLAG_IS_PREPROCESSOR_EXPRESSION = 0b00000001,
};

struct expressionable
{
    int flags;
    struct expressionable_config config;
    struct vector* token_vec;
    struct vector* node_vec_out;
};

struct fixup;

/**
 * Fixes the fixup.
 * Return true if the fixup was successful.
 *
 */
typedef bool (*FIXUP_FIX)(struct fixup *fixup);
/**
 * @brief Signifies the fixup has been removed from memory.
 * The implementor of this function pointer should free any memory related
 * to the fixup.
 */
typedef void (*FIXUP_END)(struct fixup *fixup);

struct fixup_config
{
    FIXUP_FIX fix;
    FIXUP_END end;
    void *private;
};

struct fixup_system
{
    // A vector of fixups.
    struct vector *fixups;
};

enum
{
    FIXUP_FLAG_RESOLVED = 0b00000001
};

struct fixup
{
    int flags;
    struct fixup_system *system;
    struct fixup_config config;
};

struct fixup_system *fixup_sys_new();
struct fixup_config *fixup_config(struct fixup *fixup);

void fixup_free(struct fixup *fixup);
void fixup_start_iteration(struct fixup_system *system);
struct fixup *fixup_next(struct fixup_system *system);
void fixup_sys_fixups_free(struct fixup_system *system);

void fixup_sys_free(struct fixup_system *system);
int fixup_sys_unresolved_fixups_count(struct fixup_system *system);
struct fixup *fixup_register(struct fixup_system *system, struct fixup_config *config);
bool fixup_resolve(struct fixup *fixup);
void *fixup_private(struct fixup *fixup);
bool fixups_resolve(struct fixup_system *system);



// Helpers
bool file_exists(const char* filename);

#endif