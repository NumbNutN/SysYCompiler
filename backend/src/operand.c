#include "operand.h"
#include "arm.h"
#include "arm_assembly.h"
#include "variable_map.h"
#include "interface_zzq.h"

/**
 * @brief 将Instruction中的变量转换为operand格式的方法
 * @birth: Created by LGD on 20230130
 * @todo 反思一下为什么一个函数要调用这么多复杂的形参呢？
 * @update: 2023-3-16 封装一层基础调用
*/
AssembleOperand toOperand(Instruction* this,int i)
{
    Value* var = ins_get_operand(this,i);
    return ValuetoOperand(this,var);
}

/**
 * @brief 依据Value* 返回 operand
 * @birth: Created by LGD on 2023-3-16
 * @update: 2023-4-4 将栈顶指针改为栈帧
*/
AssembleOperand ValuetoOperand(Instruction* this,Value* var)
{
    AssembleOperand op;
    RegorMem rOm = get_variable_place(this,var);
    switch(rOm)
    {
        case IN_MEMORY:
            op.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
            op.oprendVal = FP;
            op.addtion = get_variable_register_order_or_memory_offset_test(this,var);
        break;
        case IN_REGISTER:
            op.addrMode = REGISTER_DIRECT;
            op.oprendVal = get_variable_register_order_or_memory_offset_test(this,var);
        break;
        case IN_INSTRUCTION:
            op.addrMode = IMMEDIATE;
            op.oprendVal = value_getConstant(var);
        break;
    }
    op.format = valueFindFormat(var);
    return op;
}


/**
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_load_in_mem_throw(AssembleOperand op)
{
    AssembleOperand tempReg;
    switch(judge_operand_in_RegOrMem(op))
    {
        case IN_REGISTER:
            return op;
        case IN_MEMORY:
            switch(op.format)
            {
                case INTERGER_TWOSCOMPLEMENT:
                    tempReg.addrMode = REGISTER_DIRECT;
                    tempReg.oprendVal = pick_one_free_temp_arm_register();
                    tempReg.format = INTERGER_TWOSCOMPLEMENT;
                    memory_access_instructions("LDR",tempReg,op,NONESUFFIX,false,NONELABEL);
                break;
                case IEEE754_32BITS:
                    tempReg.addrMode = REGISTER_DIRECT;
                    tempReg.oprendVal = pick_one_free_vfp_register();
                    tempReg.format = IEEE754_32BITS;
                    vfp_memory_access_instructions("FLD",tempReg,op,FloatTyID);
                break;
            }
        return tempReg;
    }
    assert(judge_operand_in_RegOrMem(op) != IN_INSTRUCTION);
}

/**
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器,这次，你可以自定义用什么寄存器加载了
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_load_in_mem(AssembleOperand op,ARMorVFP type)
{
    AssembleOperand tempReg;
    switch(judge_operand_in_RegOrMem(op))
    {
        case IN_REGISTER:
            return op;
        case IN_MEMORY:
            tempReg.addrMode = REGISTER_DIRECT;
            tempReg.format = op.format;
            switch(type)
            {
                case ARM:
                    tempReg.oprendVal = pick_one_free_temp_arm_register();
                    memory_access_instructions("LDR",tempReg,op,NONESUFFIX,false,NONELABEL);
                break;
                case VFP:
                    tempReg.oprendVal = pick_one_free_vfp_register();
                    vfp_memory_access_instructions("FLD",tempReg,op,FloatTyID);
                break;
            }
            return tempReg;
    }
    assert(judge_operand_in_RegOrMem(op) != IN_INSTRUCTION);
}

/**
 * @brief 把暂存器存器再封装一层
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_pick_temp_register(ARMorVFP type)
{
    AssembleOperand tempReg;
    tempReg.addrMode = REGISTER_DIRECT;
    switch(type)
    {
        case ARM:
            tempReg.oprendVal = pick_one_free_temp_arm_register();
            tempReg.format = INTERGER_TWOSCOMPLEMENT;
        break;
        case VFP:
            tempReg.oprendVal = pick_one_free_vfp_register();
            tempReg.format = IEEE754_32BITS;
        break;
    }
    return tempReg;
}

/**
 * @brief 进行一次无用的封装，现在可以以operand为参数归还临时寄存器
 * @birth: Created by LGD on 20230130
 * @update: 20230226 归还寄存器应当依据其寄存器编号而非其数据编码类型
*/
void operand_recycle_temp_register(AssembleOperand tempReg)
{
    // switch(tempReg.format)
    // {
    //     case INTERGER_TWOSCOMPLEMENT:
    //         recycle_temp_arm_register(tempReg.oprendVal);
    //         break;
    //     case IEEE754_32BITS:
    //         recycle_vfp_register(tempReg.oprendVal);
    //         break;
    // }
    switch(register_type(tempReg.oprendVal))
    {
        case ARM:
            recycle_temp_arm_register(tempReg.oprendVal);
            break;
        case VFP:
            recycle_vfp_register(tempReg.oprendVal);
            break;
    }
}

/**
 * @brief 将浮点数字面量装填到一个浮点寄存器中
 * @param src 源字面量
*/
AssembleOperand operand_float_load_immediate(AssembleOperand src)
{
    AssembleOperand temp = operand_pick_temp_register(VFP);
    if(src.format == IEEE754_32BITS)
    {
        
    }
}

/**
 * @brief 封装直传函数，可以自行判断寄存器是什么类型
 * @param src 源寄存器
 * @param recycleSrc 是否回收源寄存器
 * @birth: Created by LGD on 20230201
*/
AssembleOperand operand_float_deliver(AssembleOperand src,bool recycleSrc)
{
    AssembleOperand temp;
    if(register_type(src.oprendVal) == ARM)
    {
        temp = operand_pick_temp_register(VFP);
        fmrs_and_fmsr_instruction("FMSR",src,temp,FloatTyID);
    }
    else
    {
        temp = operand_pick_temp_register(ARM);
        fmrs_and_fmsr_instruction("FMRS",temp,src,FloatTyID);
    }
    if(recycleSrc)
        operand_recycle_temp_register(src);
    return temp;
}

/**
 * @brief 封装转换函数，可以自行判断res是什么数据类型
 * @param src 源寄存器
 * @param recycleSrc 是否回收源寄存器
 * 
 * @param pickTempReg 是否申请一个临时寄存器，如果选否，需要为第四函数赋值，同时返回值将为nullop 否则赋nullop
 * @param res 目标寄存器
 * @birth: Created by LGD on 20230201
*/
AssembleOperand operand_float_convert(AssembleOperand src,bool recycleSrc)
{
    AssembleOperand temp;
    if(src.format == INTERGER_TWOSCOMPLEMENT)
    {
        temp = operand_pick_temp_register(VFP);
        fsito_and_fuito_instruction("FSITO",temp,src,FloatTyID);
    }
    else
    {   
        temp = operand_pick_temp_register(VFP);
        ftost_and_ftout_instruction("FTOST",temp,src,FloatTyID);
    }
    if(recycleSrc)
        operand_recycle_temp_register(src);
    return temp;
}

/**
 * @brief 将立即数用FLD伪指令读取到临时寄存器中，LDR / FLD通用
 * @birth: Created by LGD on 20230202
*/
AssembleOperand operand_ldr_immed(AssembleOperand src,ARMorVFP type)
{
    AssembleOperand temp;
    switch(type)
    {
        case ARM:
        {
            temp = operand_pick_temp_register(ARM);
            pseudo_ldr("LDR",temp,src);
        }
        break;
        case VFP:
        {
            temp = operand_pick_temp_register(VFP);
            pseudo_fld("FLD",temp,src,FloatTyID);
        }
        break;
    }
    return temp;
}

/**
 * @brief 判断一个operand是否在指令中
 * @birth: Created by LGD on 20230328
*/
bool opernad_is_in_instruction(AssembleOperand op)
{
    return (op.addrMode == IMMEDIATE);
}

