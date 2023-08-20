#include "variable_map.h"
#include "dependency.h"
#include "operand.h"
#include "arm.h"
#include "enum_2_str.h"

#include "interface_zzq.h"

/*
*将Instruction翻译为汇编后，
*将以以下的结构体形式存储每一条汇编指令
    typedef struct _assemNode
    {
        //操作码
        char opCode[4];
        //操作数
        AssembleOperand op[3];
        //定义了操作数的数量
        unsigned op_len;
        //指向下一个节点
        struct _assemNode* next;

    } assmNode;
*并以线性链表的形式串联起来
*最后可以用统一的打印指令打印成文本形式
*/


//链式汇编节点的头节点
assmNode* head;
//该指针始终指向当前上一个节点
assmNode* prev;

//目标操作数暂存器
// extern Value* tempReg; //算数运算暂存寄存器

TempReg TempARMRegList[] = \
        {
            {.reg = R0},
            {.reg = R1},
            {.reg = R2},
            {.reg = R3},
            {.reg = R4},
            {.reg = R5},
            {.reg = R6},
            {.reg = R8},
            {.reg = R9},
            {.reg = R10},
            {.reg = R11},
        };

TempReg TempVFPRegList[TEMP_VFP_REG_NUM];
TempReg AddtionalARMRegList[ADDITION_REG_NUM];



Stack* Free_Vps_Register_list;

/**
 * @brief 浮点空闲寄存器获取三件套  这是选取临时浮点寄存器保存操作数
*/
void Free_Vps_Register_Init()
{
    for(int i=0;i<TEMP_VFP_REG_NUM;i++)
    {
        TempVFPRegList[i].reg = S1+i;
        TempVFPRegList[i].isAviliable = true;
    }
}

RegisterOrder pick_one_free_vfp_register()
{   
    for(int i=0;i<TEMP_VFP_REG_NUM;i++)
    {
        if(TempVFPRegList[i].isAviliable)
        {
            TempVFPRegList[i].isAviliable = false;
            return TempVFPRegList[i].reg; 
        }
    }
    assert(0 && "No aviliable temp register");
}

void recycle_vfp_register(RegisterOrder reg)
{
    for(int i=0;i<TEMP_VFP_REG_NUM;i++)
    {
        if(reg == TempVFPRegList[i].reg)
            TempVFPRegList[i].isAviliable = true;
    }
    assert("Missing the temp regitser");
}


/**
 * @brief 初始化临时寄存器列表
 * @author Created by LGD on early age
 * @update 20221212 fix三个没有全部赋为isAviliable的bug
*/
void Init_arm_tempReg()
{

    for(int i=0;i<TEMP_ARM_REG_NUM;i++)
    {
        TempARMRegList[i].isAviliable = true;
        TempARMRegList[i].isLimited = false;
    }
}

/**
 * @brief 判断当前寄存器是否是限制级别寄存器
 * @birth: Created by LGD on 2023-5-4
 * @update: 2023-7-28 现在您每次调用仅能限制一个寄存器
*/
bool Is_limited_temp_register(RegisterOrder reg)
{
    int idx = 0;
    for(;idx <TEMP_ARM_REG_NUM;idx ++){
        if(TempARMRegList[idx].reg == reg)
        {
            //找到对应寄存器
            return TempARMRegList[idx].isLimited;
        }
    }
    assert(idx  != TEMP_ARM_REG_NUM && "当前寄存器不存在");
}

/**
 * @brief 添加新的限制级别
 * @birth: Created by LGD on 2023-5-4
 * @update: 2023-7-28 现在您每次调用仅能限制一个寄存器
*/
void add_register_limited(RegisterOrder limitedReg)
{
    int idx = 0;
    for(;idx <TEMP_ARM_REG_NUM;idx ++){
        if(TempARMRegList[idx].reg == limitedReg)
        {
            //找到对应寄存器
            TempARMRegList[idx].isLimited = true;
            break;
        }
    }
    assert(idx != TEMP_ARM_REG_NUM && "当前寄存器不存在");
}

/**
 * @brief 根据参数个数限制
 * @birth: Created by LGD on 2023-7-28
 * @update: 2023-7-28 只有序号小于4的参数才需要限制
*/
void add_parameter_limited(size_t regNum)
{
    int maxIdx = regNum > 4? 4:regNum;
    for(int regIdx=0;regIdx<maxIdx;++regIdx)
    {
        add_register_limited(regIdx);
    }
}

/**
 * @brief 移除一个限制级别，如果其本身没有这个限制级别，将忽略
 * @birth: Created by LGD on 2023-5-4
 * @update: 2023-7-28 现在您每次调用仅能限制一个寄存器
*/
void remove_register_limited(RegisterOrder limitedReg)
{
    int idx = 0;
    for(;idx <TEMP_ARM_REG_NUM;idx ++){
        if(TempARMRegList[idx].reg == limitedReg)
        {
            //找到对应寄存器
            TempARMRegList[idx].isLimited = false;
            break;
        }
    }
    assert(idx != TEMP_ARM_REG_NUM && "当前寄存器不存在");
}

/**
 * @brief 挑选一个闲置的临时寄存器 这个方法作为底层调用被相当多的方法依赖
 * @update: 2023-5-4 一个全局指示变量将提示现在不应当选择哪些寄存器作为临时寄存器
 * @update: 2023-5-30 如果临时寄存器不够用，借用其他寄存器
*/
unsigned pick_one_free_temp_arm_register()
{
    for(int i=0;i<TEMP_ARM_REG_NUM;i++)
    {
        if(!TempARMRegList[i].isAviliable || TempARMRegList[i].isLimited)continue;
        TempARMRegList[i].isAviliable = false;
        return TempARMRegList[i].reg;
    }
    assert(0 && "No aviliable temp register");
}


/**
 * @brief 如果是额外借用的寄存器，以出栈的方式回收
*/
void recycle_temp_arm_register(int reg)
{
    for(int i=0;i<TEMP_ARM_REG_NUM;i++)
    {
        if(reg == TempARMRegList[i].reg)
        {
            TempARMRegList[i].isAviliable = true;
            return;
        }
            
    }

    assert("Missing the temp regitser");
}


/**
 * @brief 创建一个新的段
*/
void initDlist()
{
    /*
    初始化链表
    */
    head = (assmNode*)malloc(sizeof(assmNode));
    head->next = NULL;
    prev = head;
    //清空nullop
    nullop.addrMode = 0;
    nullop.addtion = 0;
    nullop.oprendVal = 0;
}

//TODO
bool goto_is_conditional(TAC_OP op)
{
    switch(op)
    {
        case Goto_Equal:
        case Goto_NotEqual:
        case Goto_GreatEqual:
        case Goto_GreatThan:
        case Goto_LessEqual:
        case Goto_LessThan:
        case GotoWithConditionOP:
            return true;
        default:
            return false;
    }
}

/**
 * @brief 返回当前寄存器的类型
 * @return 两类 arm寄存器 或vfp浮点寄存器
 * @author Created by LGD on 20230113
*/
ARMorVFP register_type(RegisterOrder reg)
{
    if(reg >= R0 && reg <= SPSR)
        return ARM;
    if(reg >= S0 && reg <= S31)
        return VFP;
}

/**
 * @brief 将数据从寄存器溢出到内存，由于相对寻址范围可能解释成多条指令
 * @birth: Created by LGD on 2023-7-17
 * @update: 2023-8-20 当存在偏移值的时候，完成偏移后溢出到内存
**/
void reg2mem(struct _operand reg,struct _operand mem)
{
    struct _operand finiReg = reg;
    //当前寄存器有偏移，先mov到合理的位置
    if(reg.shiftWay != NONE_SHIFT)
        finiReg = operand_load_to_register(reg, nullop,ARM);
    switch(register_type(reg.oprendVal))
    {
        case ARM:
        {
            //当立即数偏移超出寻址范围时
            if(!check_indirect_offset_valid(mem.addtion) && mem.offsetType == OFFSET_IMMED)
            {
                //将数据传入偏移寄存器
                struct _operand immed = operand_create_immediate_op(mem.addtion,INTERGER_TWOSCOMPLEMENT);
                struct _operand offReg = operand_load_immediate(immed, ARM);
                //构造带偏移的寻址操作数
                struct _operand offMem = operand_create2_relative_adressing(mem.oprendVal, offReg);
                memory_access_instructions("STR",finiReg,offMem,NONESUFFIX,false,NONELABEL);
                //归还寄存器
                operand_recycle_temp_register(offReg);
            }
            else
                memory_access_instructions("STR",finiReg,mem,NONESUFFIX,false,NONELABEL);
        }
        break;
        case VFP:
        {
            vfp_memory_access_instructions("FST",finiReg,mem,FloatTyID);
        }
        break;
    }
    if(!operand_is_same(reg, finiReg))
        operand_recycle_temp_register(finiReg);
}

/**
 * @brief 将数据从内存加载到寄存器，由于相对寻址范围可能解释成多条指令
 * @birth: Created by LGD on 2023-7-17
 * @TODO: 2023-7-22 需要重构
 * @update: 2023-7-29 现在mem2reg支持内存在数据段
**/
void mem2reg(struct _operand reg,struct _operand mem)
{
    //判断源的位置
    if(operand_in_regOrmem(mem) == IN_STACK_SECTION)
    {
        switch(register_type(reg.oprendVal))
        {
            case ARM:
            {
                //当立即数偏移超出寻址范围时
                if(!check_indirect_offset_valid(mem.addtion) && mem.offsetType == OFFSET_IMMED)
                {
                    //创建立即数操作数
                    struct _operand offMemImmed = operand_create_immediate_op(mem.addtion,INTERGER_TWOSCOMPLEMENT);
                    //将超出寻址范围的数装载到临时寄存器
                    struct _operand offMemReg = operand_load_immediate(offMemImmed, ARM);
                    struct _operand offMem = operand_create_relative_adressing(mem.oprendVal,OFFSET_IN_REGISTER,offMemReg.oprendVal);
                    memory_access_instructions("LDR",reg,offMem,NONESUFFIX,false,NONELABEL);
                    //归还寄存器
                    operand_recycle_temp_register(offMemReg);
                }
                else
                    memory_access_instructions("LDR",reg,mem,NONESUFFIX,false,NONELABEL);
            }
            break;
            case VFP:
            {
                vfp_memory_access_instructions("FLD",reg,mem,FloatTyID);
            }
            break;
        }
    }
    assert(operand_in_regOrmem(mem) != IN_DATA_SEC && "暂时不受理IN_DATA_SEC");
}

/**
 * @brief 双目运算 双整型
 * @birth: Created by LGD on 20230226
 * @update: 20230227 添加了对寄存器的回收
 *          2023-3-29 添加在寄存器中变量的直传
*/
 BinaryOperand binaryOpii(AssembleOperand op1,AssembleOperand op2)
 {
    AssembleOperand tarOp;
    AssembleOperand cvtOp1;
    AssembleOperand cvtOp2;
    if(operand_is_in_memory(op1))
        cvtOp1 = operand_load_from_memory(op1,ARM);
    else if(operand_is_immediate(op1))
        cvtOp1 = operand_load_immediate(op1,ARM);
    else
        cvtOp1 = op1;

    if(operand_is_in_memory(op2))
        cvtOp2 = operand_load_from_memory(op2,ARM);
    else if(operand_is_immediate(op2))
        cvtOp2 = operand_load_immediate(op2,ARM);
    else
        cvtOp2 = op2;
    
    BinaryOperand binaryOp = {cvtOp1,cvtOp2};
    return binaryOp;
 }

/**
 * @brief 判断一个指令的操作数是否是浮点数
 * @param opType 可选 TARGET_OPERAND FIRST_OPERAND SECOND_OPERAND FIRST_OPERAND | SECOND_OPERAND
 * @author Created by LGD on 20230113
*/
bool ins_operand_is_float(Instruction* this,int opType)
{
    switch(opType)
    {
        case TARGET_OPERAND:
            return value_is_float(ins_get_operand(this,TARGET_OPERAND));
        case FIRST_OPERAND:
            return value_is_float(ins_get_operand(this,FIRST_OPERAND));
        case SECOND_OPERAND:
            return value_is_float(ins_get_operand(this,SECOND_OPERAND));
        case FIRST_OPERAND | SECOND_OPERAND:
            return value_is_float(ins_get_operand(this,FIRST_OPERAND)) | value_is_float(ins_get_operand(this,SECOND_OPERAND));
    }
}



/**
 * @brief 统一为整数和浮点数变量归还寄存器
 * @update 20230226 用变量的数据类型来区分要分配的寄存器是不合适的
*/
void general_recycle_temp_register(Instruction* this,int i,AssembleOperand op)
{
    //当且仅当该变量存在于内存中，并且当前寻址方式为寄存器直接寻址，使用了临时寄存器
    // Value* var = ins_get_operand(this,i);
    // TypeID type = value_get_type(var);
    // if(get_variable_place_by_order(this,i) == IN_MEMORY && op.addrMode == REGISTER_DIRECT)
    // {
    //    switch(value_get_type(ins_get_operand(this,i)))
    switch(register_type(op.oprendVal))
    {
        case ARM:
            recycle_temp_arm_register(op.oprendVal);
            break;
        case VFP:
            recycle_vfp_register(op.oprendVal);
            break;
    }
    //}
}


/**
 * @brief 回收寄存器的特殊情况，在操作数取自内存或者发生了隐式类型转换
 * @param specificOperand 可选 FIRST_OPERAND SECOND_OPERAND
 * @birth: Created by LGD on 20230227
 * @update: 2023-3-28 重构了回收寄存器条件
*/
void general_recycle_temp_register_conditional(Instruction* this,int specificOperand,AssembleOperand recycleRegister)
{

    size_t recycle_status = NO_NEED_TO_RECYCLE;
    if(variable_is_in_memory(this,ins_get_operand(this,specificOperand)))
        recycle_status |= VARIABLE_IN_MEMORY;

     if(value_is_global(ins_get_operand(this,specificOperand)))
        recycle_status |= VARIABLE_IN_MEMORY;
    
    if(variable_is_in_instruction(this,ins_get_operand(this,specificOperand)))
        recycle_status |= VARIABLE_LDR_FROM_IMMEDIATE;

    //增加确保操作数大于等于2的判断条件  否则GOTO等归还发生报错
    if(ins_get_operand_num(this) >= 2)
        if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND) && !ins_operand_is_float(this,specificOperand))
            recycle_status |= INTERGER_PART_IN_MIX_CALCULATE;

    //2023-5-3 增加两个寄存器不一致也要归还的逻辑
    if(variable_is_in_register(this,ins_get_operand(this,specificOperand)))
        if(!operand_is_same(toOperand(this,specificOperand),recycleRegister))
            recycle_status |= REGISTER_ATTRIBUTED_DIFFER_FROM_VARIABLE_REGISTER;

    if((recycle_status | NO_NEED_TO_RECYCLE) != NO_NEED_TO_RECYCLE)
        general_recycle_temp_register(this,specificOperand,recycleRegister);
        
}

//----------------------------------------//
//            新的方法                     //

/***
 * @brief tar:int = op:float
 * @birth: Created by LGD on 20230130
*/
void movif(AssembleOperand tar,AssembleOperand op1)
{
    //确保源在VFP
    struct _operand srcOp1 = op1;
    op1 = operandConvert(srcOp1, VFP, 0, 0);
    //如果目标在寄存器
    if(operand_is_in_register(tar))
    {
        //如果源在寄存器
        if(operand_is_in_register(op1))
            operand_regFloat2Int(op1,tar);
        //源在内存
        else
        {   
            struct _operand temp;
            temp = operand_load_to_register(op1, nullop,VFP);
            operand_regFloat2Int(temp,tar);
            operand_recycle_temp_register(temp);        
        }
    } 
    //目标在内存
    else if(operand_is_in_memory(tar))
    {
        struct _operand temp;
        //如果源在寄存器
        if(operand_is_in_register(op1))
        {
            temp = operand_pick_temp_register(VFP);
            operand_regFloat2Int(op1,temp);
            reg2mem(temp,tar);
        }
        //如果源在内存
        else if(operand_is_in_memory(op1))
        {
            temp = operand_load_to_register(op1,nullop,VFP);
            operand_regFloat2Int(temp,temp);
            reg2mem(temp,tar);
        }
        operand_recycle_temp_register(temp);       
    }
    if(!operand_is_same(op1, srcOp1))
        operand_recycle_temp_register(op1);
}

/***
 * @brief tar:float = op:int
 * @birth: Created by LGD on 20230130
*/
void movfi(AssembleOperand tar,AssembleOperand op1)
{
    //确保源在VFP
    struct _operand srcOp1 = op1;
    op1 = operandConvert(srcOp1, VFP, 0, 0);
    //如果目标在寄存器
    if(operand_is_in_register(tar))
    {
        //如果源在寄存器
        if(operand_is_in_register(op1))
            operand_regInt2Float(op1,tar);
        //源在内存
        else
        {   
            struct _operand temp;
            temp = operand_load_to_register(op1, nullop,VFP);
            operand_regInt2Float(temp,tar);
            operand_recycle_temp_register(temp);        
        }
    } 
    //目标在内存
    else if(operand_is_in_memory(tar))
    {
        struct _operand temp;
        //如果源在寄存器
        if(operand_is_in_register(op1))
        {
            //申请临时目标寄存器
            temp = operand_pick_temp_register(VFP);
            operand_regInt2Float(op1,temp);
            reg2mem(temp,tar);
        }
        //如果源在内存
        else if(operand_is_in_memory(op1))
        {
            temp = operand_load_to_register(op1, nullop,VFP);
            operand_regInt2Float(temp,temp);
            reg2mem(temp,tar);
        }
        operand_recycle_temp_register(temp);       
    }
    if(!operand_is_same(op1, srcOp1))
        operand_recycle_temp_register(op1);
}

/**
 * @brief movff
 * @birth: Created by LGD on 20230201
 * @todo mem = reg 还可以优化一个语句
 * @update: 2023-4-11 目标数为寄存器时的直接位移
 *          2023-7-18 重构
 *          2023-7-29 如果源在寄存器，则没必要再次申请临时寄存器
*/
void movff(AssembleOperand tar,AssembleOperand op1)
{
    AssembleOperand original_op1 = op1;

    //如果target为寄存器
    if(operand_is_in_register(tar))
        operand_load_to_register(op1, tar);
    //如果target在内存
    else
    {
        struct _operand temp = operandConvert(op1, VFP, 0, IN_STACK_SECTION | IN_INSTRUCTION);
        reg2mem(temp,tar);
        if(!operand_is_same(temp, op1))
            operand_recycle_temp_register(temp);
    }
}

/**
 * @brief movii
 * @birth: Created by LGD on 20230201
 * @update: 2023-4-11 优化了立即数传入寄存器
 *          2023-7-17 当检测两个操作数位置一致时，不作处理
 *          2023-7-18 重构
 *          2023-7-29 如果源在寄存器，则没必要再次申请临时寄存器
*/
void movii(AssembleOperand tar,AssembleOperand op1)
{
    //如果tar为寄存器
    if(operand_is_in_register(tar))
        operand_load_to_register(op1, tar);
    //如果tar在内存
    else
    {
        struct _operand temp = operandConvert(op1, ARM, 0, IN_STACK_SECTION | IN_INSTRUCTION);
        reg2mem(temp,tar);
        if(!operand_is_same(temp, op1))
            operand_recycle_temp_register(temp);
    }
}

/**
 * @brief movini
 * @birth: Created by LGD on 2023-7-11
 * @update: 修复取反操作在存在内存形式时的错误
 *          2023-7-29 修复从内存取反没有实现的bug
*/
void movini(AssembleOperand tar,AssembleOperand op1)
{
    AssembleOperand oriOp1 = op1;
    //如果tar为寄存器
    if(operand_is_in_register(tar))
    {
        //取出内存后取反
        if(operand_is_in_memory(op1))
        {
            op1 = operand_load_from_memory(op1,ARM);
            general_data_processing_instructions(MVN,tar,nullop,op1,NONESUFFIX,false);
            if(!operand_is_same(oriOp1, op1))
                operand_recycle_temp_register(op1);
        }
        //立即数直接取反
        else if(operand_is_immediate(op1))
        {
            //文字池取出后取反
            //不太可能
            assert(operand_in_regOrmem(op1) != IN_LITERAL_POOL && "把地址取反一般是毫无意义的");
            op1.oprendVal = - op1.oprendVal;
            operand_load_immediate_to_specified_register(op1,tar);
        }
        //如果在寄存器
        else if(operand_is_in_register(op1))
        {
            general_data_processing_instructions(MVN,tar,nullop,op1,NONESUFFIX,false);
        }
    }
    else
    {
        if(operand_is_in_memory(op1))
        {
            op1 = operand_load_from_memory(op1,ARM);
            general_data_processing_instructions(MVN,op1,nullop,op1,NONESUFFIX,false);
        }
        else if(operand_is_immediate(op1))
        {
            assert(operand_in_regOrmem(op1) != IN_LITERAL_POOL && "把地址取反一般是毫无意义的");
            op1.oprendVal = -op1.oprendVal;
            op1 = operand_load_immediate(op1,ARM);
        }

        reg2mem(op1,tar);

        if(operand_is_in_memory(oriOp1) || operand_is_immediate(oriOp1))
            operand_recycle_temp_register(op1);
    }
}

/**
 * @brief 数据与数据之间的传递，包含隐式转换，支持所有内存、立即数和寄存器类型
 * @birth: Created by LGD on 2023-7-23
*/
void mov(struct _operand tar,struct _operand src)
{
    if(operand_get_format(src) == INTERGER_TWOSCOMPLEMENT && operand_get_format(tar) == INTERGER_TWOSCOMPLEMENT)
        movii(tar,src);
    else if(operand_get_format(src) == IEEE754_32BITS && operand_get_format(tar) == IEEE754_32BITS)
        movff(tar,src);
    else if((operand_get_format(src) == INTERGER_TWOSCOMPLEMENT) && (operand_get_format(tar) == IEEE754_32BITS))
        movfi(tar,src);
    else if((operand_get_format(src) == IEEE754_32BITS) && (operand_get_format(tar) == INTERGER_TWOSCOMPLEMENT))
        movif(tar,src);
    else
        assert(false && "unrecognize format convert");
}

/**
 * @brief cmpii
 * @birth: Created by LGD on 2023-4-4
 * @todo 更改回收寄存器的方式
 * @update: 2023-5-29 考虑了目标操作数是立即数的情况
 *          2023-7-23 重构代码
*/
void cmpii(AssembleOperand op1,AssembleOperand op2)
{
    AssembleOperand srcOp1 = op1;
    AssembleOperand srcOp2 = op2;

    if(!operand_is_in_register(op1))
        op1 = operand_load_to_register(srcOp1, nullop, ARM);

    if(!operand_is_in_register(op2))
        op2 = operand_load_to_register(srcOp2, nullop, ARM);
    
    general_data_processing_instructions(CMP,op1,nullop,op2,NONESUFFIX,false);

    if(!operand_is_same(srcOp1, op1))
        operand_recycle_temp_register(op1);

    if(!operand_is_same(srcOp2,op2))
        operand_recycle_temp_register(op2);

    //不涉及回写
}

/**
 * @brief cmpff 比较两个浮点数
 * @birth: Created by LGD on 2023-7-22
*/
void cmpff(struct _operand op1,struct _operand op2)
{
    struct _operand srcOp1 = op1;
    struct _operand srcOp2 = op2;
    //确保两数在浮点寄存器
    op1 = operandConvert(srcOp1,VFP,0,0);
    op2 = operandConvert(srcOp2,VFP,0,0);
    //进行比较
    fcmp_instruction(op1,op2,FloatTyID);
    //释放可能的临时寄存器
    if(!operand_is_same(op1, srcOp1))
        operand_recycle_temp_register(op1);
    if(!operand_is_same(op2, srcOp2))
        operand_recycle_temp_register(op2);

    //由于手册提到，浮点FCMP指令执行完后需要通过FMSTAT指令将FPSCR拷贝到CPSR作为条件码的译码依据
    fmstat();
    
}

/**
 * @brief 取两个数相与的结果
 * @birth: Created by LGD on 2023-7-16
**/
void andiii(AssembleOperand tar,AssembleOperand op1,AssembleOperand op2)
{
    AssembleOperand middleOp;
    BinaryOperand binaryOp;
    
    binaryOp = binaryOpii(op1,op2);
    
    middleOp = operand_pick_temp_register(ARM);
    general_data_processing_instructions(AND,
        middleOp,binaryOp.op1,binaryOp.op2,NONESUFFIX,false);
    
    movii(tar,middleOp);

    if(!operand_is_same(op1,binaryOp.op1))
        operand_recycle_temp_register(binaryOp.op1);
    
    if(!operand_is_same(op2,binaryOp.op2))
        operand_recycle_temp_register(binaryOp.op2);

    //释放中间操作数
    operand_recycle_temp_register(middleOp);
}

/**
 * @brief 取两个数相或的结果
 * @birth: Created by LGD on 2023-7-16
**/
void oriii(AssembleOperand tar,AssembleOperand op1,AssembleOperand op2)
{
    AssembleOperand middleOp;
    BinaryOperand binaryOp;
    
    binaryOp = binaryOpii(op1,op2);
    
    middleOp = operand_pick_temp_register(ARM);
    general_data_processing_instructions(ORR,
        middleOp,binaryOp.op1,binaryOp.op2,NONESUFFIX,false);
    
    movii(tar,middleOp);

    if(!operand_is_same(op1,binaryOp.op1))
        operand_recycle_temp_register(binaryOp.op1);
    
    if(!operand_is_same(op2,binaryOp.op2))
        operand_recycle_temp_register(binaryOp.op2);

    //释放中间操作数
    operand_recycle_temp_register(middleOp);
}

/**
 * @brief movCondition
 * @birth: Created by LGD on 2023-2-1
 * @update: 2023-5-29 考虑了布尔变量在内存中的情况
*/
void movCondition(AssembleOperand tar,AssembleOperand op1,enum _Suffix cond)
{
    //如果tar为寄存器
    struct _operand original_op1 = op1;
    if(operand_is_in_register(tar))
    {
        general_data_processing_instructions(MOV,tar,nullop,op1,cond2Str(cond),false);
    }
    else
    {   
        op1 = operand_load_to_register(original_op1, nullop,ARM);
        //TODO
        memory_access_instructions("STR",op1,tar,cond2Str(cond),false,NONELABEL);
        if(!operand_is_same(op1,original_op1))
            operand_recycle_temp_register(op1);
    }
}



/**
 * @brief:完成一次整数的相加
 * @birth:Created by LGD on 2023-5-29
 * @update:2023-7-18 消去一条无用的VFP寄存器分配指令
 *         2023-7-21 添加对超过相对寻址范围的检查
 *         2023-7-21 完全重构
*/
void addiii(struct _operand tarOp,struct _operand op1,struct _operand op2)
{
    AssembleOperand middleOp;

    struct _operand srcOp1 = op1;
    struct _operand srcOp2 = op2;

    if(!operand_is_in_register(srcOp1))
        op1 = operand_load_to_register(srcOp1, nullop, ARM);
    if(!operand_is_in_register(srcOp2))
        op2 = operand_load_to_register(srcOp2, nullop, ARM);
    

    middleOp = operand_pick_temp_register(ARM);
    middleOp.format = INTERGER_TWOSCOMPLEMENT;
    general_data_processing_instructions(ADD,
        middleOp,op1,op2,NONESUFFIX,false);
            
    
    //释放第一、二操作数
    if(!operand_is_same(op1, srcOp1))
        operand_recycle_temp_register(op1);
    if(!operand_is_same(op2, srcOp2))
        operand_recycle_temp_register(op2);

    movii(tarOp,middleOp);

    //释放中间操作数
    operand_recycle_temp_register(middleOp);
}

/**
 * @brief 完成一次整数相减，并把运算结果返回到一个临时的寄存器中
 * @birth: Created by LGD on 2023-7-23
*/
struct _operand subii(struct _operand op1,struct _operand op2)
{
    struct _operand oriOp1,oriOp2;
    oriOp1 = op1;
    oriOp2 = op2;
    AssembleOperand middleOp;
    middleOp = operand_pick_temp_register(ARM);
    middleOp.format = INTERGER_TWOSCOMPLEMENT;
#ifndef ALLOW_TWO_IMMEDIATE
        assert(!(opernad_is_immediate(op1) && opernad_is_immediate(op2)) && "减法中两个操作数都是立即数是不允许的");
#endif
    //如果第1个操作数为立即数，第2个不是，使用RSB指令
    if(operand_is_immediate(op1) && !operand_is_immediate(op2))
    {
        op2 = operandConvert(op2,ARM,false,IN_STACK_SECTION);
        general_data_processing_instructions(RSB,
            middleOp,op2,op1,NONESUFFIX,false);
    }
    //第1个操作数不是立即数，第2个操作数可以是任何数，使用SUB指令
    else{
        op1 = operandConvert(op1,ARM,false,IN_STACK_SECTION | IN_INSTRUCTION);
        op2 = operandConvert(op2,ARM,false,IN_STACK_SECTION | IN_INSTRUCTION);
        general_data_processing_instructions(SUB,
            middleOp,op1,op2,NONESUFFIX,false);
    }

    //释放第一、二操作数
    if(!operand_is_same(op1, oriOp1))operand_recycle_temp_register(op1);
    if(!operand_is_same(op2, oriOp2))operand_recycle_temp_register(op2);

    return middleOp;
}

/**
 * @brief 完成一次整数的相减
 * @birth: Created by LGD on 2023-7-22
*/
void subiii(struct _operand tarOp,struct _operand op1,struct _operand op2)
{   
    AssembleOperand middleOp;
    middleOp = subii(op1,op2);
    //中间量传给目标变量
    movii(tarOp,middleOp);
    //释放中间操作数
    operand_recycle_temp_register(middleOp);
}

/**
 * @brief 完成一次浮点相减，并把运算结果返回到一个生成的寄存器中
 * @birth: Created by LGD on 2023-7-23
*/
struct _operand subff(struct _operand op1,struct _operand op2)
{
    struct _operand srcOp1 = op1;
    struct _operand srcOp2 = op2;
    struct _operand middleOp;
    //寄存器 内存 立即数 都转换
    op1 = operandConvert(srcOp1,VFP,0,0);
    op2 = operandConvert(srcOp2,VFP,0,0);
    middleOp = operand_pick_temp_register(VFP);
    middleOp.format = IEEE754_32BITS;
    fadd_and_fsub_instruction("FSUB",
            middleOp,op1,op2,FloatTyID);

    if(!operand_is_same(op1, srcOp1))
        operand_recycle_temp_register(op1);
    if(!operand_is_same(op2, srcOp2))
        operand_recycle_temp_register(op2);
    return middleOp;
}

/**
 * @brief 实现三个IEEE754浮点数的减法，寄存器不需要VFP，不提供格式转换
 * @update: Created by LGD on 2023-7-23
*/
void subfff(struct _operand tarOp,struct _operand op1,struct _operand op2)
{
    struct _operand middleOp;
    middleOp = subff(op1,op2);
    movff(tarOp,middleOp);
    operand_recycle_temp_register(middleOp);
}

/**
 * @brief 为代码添加文字池以避免文字池太远的问题
 * @birth: Created by LGD on 2023-7-27
*/
void add_interal_pool()
{
    size_t cnt = 0;
    size_t idx = 0;
    char poolName[32];
    for (prev = head->next; prev != NULL; prev = prev->next) {
        ++cnt;
        ++idx;
        if(cnt >= 500)
        {
            sprintf(poolName,"poolEnd%lu",idx);
            branch_instructions(poolName, NONESUFFIX, false,NONELABEL);
            pool();
            Label(poolName);
            cnt = 0;
        }
    }
}