#include <stdio.h>
#include "operand.h"
//global_array_init_item
#include <value.h>

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
    print_single_data(node);
    
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
 * @brief 只在数据段打印标签
 * @birth: Created by LGD on 2023-7-20
*/
void data_label(char* name)
{
    struct _dataNode* node = (struct _dataNode*)malloc(sizeof(struct _dataNode));
    memset(node, 0, sizeof(struct _dataNode));
    node->label = name;
    node->dExp = NONE_CNT;
    data_link_node(node);
}

/**
 * @brief: .long表达式 直接指定常数
 * @birth:Created by LGD on 2023-7-20
*/
void dot_long_expression_literal(char* name,int num,bool replacale){
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
                //如果当前.long语句没有标号，跳过
                if(!p->label)continue;
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
    struct _dataNode* node = (struct _dataNode*)malloc(sizeof(struct _dataNode));
    memset(node, 0, sizeof(struct _dataNode));
    node->label = name;
    node->dExp = DOT_LONG;
    node->content = num;
    data_link_node(node);    
}

/**
 * @brief: .long表达式
 * @birth:Created by LGD on 2023-5-29
 * @update: long指令被设计为可替换的
 *          2023-7-20 简化逻辑
*/
void dot_long_expression(char* name,struct _operand expr,bool replacale)
{
    //创建新的节点
    assert(expr.addrMode == IMMEDIATE && ".long expression could only be integer literal");
    dot_long_expression_literal(name,expr.oprendVal,replacale);
}

/**
 * @brief: .zero表达式
 * @birth:Created by LGD on 2023-5-29
 * @update: 2023-7-20 必须指定空间大小
*/
void dot_zero_expression(char* name,size_t space)
{
    struct _dataNode* node = (struct _dataNode*)malloc(sizeof(struct _dataNode));
    memset(node, 0, sizeof(struct _dataNode));
    node->label = name;
    node->dExp = DOT_ZERO;
    node->content = space;
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

/**
 * @brief 翻译数组字面量
 * @return 为1代表当前数组有初始化，为0则否
 * @birth: Created by LGD on 2023-7-20
 * @update: 2023-7-21 返回布尔值判断是否有字面量
 *          2023-7-21 数组的最后空白部分也要补齐
*/
bool array_init_literal(char* name,size_t total_space,List* literalList)
{
    //判断当前数组是否有字面量
    if(literalList == NULL)return false;
    //判断当前数组是否需要字面量初始化
    if(ListSize(literalList) == 0)
        return false;

    ListFirst(literalList, false);
    global_array_init_item* item;
    while(ListNext(literalList, (void*)&item) != false){
        printf("%d %f\n",item->offset,item->value);
    }

    ListFirst(literalList, false);
    data_label(name);
    //当前下标计数器
    size_t idx = 0;
    while(ListNext(literalList, (void*)&item) != false){
        if(item->offset != idx)
        {
            dot_zero_expression(NULL,4*(item->offset-idx));
            idx = item->offset;
        }
        dot_long_expression_literal(NULL, (int)item->value, false);
        ++idx;
    }
    //补齐数组末端
    dot_zero_expression(NULL, total_space - idx * 4);
    return true;
}



