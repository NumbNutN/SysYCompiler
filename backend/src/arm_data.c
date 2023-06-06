#include <stdio.h>
#include "operand.h"

struct _dataNode* dataList = NULL;
struct _dataNode* bssList = NULL;
struct _dataNode* dataPrev = NULL;
struct _dataNode* bssPrev = NULL;

struct _AsDirective* asList = NULL;
struct _AsDirective* asPrev = NULL;

void data_link_node(struct _dataNode* node)
{
    if(!dataList)
    {
        dataList = node;
        dataPrev = dataList;
    }
    else
    {
        dataPrev->next = node;
        dataPrev = node;
    }
}

void bss_link_node(struct _dataNode* node)
{
    if(!bssList)
    {
        bssList = node;
        bssPrev = bssList;
    }
    else
    {
        bssPrev->next = node;
        bssPrev = node;
    }
}

void asDirective_link_node(struct _AsDirective* node)
{
    if(!asList)
    {
        asList = node;
        asPrev = asList;
    }
    else
    {
        asPrev->next = node;
        asPrev = node;
    }
}

/**
@brief: .long表达式
@birth:Created by LGD on 2023-5-29
*/
void dot_long_expression(char* name,struct _operand expr)
{
        
    assert(expr.addrMode == IMMEDIATE && ".long expression could only be integer literal");
    struct _dataNode* node = (struct _dataNode*)malloc(sizeof(struct _dataNode));
    memset(node, 0, sizeof(struct _dataNode));
    node->label = name;
    node->dExp = DOT_LONG;
    node->content = expr.oprendVal;
    data_link_node(node);
}

/**
@brief: .zero表达式
@birth:Created by LGD on 2023-5-29
*/
void dot_zero_expression(char* name)
{
    struct _dataNode* node = (struct _dataNode*)malloc(sizeof(struct _dataNode));
    memset(node, 0, sizeof(struct _dataNode));
    node->label = name;
    node->dExp = DOT_ZERO;
    node->content = 4;
    data_link_node(node);
}

/**
 * @brief As伪指令-set function
 * @birth: Created by LGD on 2023-6-6
*/
void as_set_function_type(char* name)
{
    struct _AsDirective* node= (struct _AsDirective*)malloc(sizeof(struct _AsDirective));
    memset(node, 0, sizeof(struct _AsDirective));
    node->label = name;
    node->type = TYPE;
    asDirective_link_node(node);
}




