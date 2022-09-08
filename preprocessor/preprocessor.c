#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"

enum
{
    TYPEDEF_TYPE_STANDARD,
    // A structure typedef is basically "typedef struct ABC { int x; } AAA;"
    TYPEDEF_TYPE_STRUCTURE_TYPEDEF
};

struct typedef_type
{
    int type;
    const char* definiton_name;
    struct vector* value;
    struct typedef_structure
    {
        const char* sname;
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
            struct preprocessor_node* left;
            struct preprocessor_node* right;
            const char* op;
        } exp;

        struct preprocessor_unary_node
        {
            struct preprocessor_node* operand_node;
            const char* op;
            struct preprocessor_unary_indirection
            {
                int depth;
            } indirection;
        } unary_node;

        struct preprocessor_parenthesis
        {
            struct preprocessor_node* exp;
        } parenthesis;


        struct preprocessor_joined_node
        {
            struct preprocessor_node* left;
            struct preprocessor_node* right;
        } joined;

        struct preprocessor_tenary_node
        {
            struct preprocessor_node* true_node;
            struct preprocessor_node* false_node;
        } tenary;
    };

    const char* sval;
    
};