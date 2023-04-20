#include "arm.h"
#include "operand.h"

/**
 * @brief 返回汇编指令指定的操作数
 * @birth: Created by LGD on 2023-4-18
*/
struct _operand AssemblyNode_get_opernad(struct _assemNode* assemNode,size_t idx)
{
    return assemNode->op[idx];
}
