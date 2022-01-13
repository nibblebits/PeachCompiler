#include "compiler.h"

/**
 * Format: {operator1, operator2, operator3, NULL}
 */

struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS] = {
    {.operators={"++", "--", "()", "[]", "(", "[", ".", "->",  NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"*", "/", "%", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"+", "-", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"<<", ">>", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"<", "<=", ">", ">=", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"==", "!=", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"&", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"^", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"|", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"&&", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"||", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"?", ":", NULL}, .associtivity=ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators={"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", NULL}, .associtivity=ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators={",", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT}
};