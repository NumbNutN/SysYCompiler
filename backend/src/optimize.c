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
 * @update: Created by LGD on 2023-4-19
*/
void ins_mul_2_lsl(Instruction* ins)
{
    AssembleOperand op1 = toOperand(ins,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(ins,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(ins,TARGET_OPERAND);

    if(op1.addrMode == IMMEDIATE && number_is_power_of_2(op1.oprendVal))
    {
        AssembleOperand cvtOp2;
        if(judge_operand_in_RegOrMem(op2) == IN_MEMORY)
            cvtOp2 = operand_load_from_memory(op2,ARM);
        else if(judge_operand_in_RegOrMem(op2) == IN_INSTRUCTION)
            cvtOp2 = operand_load_immediate(op2,ARM);
        else
            cvtOp2 = op2;

        operand_set_shift(&cvtOp2,LSL,log(op1.oprendVal)/log(2));
        
        //第二操作数需要放置在合适的位置
        general_data_processing_instructions(MOV,tarOp,nullop,cvtOp2,NONESUFFIX,false);
    }
    else
    {
        AssembleOperand cvtOp1;

        if(judge_operand_in_RegOrMem(op1) == IN_MEMORY)
            cvtOp1 = operand_load_from_memory(op1,ARM);
        else if(judge_operand_in_RegOrMem(op1) == IN_INSTRUCTION)
            cvtOp1 = operand_load_immediate(op1,ARM);
        else
            cvtOp1 = op1;
            
        operand_set_shift(&cvtOp1,LSL,log(op2.oprendVal)/log(2));

        general_data_processing_instructions(MOV,tarOp,nullop,cvtOp1,NONESUFFIX,false);
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