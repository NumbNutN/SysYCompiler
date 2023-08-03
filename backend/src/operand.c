#include "arm.h"
#include "arm_assembly.h"

#include "interface_zzq.h"

#include <stdarg.h>


struct _operand r0 = {REGISTER_DIRECT,R0,0};
struct _operand r1 = {REGISTER_DIRECT,R1,0};
struct _operand r2 = {REGISTER_DIRECT,R2,0};
struct _operand immedOp = {IMMEDIATE,0,0,.format=INTERGER_TWOSCOMPLEMENT};
struct _operand sp = {REGISTER_DIRECT,SP,0,.format=INTERGER_TWOSCOMPLEMENT};
struct _operand lr = {REGISTER_DIRECT,LR,0,.format=INTERGER_TWOSCOMPLEMENT};
struct _operand fp = {REGISTER_DIRECT,R7,0,.format=INTERGER_TWOSCOMPLEMENT};
struct _operand pc = {REGISTER_DIRECT,PC,0,.format=INTERGER_TWOSCOMPLEMENT};

struct _operand sp_indicate_offset = {
                REGISTER_INDIRECT_WITH_OFFSET,
                SP,
                0,
                .format=INTERGER_TWOSCOMPLEMENT
};

struct _operand fp_indicate_offset = {
    .addrMode = REGISTER_INDIRECT_WITH_OFFSET,
    .oprendVal = FP,
    .addtion = 0
};

struct _operand param_push_op_interger = {
    .addrMode = REGISTER_INDIRECT_PRE_INCREMENTING,
    .oprendVal = SP,
    .addtion = -4,
    .format = INTERGER_TWOSCOMPLEMENT,
    .point2spaceFormat = INTERGER_TWOSCOMPLEMENT
};

struct _operand param_push_op_float = {
    .addrMode = REGISTER_INDIRECT_WITH_OFFSET,
    .oprendVal = SP,
    .addtion = -4,
    .format = INTERGER_TWOSCOMPLEMENT,
    .point2spaceFormat = IEEE754_32BITS
};

struct _operand r027[8] = {{.addrMode=REGISTER_DIRECT,.oprendVal=R0,.format=NO_FORMAT},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R1,.format=NO_FORMAT},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R2,.format=NO_FORMAT},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R3,.format=NO_FORMAT},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R4,.format=NO_FORMAT},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R5,.format=NO_FORMAT},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R6,.format=NO_FORMAT},
                            {.addrMode=REGISTER_DIRECT,.oprendVal=R7,.format=NO_FORMAT}};

struct _operand r023_int[4] = {
    {.addrMode=REGISTER_DIRECT,.oprendVal=R0,.format=INTERGER_TWOSCOMPLEMENT},
    {.addrMode=REGISTER_DIRECT,.oprendVal=R1,.format=INTERGER_TWOSCOMPLEMENT},
    {.addrMode=REGISTER_DIRECT,.oprendVal=R2,.format=INTERGER_TWOSCOMPLEMENT},
    {.addrMode=REGISTER_DIRECT,.oprendVal=R3,.format=INTERGER_TWOSCOMPLEMENT}
};

struct _operand r023_float[4] = {
    {.addrMode=REGISTER_DIRECT,.oprendVal=R0,.format=IEEE754_32BITS},
    {.addrMode=REGISTER_DIRECT,.oprendVal=R1,.format=IEEE754_32BITS},
    {.addrMode=REGISTER_DIRECT,.oprendVal=R2,.format=IEEE754_32BITS},
    {.addrMode=REGISTER_DIRECT,.oprendVal=R3,.format=IEEE754_32BITS}
};

struct _operand s023_float[4] = {
    {.addrMode=REGISTER_DIRECT,.oprendVal=S0,.format=IEEE754_32BITS},
    {.addrMode=REGISTER_DIRECT,.oprendVal=S1,.format=IEEE754_32BITS},
    {.addrMode=REGISTER_DIRECT,.oprendVal=S2,.format=IEEE754_32BITS},
    {.addrMode=REGISTER_DIRECT,.oprendVal=S3,.format=IEEE754_32BITS}
};

struct _operand trueOp = {IMMEDIATE,1,0,.format=INTERGER_TWOSCOMPLEMENT};
struct _operand falseOp = {IMMEDIATE,0,0,.format=INTERGER_TWOSCOMPLEMENT};

struct _operand floatZeroOp = {.addrMode=IMMEDIATE,.oprendVal=0,.addtion=0,.format=IEEE754_32BITS};

struct _operand returnIntOp = {
    .addrMode = REGISTER_DIRECT,
    .oprendVal = R0,
    .addtion = 0,
    .format = INTERGER_TWOSCOMPLEMENT
};

struct _operand returnFloatSoftOp = {
    .addrMode = REGISTER_DIRECT,
    .oprendVal = R0,
    .addtion = 0,
    .format = IEEE754_32BITS
};

struct _operand returnFloatOp = {
    .addrMode = REGISTER_DIRECT,
    .oprendVal = S0,
    .addtion = 0,
    .format = IEEE754_32BITS
};

AssembleOperand operand_load_to_register(AssembleOperand srcOp,AssembleOperand tarOp,...);

/**
 * @brief 由于仅通过operand判断需不需要临时寄存器需要额外的归类方法
 * @birth: Created by LGD on 20230130
 * @update: 2023-7-24 断言寻址模式是已知的
*/
RegorMem operand_in_regOrmem(AssembleOperand op)
{
    switch(op.addrMode)
    {
        case REGISTER_INDIRECT:
        case REGISTER_INDIRECT_POST_INCREMENTING:
        case REGISTER_INDIRECT_PRE_INCREMENTING:
        case REGISTER_INDIRECT_WITH_OFFSET:
            return IN_STACK_SECTION;
        case REGISTER_DIRECT:
            return IN_REGISTER;
        case IMMEDIATE:
            return IN_INSTRUCTION;
        case NONE_ADDRMODE:
            return UNALLOCATED;
        case LABEL_MARKED_LOCATION:
            return IN_LITERAL_POOL;
        default:
            assert(false && "unrecognize addressing mode");
    }
}

/**
 * @brief 依据Value* 返回 operand
 * @birth: Created by LGD on 2023-3-16
 * @update: 2023-4-4 将栈顶指针改为栈帧
 * @update: 2023-4-20 清空内存
 *          2023-7-29 现在ValuetoOperand不会再判断IN_DATA_SEC
*/
AssembleOperand ValuetoOperand(Instruction* this,Value* val)
{
    AssembleOperand op;
    memset(&op,0,sizeof(AssembleOperand));
    RegorMem rOm = get_variable_place(this,val);
    switch(rOm)
    {
        case IN_STACK_SECTION:
            op.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
            op.oprendVal = FP;
            op.addtion = get_variable_register_order_or_memory_offset_test(this,val);
            op.offsetType = OFFSET_IMMED;
            op.format = INTERGER_TWOSCOMPLEMENT;
            op.point2spaceFormat = valueFindFormat(val);
        break;
        case IN_DATA_SEC:
            assert(false && "ValuetoOperand方法不再受理在Data_Sec的情况");
        break;
        case IN_LITERAL_POOL:
            op.addrMode = LABEL_MARKED_LOCATION;
            op.oprendVal = val->name;
            op.format = INTERGER_TWOSCOMPLEMENT;
            op.point2spaceFormat = valueFindFormat(val);
        break;
        case IN_REGISTER:
            op.addrMode = REGISTER_DIRECT;
            op.oprendVal = get_variable_register_order_or_memory_offset_test(this,val);
            op.format = valueFindFormat(val);
        break;
        case IN_INSTRUCTION:
            op.addrMode = IMMEDIATE;
            op.oprendVal = value_getConstant(val);
            op.format = valueFindFormat(val);
        break;
        case UNALLOCATED:
            op.addrMode = NONE_ADDRMODE;
            op.oprendVal = 0;
            op.format = valueFindFormat(val);
    }
    

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
 * @update: 2023-7-23 支持浮点立即数的创建（IEEE754格式）
*/
struct _operand operand_create_immediate_op(uint32_t immd,enum _DataFormat format)
{
    struct _operand op = {.addrMode = IMMEDIATE,.oprendVal = immd,.format = format};
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
 * @brief 判断一个operand是否在指令中
 * @birth: Created by LGD on 20230328
*/
bool operand_is_immediate(AssembleOperand op)
{
    return (op.addrMode == IMMEDIATE || op.addrMode == LABEL_MARKED_LOCATION);
}

/**
 * @brief 判断一个操作数是否在栈区
 * @birth: Created by LGD on 2023-7-29
*/
bool operand_is_in_stack_section(AssembleOperand op)
{
    return (op.addrMode == REGISTER_INDIRECT ||   //寄存器间接寻址 LDR R0, [R1]
            op.addrMode == REGISTER_INDIRECT_WITH_OFFSET ||     //前变址
            op.addrMode == REGISTER_INDIRECT_PRE_INCREMENTING ||  //自动变址
            op.addrMode == REGISTER_INDIRECT_POST_INCREMENTING);  //后变址
}

/**
 * @brief 判断一个操作数是否在数据段中
 * @birth: Created by LGD on 2023-7-29
*/
bool operand_is_in_data_section(AssembleOperand op)
{
    return op.addrMode == LABEL_MARKED_LOCATION;
}

/**
 * @brief 判断一个operand是否在内存中
 * @birth: Created by LGD on 2023-4-24
 * @update: 在数据段中也会视为operand_is_in_memory
*/
bool operand_is_in_memory(AssembleOperand op)
{
    return (operand_is_in_stack_section(op)); 
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
 * @brief 获取操作数寄存器或立即数的编码格式
 * @birth: Created by LGD on 2023-7-24
*/
enum _DataFormat operand_get_reg_format(struct _operand op)
{
    assert(op.format != NO_FORMAT && "当前寄存器格式为空");
    return op.format;
}

/**
 * @brief 获取操作数的编码格式
 * @birth: Created by LGD on 2023-7-23
 * @update: 2023-7-24 根据在寄存器、立即数和在内存返回不同的格式
*/
enum _DataFormat operand_get_format(struct _operand op)
{
    if(operand_is_in_memory(op))
    {
        assert(op.point2spaceFormat != NO_FORMAT && "当前间址内存格式为空");
        return op.point2spaceFormat;
    }
    else
        return operand_get_reg_format(op);
}

/**
 * @brief 把暂存器存器再封装一层
 * @birth: Created by LGD on 20230130
 * @update: 2023-4-20 初始化内存
 *          2023-7-25 该方法不再有权限决定分配寄存器的数据格式
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
            tempReg.format = NO_FORMAT;
        break;
        case VFP:
            tempReg.oprendVal = pick_one_free_vfp_register();
            tempReg.format = NO_FORMAT;
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
 * @update: 2023-7-19 可以指定目标寄存器
 * @TODO: 2023-7-22 可以被舍弃
*/
AssembleOperand operand_float_deliver(AssembleOperand src,struct _operand tar,bool recycleSrc)
{
    if(operand_is_none(tar))
    {
        AssembleOperand temp;
        if(operand_get_regType(src) == ARM)
        {
            temp = operand_pick_temp_register(VFP);
            fmrs_and_fmsr_instruction("FMSR",src,temp,FloatTyID);
        }
        else
        {
            temp = operand_pick_temp_register(ARM);
            fmrs_and_fmsr_instruction("FMRS",temp,src,FloatTyID);
        }
        tar = temp;
    }
    else{
        if(operand_get_regType(src) == ARM)
        {
            assert(operand_get_regType(tar) == VFP && "target Reg 不符合传递要求");
            fmrs_and_fmsr_instruction("FMSR",src,tar,FloatTyID);
        }
        else{
            assert(operand_get_regType(tar) == ARM && "target Reg 不符合传递要求");
            fmrs_and_fmsr_instruction("FMRS",tar,src,FloatTyID);
        }
    }
    if(recycleSrc)
        operand_recycle_temp_register(src);
    return tar;
}

/**
 * @brief 将浮点数从寄存器转换为整数
 * @birth: Created by LGD on 2023-7-19
 * @update: 2023-7-19 不再支持申请临时寄存器
*/
AssembleOperand operand_regFloat2Int(AssembleOperand src,struct _operand tar)
{
    AssembleOperand temp;
    enum _ARMorVFP type;
    assert(operand_is_in_register(src) && "当前转换方法仅支持寄存器");
    assert(operand_get_regType(src)== VFP && "源寄存器只支持IEEE754");

    if(operand_get_regType(src) == VFP)
    {
        //如果目标为VFP
        if(operand_get_regType(tar) == VFP)
            ftosi_and_ftout_instruction("vcvt.s32.f32",tar,src,FloatTyID);
        //目标为arm
        else if(operand_get_regType(tar) == ARM)
        {
            temp = operand_pick_temp_register(VFP);
            ftosi_and_ftout_instruction("vcvt.s32.f32",temp,src,FloatTyID);
            operand_float_deliver(temp,tar,true);
        }
    }
    return temp;
}

/**
 * @brief 将整数从寄存器转换为浮点数
 * @birth: Created by LGD on 2023-7-19
 * @update: 2023-7-19 不再支持申请临时寄存器
 *          2023-7-25 修改判断源寄存器为VFP的方式为看寄存器序号而非格式
*/
AssembleOperand operand_regInt2Float(AssembleOperand src,struct _operand tar)
{
    AssembleOperand temp;
    enum _ARMorVFP type;
    assert(operand_is_in_register(src) && "当前转换方法仅支持寄存器");
    assert(operand_get_regType(src)== VFP && "源寄存器只支持IEEE754");

    //指定目标寄存器
    //如果目标为VFP
    if(operand_get_regType(tar) == VFP)
        fsito_and_fuito_instruction("vcvt.f32.s32",tar,src,FloatTyID);
    //目标为arm
    else if(operand_get_regType(tar) == ARM)
    {
        struct _operand temp = operand_pick_temp_register(VFP);
        fsito_and_fuito_instruction("vcvt.f32.s32",temp,src,FloatTyID);
        operand_float_deliver(temp,tar,true);
    }
    return temp;
}

/**
 * @brief 封装转换函数，判断src的格式确定转换格式
 * @param src 源寄存器
 * @param tar 目标寄存器，如果留空，则选取临时寄存器，同时需要设置第三个参数确定寄存器类型
 * @param pickTempReg 是否申请一个临时寄存器，如果选否，需要为第四函数赋值，同时返回值将为nullop 否则赋nullop

 * @birth: Created by LGD on 20230201
*/
struct _operand operand_r2r_cvt(struct _operand src,struct _operand tar,...)
{
    AssembleOperand temp;
    enum _ARMorVFP type;
    if(operand_is_none(tar))
    {
        //处理不定长参数列表
        va_list ops;
        va_start(ops,tar);
        type = va_arg(ops,enum _ARMorVFP);
        va_end(ops);
        struct _operand temp = operand_pick_temp_register(VFP);
        if(src.format == IEEE754_32BITS)
            operand_regFloat2Int(src,temp);
        else if(src.format == INTERGER_TWOSCOMPLEMENT)
            operand_regInt2Float(src,temp);
        return temp;
    }
    else{
        if(src.format == IEEE754_32BITS)
            return operand_regFloat2Int(src,tar);
        else if(src.format == INTERGER_TWOSCOMPLEMENT)
            return operand_regInt2Float(src,tar);
    }
}

/**
 * @brief 立即数读取到指令寄存器
 * @update:Created by LGD on 2023-4-11
 *         2023-7-18 删去不必要的参数type
 *          2023-7-19 保持格式一致
 *          2023-7-25 检查数据格式
 *          2023-7-25 对浮点字面量进行类型转变
*/
AssembleOperand operand_load_immediate_to_specified_register(AssembleOperand src,AssembleOperand dst)
{
    assert(operand_is_immediate(src) && "源必须为立即数");
    assert(src.format != NO_FORMAT && "源必须有格式");
    assert((src.format == dst.format ||
            dst.format == NO_FORMAT) &&
             "数据格式必须一致");

    if(dst.format == NO_FORMAT) dst.format = src.format;

    enum _ARMorVFP regType = register_type(dst.oprendVal);
    switch(regType)
    {
        case ARM:
        {
            //只有当立即数为字面量而非标号时，检测合法性才有意义
            if(src.addrMode == IMMEDIATE)
            {
                if(check_immediate_valid(src.oprendVal))
                    general_data_processing_instructions_extend(MOV, NONESUFFIX, false, dst,src,nullop);
                else
                {
                    //movw will zero the upper 16 bits. So there's a order to first w then t
                    struct _operand h32 = operand_create_immediate_op((uint32_t)src.oprendVal >> 16, INTERGER_TWOSCOMPLEMENT);
                    struct _operand l32 = operand_create_immediate_op(src.oprendVal & 0xFFFF,INTERGER_TWOSCOMPLEMENT);
                    general_data_processing_instructions_extend(MOVW, NONESUFFIX, false, dst,l32,nullop);
                    general_data_processing_instructions_extend(MOVT, NONESUFFIX, false, dst,h32,nullop);
                }
            }
            else if(src.addrMode == LABEL_MARKED_LOCATION){
                pseudo_ldr("LDR", dst, src);
            }
        }
        break;
        case VFP:
            pseudo_fld("FLDS",dst,src,FloatTyID);
        break;
    }
    return dst;
}

/**
 * @brief 将立即数用FLD伪指令读取到临时寄存器中，LDR / FLD通用
 * @birth: Created by LGD on 20230202
 * @update: 2023-4-20 初始化内存
 *          2023-7-18 更简洁的写法
*/
AssembleOperand operand_load_immediate(AssembleOperand src,enum _ARMorVFP type)
{
    AssembleOperand temp;
    temp = operand_pick_temp_register(type);
    temp = operand_load_immediate_to_specified_register(src,temp);
    return temp;
}

/**
 * @brief 
 * @param op 需要读取到寄存器的operand
 * @param type 读取到的寄存器类型
 * @update: Created by LGD on 2023-4-11
 *          2023-7-19 保持格式一致
 *          2023-7-25 检查数据格式
 *          2023-7-29 现在正式支持从数据段加载数据到寄存器
*/
AssembleOperand operand_load_from_memory_to_spcified_register(AssembleOperand src,AssembleOperand dst)
{
    assert(operand_is_in_memory(src) && "源必须要在内存中");
    assert(src.point2spaceFormat != NO_FORMAT && "源必须有格式");
    assert((src.point2spaceFormat == dst.format ||
            dst.format == NO_FORMAT) &&
             "数据格式必须一致");

    if(dst.format == NO_FORMAT) dst.format = src.point2spaceFormat;

    enum _ARMorVFP regType = register_type(dst.oprendVal);
    mem2reg(dst, src);
    return dst;
}

/**
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器,这次，你可以自定义用什么寄存器加载了
 * @birth: Created by LGD on 20230130
 * @update: 2023-4-10 更改名字为load_from_memory
 * @update: 2023-4-20 初始化时清空内存
 *          2023-7-18 更简洁的写法
*/
AssembleOperand operand_load_from_memory(AssembleOperand op,enum _ARMorVFP type)
{
    AssembleOperand tempReg = operand_pick_temp_register(type);
    tempReg = operand_load_from_memory_to_spcified_register(op,tempReg);
    return tempReg;
}

/**
 * @brief 将一个操作数加载到指定的寄存器，无论其在任何位置均确保生产合法的指令，且不破坏其他的寄存器
 * @birth: Created by LGD on 2023-5-14
 * @update: 2023-7-18 目标可以为ARM/VFP的寄存器
 *          2023-7-18 该方法可以判断源和目标是否是相同的寄存器
 *          2023-7-19 保持格式一致
*/
void operand_load_to_specified_register(struct _operand oriOp,struct _operand tarOp)
{
    assert(tarOp.addrMode == REGISTER_DIRECT && "load to register should specify a real register!");

    if(operand_is_same(tarOp, oriOp)) return;
    if(operand_is_in_memory(oriOp))
        operand_load_from_memory_to_spcified_register(oriOp,tarOp);
    else if(operand_is_immediate(oriOp))
        operand_load_immediate_to_specified_register(oriOp,tarOp);
    else if(operand_is_in_register(oriOp))
    {
        assert(oriOp.format != NO_FORMAT && "源必须有格式");
        assert((oriOp.format == tarOp.format ||
            tarOp.format == NO_FORMAT) &&
             "该方法不支持数据类型转换");

        if((operand_get_regType(tarOp) == ARM) && (operand_get_regType(oriOp) == ARM))
            general_data_processing_instructions(MOV,tarOp,nullop,oriOp,NONESUFFIX,false);
        else if((operand_get_regType(tarOp) == VFP) && (operand_get_regType(oriOp) == VFP))
            fabs_fcpy_and_fneg_instruction("FCPYS",tarOp,oriOp,FloatTyID);
        else if((operand_get_regType(tarOp) == VFP) && (operand_get_regType(oriOp) == ARM))
            fmrs_and_fmsr_instruction("FMSR",oriOp,tarOp,FloatTyID);
        else if((operand_get_regType(tarOp) == ARM) && (operand_get_regType(oriOp) == VFP))
            fmrs_and_fmsr_instruction("FMRS",tarOp,oriOp,FloatTyID);
    }
}

/**
 * @brief 从内存或者将立即数加载到一个寄存器中，将操作数加载到指定寄存器，该方法不负责检查该寄存器是否被使用
 * @param tarOp 如果指定为一个register operand，加载到该operand；若指定为nullop，选择一个operand
 * @param type 若选择nullop，需要用第三个参数指定传递的类型
 * @birth: Created by LGD on 2023-5-1
 * @update: 当目标为null时，允许选择寄存器类型
 *          2023-7-25 分配寄存器的数据格式与源一致
*/
AssembleOperand operand_load_to_register(AssembleOperand srcOp,AssembleOperand tarOp,...)
{
    //假如选择一个暂存寄存器负责加载
    if(operand_is_none(tarOp))
    {
        //处理不定长参数列表
        va_list ops;
        va_start(ops,tarOp);
        enum _ARMorVFP type = va_arg(ops,enum _ARMorVFP);
        va_end(ops);

        if(operand_is_in_memory(srcOp))
        {
            tarOp = operand_load_from_memory(srcOp,type);
        }
        else if(operand_is_immediate(srcOp))
        {
            tarOp = operand_load_immediate(srcOp,type);
        }
        //当源操作数在寄存器时
        //挑选一个寄存器进行寄存器间传递
        else if(operand_is_in_register(srcOp))
        {
            tarOp = operand_pick_temp_register(type);
            //该寄存器值的格式与源一致
            tarOp.format = srcOp.format;
            operand_load_to_specified_register(srcOp,tarOp);
        }
    //挑选指定的寄存器
    }
    else
    {
        operand_load_to_specified_register(srcOp,tarOp);
    }
    return tarOp;
}

/**
 * @brief 将源转换格式后送入目标寄存器
 * @birth: Created by LGD on 2023-7-19
*/
struct _operand operand_load_to_reg_cvt(struct _operand src,struct _operand tar,...)
{
    AssembleOperand temp;
    enum _ARMorVFP type;
    if(operand_is_none(tar))
    {
        //处理不定长参数列表
        va_list ops;
        va_start(ops,tar);
        type = va_arg(ops,enum _ARMorVFP);
        va_end(ops);

        if(!operand_is_in_register(src))
        {
            temp = operand_load_to_register(src, nullop, VFP);
            operand_r2r_cvt(temp,temp);
        }
        else{
            operand_r2r_cvt(src,temp);
        }
        return temp;
    }
    //指定目的
    else{
        //源在寄存器
        if(operand_is_in_register(src))
        {
            operand_r2r_cvt(src,tar);
        }
        //源不在寄存器
        else{
            temp = operand_load_to_register(src, nullop, VFP);
            operand_r2r_cvt(temp,tar);
        }
    }
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
        struct _operand immd = operand_create_immediate_op(offset,INTERGER_TWOSCOMPLEMENT);
        struct _operand regStoreImmd = operand_load_immediate(immd,ARM);
        return regStoreImmd;
    }
}

/**
 * @brief 创建一个相对FP/SP偏移的寻址方式操作数
 *        这个方法不负责回收多余的寄存器
 * @birth: Created by LGD on 2023-5-3
 * @TODO: 2023-7-22 写的太烂了
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
    if(operand_is_immediate(offset))
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
 * @brief 创建一个寄存器间接寻址操作数
 * @birth: Created by LGD on 2023-7-20
 * @update: 2023-7-24 需要指定间址指向的空间存放的数据类型
*/
struct _operand operand_Create_indirect_addressing(RegisterOrder reg,enum _DataFormat point2format)
{
    struct _operand reg_indirect = {
        .addrMode = REGISTER_INDIRECT,
        .oprendVal = reg,
        .addtion = 0,
        .format = INTERGER_TWOSCOMPLEMENT,
        .point2spaceFormat = point2format
    };
    return reg_indirect;
}

/**
 * @brief 创建一个寄存器直接寻址的操作数
 * @birth: Created by LGD on 2023-7-24
*/
struct _operand operand_create_register_direct(RegisterOrder reg,enum _DataFormat format)
{
    struct _operand op = {.addrMode = REGISTER_DIRECT,
                .oprendVal = reg,
                .addtion = 0,
                .format = format};
    return op;
}



/**
 * @brief 将操作数取到一个寄存器中，或者其他定制化需求
 *          当读取寄存器被阻止时，返回原来的值
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
        if( (((rom & IN_STACK_SECTION) == IN_STACK_SECTION) || !rom) &&  
            operand_is_in_memory(op))
            cvtOp = operand_load_from_memory(op,aov);

        if((((rom & IN_INSTRUCTION) == IN_INSTRUCTION) || !rom ) && 
            operand_is_immediate(op))
            cvtOp = operand_load_immediate(op,aov);

        if((((rom & IN_INSTRUCTION) == IN_REGISTER) || !rom ) && 
            operand_is_in_register(op) &&
            operand_get_regType(op) != aov)
            cvtOp = operand_load_to_register(op,nullop,aov);
    }
    else
    {
        if( (((rom & IN_STACK_SECTION) == IN_STACK_SECTION) || !rom) && 
            operand_is_in_memory(op))
            return cvtOp;
        if((((rom & IN_INSTRUCTION) == IN_INSTRUCTION) || !rom ) &&
            operand_is_immediate(op))
            return cvtOp;
        if((((rom & IN_INSTRUCTION) == IN_REGISTER) || !rom ) &&
            operand_is_in_register(op))
            return cvtOp;
        
        cvtOp = operand_load_to_register(op,nullop,aov);
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

/**
 * @brief 为操作数添加寄存器限制
 * @birth: Created by LGD on 2023-7-28
*/
void operand_add_register_limited(struct _operand op)
{
    add_register_limited(op.oprendVal);
}

/**
 * @brief 为操作数列表添加寄存器限制
 * @birth: Created by LGD on 2023-7-28
*/
void bash_operand_add_register_limited(struct _operand* opList)
{
    for(struct _operand* ptr = opList;!operand_is_none(*ptr);++ptr)
    {
        operand_add_register_limited(*ptr);
    }
}

/**
 * @brief 为操作数取消寄存器限制
 * @birth: Created by LGD on 2023-7-28
*/
void operand_remove_register_limited(struct _operand op)
{
    remove_register_limited(op.oprendVal);
}

/**
 * @brief 取消列表的所有寄存器限制
 * @birth: Created by LGD on 2023-7-28
*/
void bash_operand_remove_register_limited(struct _operand* opList)
{
    for(struct _operand* ptr = opList;!operand_is_none(*ptr);++ptr)
    {
        operand_remove_register_limited(*ptr);
    }
}