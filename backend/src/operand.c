#include "arm.h"
#include "arm_assembly.h"

#include "interface_zzq.h"


struct _operand r0 = {REGISTER_DIRECT,R0,0};
struct _operand r1 = {REGISTER_DIRECT,R1,0};
struct _operand immedOp = {IMMEDIATE,0,0};
struct _operand sp = {REGISTER_DIRECT,SP,0};
struct _operand lr = {REGISTER_DIRECT,LR,0};
struct _operand fp = {REGISTER_DIRECT,R7,0};
struct _operand pc = {REGISTER_DIRECT,PC,0};

struct _operand sp_indicate_offset = {
                REGISTER_INDIRECT_WITH_OFFSET,
                SP,
                0
};

struct _operand fp_indicate_offset = {
    .addrMode = REGISTER_INDIRECT_WITH_OFFSET,
    .oprendVal = FP,
    .addtion = 0
};

struct _operand param_push_op = {
    .addrMode = REGISTER_INDIRECT_PRE_INCREMENTING,
    .oprendVal = SP,
    .addtion = -4
};

struct _operand r027[8] = {{.addrMode=REGISTER_DIRECT,.oprendVal=R0},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R1},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R2},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R3},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R4},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R5},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R6},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R7}};
struct _operand trueOp = {IMMEDIATE,1,0};
struct _operand falseOp = {IMMEDIATE,0,0};


/**
 * @brief 由于仅通过operand判断需不需要临时寄存器需要额外的归类方法
 * @birth: Created by LGD on 20230130
*/
RegorMem judge_operand_in_RegOrMem(AssembleOperand op)
{
    switch(op.addrMode)
    {
        case REGISTER_INDIRECT:
        case REGISTER_INDIRECT_POST_INCREMENTING:
        case REGISTER_INDIRECT_PRE_INCREMENTING:
        case REGISTER_INDIRECT_WITH_OFFSET:
            return IN_MEMORY;
        case REGISTER_DIRECT:
            return IN_REGISTER;
        case IMMEDIATE:
            return IN_INSTRUCTION;
        case NONE_ADDRMODE:
            return UNALLOCATED;
    }
}

/**
 * @brief 依据Value* 返回 operand
 * @birth: Created by LGD on 2023-3-16
 * @update: 2023-4-4 将栈顶指针改为栈帧
 * @update: 2023-4-20 清空内存
*/
AssembleOperand ValuetoOperand(Instruction* this,Value* var)
{
    AssembleOperand op;
    memset(&op,0,sizeof(AssembleOperand));
    RegorMem rOm = get_variable_place(this,var);
    switch(rOm)
    {
        case IN_MEMORY:
            op.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
            op.oprendVal = FP;
            op.addtion = get_variable_register_order_or_memory_offset_test(this,var);
            op.offsetType = OFFSET_IMMED;
        break;
        case IN_DATA_SEC:
            op.addrMode = LABEL_MARKED_LOCATION;
            op.oprendVal = var->name;
        break;
        case IN_REGISTER:
            op.addrMode = REGISTER_DIRECT;
            op.oprendVal = get_variable_register_order_or_memory_offset_test(this,var);
        break;
        case IN_INSTRUCTION:
            op.addrMode = IMMEDIATE;
            op.oprendVal = value_getConstant(var);
        break;
        case UNALLOCATED:
            op.addrMode = NONE_ADDRMODE;
            op.oprendVal = 0;
    }
    op.format = valueFindFormat(var);
    return op;
}

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
 * @brief 创建一个立即数操作数
 * @birth: Created by LGD on 2023-5-1
*/
struct _operand operand_create_immediate_op(int immd)
{
    struct _operand op = {.addrMode = IMMEDIATE,.oprendVal = immd};
    return op;
}

/**
 * @brief 比较两个操作数是否一致
*/
bool operand_is_same(struct _operand dst,struct _operand src)
{
    if(memcmp(&dst,&src,sizeof(struct _operand)))
        return false;
    else 
        return true;
}


/**
 * @brief 把暂存器存器再封装一层
 * @birth: Created by LGD on 20230130
 * @update: 2023-4-20 初始化内存
*/
AssembleOperand operand_pick_temp_register(enum _ARMorVFP type)
{
    AssembleOperand tempReg;
    memset(&tempReg,0,sizeof(AssembleOperand));
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
 * @update: 2023-4-20 初始化内存
*/
AssembleOperand operand_load_immediate(AssembleOperand src,enum _ARMorVFP type)
{
    AssembleOperand temp;
    memset(&temp,0,sizeof(AssembleOperand));
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
 * @brief 立即数读取到指令寄存器
 * @update:Created by LGD on 2023-4-11
 *         2023-7-18 删去不必要的参数type
*/
AssembleOperand operand_load_immediate_to_specified_register(AssembleOperand src,AssembleOperand dst)
{
    enum _ARMorVFP regType = register_type(dst.oprendVal);
    switch(regType)
    {
        case ARM:
            pseudo_ldr("LDR",dst,src);
        break;
        case VFP:
            pseudo_fld("FLD",dst,src,FloatTyID);
        break;
    }
    return dst;
}

/**
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器,这次，你可以自定义用什么寄存器加载了
 * @birth: Created by LGD on 20230130
 * @update: 2023-4-10 更改名字为load_from_memory
 * @update: 2023-4-20 初始化时清空内存
*/
AssembleOperand operand_load_from_memory(AssembleOperand op,enum _ARMorVFP type)
{
    AssembleOperand tempReg;
    memset(&tempReg,0,sizeof(AssembleOperand));
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
                    mem2reg(tempReg, op);
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
 * @brief 
 * @param op 需要读取到寄存器的operand
 * @param type 读取到的寄存器类型
 * @update: Created by LGD on 2023-4-11
*/
AssembleOperand operand_load_from_memory_to_spcified_register(AssembleOperand op,AssembleOperand dst)
{
    enum _ARMorVFP regType = register_type(dst.oprendVal);
    switch(judge_operand_in_RegOrMem(op))
    {
        case IN_REGISTER:
            return op;
        case IN_MEMORY:
            switch(regType)
            {
                case ARM:
                    mem2reg(dst, op);
                break;
                case VFP:
                    vfp_memory_access_instructions("FLD",dst,op,FloatTyID);
                break;
            }
            return dst;
    }
    assert(judge_operand_in_RegOrMem(op) != IN_INSTRUCTION);
}

/**
 * @brief 将一个操作数加载到指定的寄存器，无论其在任何位置均确保生产合法的指令，且不破坏其他的寄存器
 * @birth: Created by LGD on 2023-5-14
 * @update: 2023-7-18 目标可以为ARM/VFP的寄存器
*/
void operand_load_to_specified_register(struct _operand oriOp,struct _operand tarOp)
{
    assert(tarOp.addrMode == REGISTER_DIRECT && "该方法要求目的操作数必须是寄存器！");


}


/**
 * @brief 从内存或者将立即数加载到一个寄存器中，将操作数加载到指定寄存器，该方法不负责检查该寄存器是否被使用
 * @param tarOp 如果指定为一个register operand，加载到该operand；若指定为nullop，选择一个operand
 * @param type 若选择nullop，需要用第三个参数指定传递的类型
 * @birth: Created by LGD on 2023-5-1
 * @update: 当目标为null时，允许选择寄存器类型
*/
AssembleOperand operand_load_to_register(AssembleOperand srcOp,AssembleOperand tarOp,...)
{
    if(operand_is_none(tarOp))
    {
        //处理不定长参数列表
        va_list ops;
        va_start(ops,tarOp);
        enum _ARMorVFP type = va_arg(ops,enum _ARMorVFP type);
        va_end(ops);

        if(operand_is_in_memory(srcOp))
        {
            tarOp = operand_load_from_memory(srcOp,type);
        }
        else if(opernad_is_immediate(srcOp))
        {
            tarOp = operand_load_immediate(srcOp,type);
        }
        else if(operand_is_in_register(srcOp))
        {
            if(operand_get_regType(srcOp) != type)
            {
                if(type == ARM)
                {
                    struct _operand temp = operand_pick_temp_register(ARM);

                }
                else if(type ==VFP)
            }
            tarOp = srcOp;
        }
    }
    else
    {
        movii(tarOp,srcOp);
    }
    return tarOp;
}


/**
 * @brief 判断一个operand是否在指令中
 * @birth: Created by LGD on 20230328
*/
bool opernad_is_immediate(AssembleOperand op)
{
    return (op.addrMode == IMMEDIATE);
}

/**
 * @brief 判断一个operand是否在内存中
 * @birth: Created by LGD on 2023-4-24
*/
bool operand_is_in_memory(AssembleOperand op)
{
    return (op.addrMode == REGISTER_INDIRECT ||   //寄存器间接寻址 LDR R0, [R1]
            op.addrMode == REGISTER_INDIRECT_WITH_OFFSET ||     //前变址
            op.addrMode == REGISTER_INDIRECT_PRE_INCREMENTING ||  //自动变址
            op.addrMode == REGISTER_INDIRECT_POST_INCREMENTING); //后变址
}

/**
 * @brief 判断一个操作数是否是空指针
 * @birth: Created by LGD on 2023-5-2
*/
bool operand_is_none(AssembleOperand op)
{
    return (op.addrMode == NONE_ADDRMODE && op.addtion == 0 && op.oprendVal == 0);
}

/**
* @brief 判断当前操作数是否未分配
* @brith: Created by LGD on 2023-7-9
*/
bool operand_is_unallocated(AssembleOperand op)
{
    return (op.addrMode == NONE_ADDRMODE);
}

/**
 * @brief 判断一个operand是否在寄存器中
 * @birth: Created by LGD on 2023-4-24
*/
bool operand_is_in_register(AssembleOperand op)
{
    return (op.addrMode == REGISTER_DIRECT);
}

/**
 * @brief 判断一个寄存器属于R4-R12(不包括R7)
 * @birth: Created by LGD on 2023-5-13
*/
bool operand_is_via_r4212(struct _operand reg)
{
    return (reg.oprendVal >= R4 && reg.oprendVal <= R12 && reg.oprendVal!=R7);
}


/**
 * @brief 创建一个相对FP/SP偏移的寻址方式操作数
 * @birth: Created by LGD on 2023-5-3
*/
struct _operand operand_create_relative_adressing(RegisterOrder SPorFP,enum _OffsetType immedORreg,int offset)
{
    struct _operand memOff = 
    {
        .addrMode = REGISTER_INDIRECT_WITH_OFFSET,
        .oprendVal = SPorFP,
        .offsetType = immedORreg,
        .addtion = offset
    };
    //为寄存器时
    if(immedORreg == OFFSET_IN_REGISTER)return memOff;
    //为立即数时
    if(abs(offset) <= ARM_WORD_IMMEDIATE_OFFSET_RANGE)return memOff;
    //超过立即数限制时，需要调用临时寄存器
    else
    {
        assert("function hasn't supported out of range");
        struct _operand immd = operand_create_immediate_op(offset);
        struct _operand regStoreImmd = operand_load_immediate(immd,ARM);
        return regStoreImmd;
    }
    
}

/**
 * @brief 创建一个相对FP/SP偏移的寻址方式操作数
 *        这个方法不负责回收多余的寄存器
 * @birth: Created by LGD on 2023-5-3
*/
struct _operand operand_create2_relative_adressing(RegisterOrder SPorFP,struct _operand offset)
{
    struct _operand memOff = \
    {.addrMode = REGISTER_INDIRECT_WITH_OFFSET,.oprendVal = SPorFP};
    //为寄存器时
    if(operand_is_in_register(offset))
    {
        memOff.offsetType = OFFSET_IN_REGISTER;
        memOff.addtion = offset.oprendVal;
    }
    //为立即数时
    if(opernad_is_immediate(offset))
    {
        //为合法立即数时
        if(abs(offset.oprendVal) <= ARM_WORD_IMMEDIATE_OFFSET_RANGE)
        {
            memOff.offsetType = OFFSET_IMMED;
            memOff.addtion = offset.oprendVal;
        }
        //超过立即数限制时，需要调用临时寄存器
        else
        {
            struct _operand regStoreImmd = operand_load_immediate(offset,ARM);
            memOff.offsetType = OFFSET_IN_REGISTER;
            memOff.addtion = regStoreImmd.oprendVal;
        }
    }
    //在内存中时
    if(operand_is_in_memory(offset))
    {
        struct _operand regStore = operand_load_from_memory(offset,ARM);
        memOff.offsetType = OFFSET_IN_REGISTER;
        memOff.addtion = regStore.oprendVal;
    }
    return memOff;
    
    
}


/**
 * @brief 获取操作数的立即数
 * @birth: Created by LGD on 2023-4-18
*/
size_t operand_get_immediate(AssembleOperand op)
{
    assert(op.addrMode == IMMEDIATE);
    return op.oprendVal;
}

/**
 * @brief 设置操作数的移位操作
 * @birth: Created by LGD on 2023-4-20
*/
void operand_set_shift(AssembleOperand* rm,enum SHIFT_WAY shiftWay,size_t shiftNum)
{
    rm->shiftWay = shiftWay;
    rm->shiftNum = shiftNum;
}

/**
 * @brief 获取操作数的寄存器类型
 * @birth: Created by LGD on 2023-7-18
*/
enum _ARMorVFP operand_get_regType(struct _operand op)
{
    return register_type(op.oprendVal);
}

/**
 * @brief 将操作数取到一个寄存器中，或者其他定制化需求
 *        
 * @param op 需要被加载到其他位置的操作数，如果第四个参数置为0，则视为全部情况加载
 * @param mask 为1时表示第四个参数为屏蔽选项，及当前操作数处于这些位置时不加载到寄存器
 *             为0时表示第四个参数为选择选项，及当前操作数处于这些位置时要加载到寄存器
 *             可供的选项由第三个参数提供
 * @param rom  选项包括 IN_MEMORY IN_REGISTER IN_INSTRUCTION 0
 *              为0时代表所有情况
 *              当选项为寄存器时，只有在寄存器类型不一致的情况下才会加载到寄存器
 *              其余情况下，函数返回源操作数自身
 * @birth: Created by LGD on 2023-4-24
 * @update: 2023-7-11 修复了优先级导致判断错误的BUG
*/
struct _operand operandConvert(struct _operand op,enum _ARMorVFP aov,bool mask,enum _RegorMem rom)
{
    struct _operand cvtOp = op;
    assert(!(mask == true && rom == false) && "operandConvert failed");

    if(!mask)
    {
        if( (((rom & IN_MEMORY) == IN_MEMORY) || !rom) &&  
            judge_operand_in_RegOrMem(op) == IN_MEMORY)
            cvtOp = operand_load_from_memory(op,aov);

        if((((rom & IN_INSTRUCTION) == IN_INSTRUCTION) || !rom ) && 
            judge_operand_in_RegOrMem(op) == IN_INSTRUCTION)
            cvtOp = operand_load_immediate(op,aov);

        if((((rom & IN_INSTRUCTION) == IN_REGISTER) || !rom ) && 
            (judge_operand_in_RegOrMem(op) == IN_REGISTER) &&
            operand_get_regType(op) != aov)
            cvtOp = operand_load_to_register(op,nullop,aov);

    }
    else
    {
        if( ((rom & IN_MEMORY == IN_MEMORY) || !rom) && 
            judge_operand_in_RegOrMem(op) == IN_MEMORY)
            return cvtOp;
        if(((rom & IN_INSTRUCTION == IN_INSTRUCTION) || !rom ) &&
            judge_operand_in_RegOrMem(op) == IN_INSTRUCTION)
            return cvtOp;
        if(((rom & IN_INSTRUCTION == IN_REGISTER) || !rom ) &&
            judge_operand_in_RegOrMem(op) == IN_REGISTER)
            return cvtOp;

        return operandConvert(op,aov,0,0);
    }
    return cvtOp;
}

/**
 *@brief 这个方法可以更改操作数的寻址方式
 *@birth: Created by LGD on 2023-5-29
*/
void operand_change_addressing_mode(struct _operand* op,AddrMode addrMode)
{
    op->addrMode = addrMode;
}

/**
 * @brief 判断一个操作数是否是合法立即数
 * @birth:Created by LGD on 2023-7-18
*/
bool operand_check_immed_valid(struct _operand op)
{
    assert(op.addrMode == IMMEDIATE && "Could only check if immediate is valid");
    return check_immediate_valid(op.oprendVal);
}

