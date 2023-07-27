/**
  ******************************************************************************
  * @file           : var_info.c
  * @brief          : 后端操作变量
  ******************************************************************************
*/

#include <stdbool.h>

#include "variable_map.h"
#include "operand.h"
#include "arm_assembly.h"

/**
 * @brief 初始化一个变量信息块
 * @birth: Created by LGD on 2023-7-17
*/
VarInfo* varInfoInit()
{
    VarInfo* varInfo = (VarInfo*)malloc(sizeof(VarInfo));
    memset(varInfo, 0, sizeof(VarInfo));
    return varInfo;
}

/**
 * @brief 使变量位置转换生效
 *        当原始位置为空时，或原始和当前位置一致时，不产生指令
 *        当当前位置为空时，不产生指令，并将原始位置补充到当前位置
 * @birth: Created by LGD on 2023-7-17
 * @param doClear 舍弃变量原来的位置，将新位置定义为原始位置
*/
void update_variable_location(VarInfo* varInfo,bool doClear)
{
    //assert(!operand_is_none(varInfo->current) && "current location is unallocated!");
    if(operand_is_none(varInfo->current))
    {
        //TODO 内存溢出！！！ 2023-7-17
        //memcpy(&(varInfo->current),&(varInfo->ori),sizeof(VarInfo));
        varInfo->current = varInfo->ori;
        return;
    }
    varInfo->current.format = INTERGER_TWOSCOMPLEMENT;
    varInfo->ori.format = INTERGER_TWOSCOMPLEMENT;
    varInfo->ori.point2spaceFormat = INTERGER_TWOSCOMPLEMENT;
    varInfo->current.point2spaceFormat = INTERGER_TWOSCOMPLEMENT;
    movii(varInfo->current,varInfo->ori);
    if(doClear)
        varInfo->ori = varInfo->current;
}

/**
 * @brief 设置变量的原始位置
 * @birth: Created by LGD on 2023-7-17
**/
void set_var_oriLoc(VarInfo* varInfo,struct _operand loc)
{
    memcpy(&varInfo->ori, &loc, sizeof(struct _operand));
}

/**
 * @brief 设置变量的当前位置
 * @birth: Created by LGD on 2023-7-17
**/
void set_var_curLoc(VarInfo* varInfo,struct _operand loc)
{
    memcpy(&varInfo->current, &loc, sizeof(struct _operand));
}

/**
 * @brief 设置当前所处寄存器
 * @birth: Created by LGD on 2023-7-17
*/
void set_var_curReg(VarInfo* varInfo,RegisterOrder reg)
{
    varInfo->current.addrMode = REGISTER_DIRECT;
    varInfo->current.oprendVal = reg;
    varInfo->current.addtion = 0;
}

/**
 * @brief 设置变量当前偏移
 * @birth: Created by LGD on 2023-7-17
**/
void set_var_curStkOff(VarInfo* varInfo,int offset)
{
    varInfo->current.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
    varInfo->current.oprendVal = FP;
    varInfo->current.addtion = offset;
}

/**
 * @brief 将变量传递到一个新位置
*/
void chg_var_loc2();