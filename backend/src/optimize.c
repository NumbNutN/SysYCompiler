#include "operand.h"
#include "arm_assembly.h"
#include "arm.h"
#include "variable_map.h"

#include <assert.h>
#include <string.h>
#include <math.h>


void ins_mul_2_lsl(Instruction* ins)
{
    AssembleOperand op1 = toOperand(ins,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(ins,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(ins,TARGET_OPERAND);

    if(op1.oprendVal % 2 == 0)
    {
        AssembleOperand cvtOp2;
        if(judge_operand_in_RegOrMem(op2) == IN_MEMORY)
            cvtOp2 = operand_load_from_memory(op2,ARM);
        else if(judge_operand_in_RegOrMem(op2) == IN_INSTRUCTION)
            cvtOp2 = operand_load_immediate(op2,ARM);
        else
            cvtOp2 = op2;

        operand_set_shift(&op1,LSL,log(op1.oprendVal)/log(2));
        
        general_data_processing_instructions("MOV",tarOp,op1,cvtOp2,NONESUFFIX,false,NONELABEL);
    }
    else
    {
        //已知有2的倍数
        AssembleOperand cvtOp1;

        if(judge_operand_in_RegOrMem(op1) == IN_MEMORY)
            cvtOp1 = operand_load_from_memory(op1,ARM);
        else if(judge_operand_in_RegOrMem(op1) == IN_INSTRUCTION)
            cvtOp1 = operand_load_immediate(op1,ARM);
        else
            cvtOp1 = op1;
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