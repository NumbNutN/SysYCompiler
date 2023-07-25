#include <stdio.h>

#include "arm.h"

//enum_2_str
#include "enum_2_str.h"

enum Section currentSection = NONESECTION;

enum _Procedure_Call_Strategy procudre_call_strategy;

void change_currentSection(enum Section c)
{
    if(c != currentSection)
        printf("%s %s\n",".section",enum_section_2_str(c));
    currentSection = c;
}

/**
 * @brief 为arm指令节点进行初始化
 * @birth: Created by LGD on 2023-7-15
 */
assmNode* arm_instruction_node_init()
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    memset(node, 0, sizeof(assmNode));
    return node;
}