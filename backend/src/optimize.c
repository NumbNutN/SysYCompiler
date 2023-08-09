#include "operand.h"
#include "arm_assembly.h"
#include "arm.h"
#include "variable_map.h"
#include "instruction.h"
#include "operand.h"

#include "dependency.h"

#include <assert.h>
#include <string.h>
#include <math.h>

/**
 * @brief 乘法转移码条件触发器
 * @update: Created by LGD on 2023-4-20
*/
bool ins_mul_2_lsl_trigger(Instruction* ins)
{
    AssembleOperand op1 = toOperand(ins,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(ins,SECOND_OPERAND);

    return ins_get_opCode(ins) == MulOP &&
    (
        (op1.addrMode == IMMEDIATE && number_is_power_of_2(op1.oprendVal))||
        (op2.addrMode == IMMEDIATE && number_is_power_of_2(op2.oprendVal))
    );
}

/**
 * @brief 乘法移位优化
 * @birth: Created by LGD on 2023-4-19
 * @update: 2023-5-29 添加寄存器回收
 *          2023-7-29 更简洁的表达
*/
void ins_mul_2_lsl(Instruction* ins)
{
    AssembleOperand op1 = toOperand(ins,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(ins,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(ins,TARGET_OPERAND);

    if(op1.addrMode == IMMEDIATE && number_is_power_of_2(op1.oprendVal))
    {
        AssembleOperand cvtOp2;
        
        cvtOp2 = operandConvert(op2, ARM, 0, 0);

        operand_set_shift(&cvtOp2,LSL,log(op1.oprendVal)/log(2));
        
        //第二操作数需要放置在合适的位置
        general_data_processing_instructions(MOV,tarOp,nullop,cvtOp2,NONESUFFIX,false);

        //回收寄存器
        if(!operand_is_same(cvtOp2, op2))
            operand_recycle_temp_register(cvtOp2);
    }
    else
    {
        AssembleOperand cvtOp1;

        cvtOp1 = operandConvert(op1, ARM, 0, 0);
            
        operand_set_shift(&cvtOp1,LSL,log(op2.oprendVal)/log(2));

        general_data_processing_instructions(MOV,tarOp,nullop,cvtOp1,NONESUFFIX,false);

        if(!operand_is_same(cvtOp1, op1))
            operand_recycle_temp_register(cvtOp1);
    }
}


void mul2lsl(assmNode* ins)
{   
    //判断是否是乘法
    assert(!strcmp(ins->opCode,"MUL"));
    //判断是否乘以2的倍数
    //struct _operand secondOperand = AssemblyNode_get_opernad(ins,SECOND_OPERAND);
    //assert(operand_get_immediate(secondOperand) % 2 == 0);

    strcpy(ins->opCode,"MOV");
    ins->op[2].oprendVal = 2;
}

/**
 * @brief 当判断出强制跳转语句接一个标号时，可以删去该跳转
 * @birth: Created by LGD on 2023-8-9
*/
void remove_unnessary_branch(){
    assmNode* p;
    for(p = head;p != NULL;p = p->next){
        //当前语句是branch
        if(strcmp("B",p->opCode))continue;
        assmNode* pNext = p->next;
        //下一个语句是label
        if(pNext->assemType != LABEL)continue;
        //branch语句指向标号和下一个标号一致
        if(strcmp((char*)p->op[0].oprendVal,pNext->label))continue;
        //移除节点
        codeRemoveNode(p);
    }
}