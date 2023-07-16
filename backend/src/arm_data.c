#include <stdio.h>
#include "operand.h"

List* dataList = NULL;

struct _dataNode* bssList = NULL;
struct _dataNode* bssPrev = NULL;

struct _AsDirective* asList = NULL;
struct _AsDirective* asPrev = NULL;

/**
 * @brief 初始化数据链表
 * @birth: Created by LGD on 2023-7-16
**/
void data_list_init()
{
    dataList = ListInit();
}

/**
 * @brief:链接下一个数据节点
 * @update: 2023-7-16 use List Lib
*/
void data_link_node(struct _dataNode* node)
{

    if(!dataList)
        assert("data list has not been inited");
    ListPushBack(dataList, node);
    
}

void data_remove_node(struct _dataNode* node)
{
    if(!dataList)
        assert("data list has not been inited");
    ListFirst(dataList, false);
    size_t cnt = 0;
    struct _dataNode* p;
    do {
        if(!ListNext(dataList, &p))break;
        cnt++;
    }
    while(node != p);
        
    if(node != p)
        assert("Could not find the node!");
    ListRemove(dataList, cnt-1);
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
 * @brief: .long表达式
 * @birth:Created by LGD on 2023-5-29
 * @update: long指令被设计为可替换的
*/
void dot_long_expression(char* name,struct _operand expr,bool replacale)
{
    //检索已经存在的long或zero
    if(replacale)
    {
        struct _dataNode* p;
        ListFirst(dataList, false);
        do{
            if(!ListNext(dataList, &p))break;
            //判断重复项
            if((p->dExp == DOT_LONG) || (p->dExp == DOT_ZERO))
            {
                //找到重复项
                if(!strcmp(p->label,name))
                {
                    data_remove_node(p);
                    //找到重复项后务必退出，因为链表的遍历指针会改变
                    break;
                }
            }
        }while(1);
    }

    //创建新的节点
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




