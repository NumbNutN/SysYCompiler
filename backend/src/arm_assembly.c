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

TempReg TempARMRegList[TEMP_REG_NUM];
TempReg TempVFPRegList[TEMP_REG_NUM];
TempReg AddtionalARMRegList[ADDITION_REG_NUM];



Stack* Free_Vps_Register_list;

/**
 * @brief 浮点空闲寄存器获取三件套  这是选取临时浮点寄存器保存操作数
*/
void Free_Vps_Register_Init()
{
    for(int i=0;i<TEMP_REG_NUM;i++)
    {
        TempVFPRegList[i].reg = S1+i;
        TempVFPRegList[i].isAviliable = true;
    }
}

RegisterOrder pick_one_free_vfp_register()
{   
    for(int i=0;i<TEMP_REG_NUM;i++)
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
    for(int i=0;i<TEMP_REG_NUM;i++)
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

    for(int i=0;i<TEMP_REG_NUM;i++)
    {
        TempARMRegList[i].reg = R0+i;
        TempARMRegList[i].isAviliable = true;
    }
}

/**
 * @brief 判断当前寄存器是否是限制级别寄存器
 * @birth: Created by LGD on 2023-5-4
*/
bool Is_limited_temp_register(RegisterOrder reg)
{
    enum _Pick_Arm_Register_Limited result = NONE_LIMITED;
    if((global_arm_register_limited & RETURN_VALUE_LIMITED) == RETURN_VALUE_LIMITED)
        result |= (reg == R0);
    if((global_arm_register_limited & PARAMETER1_LIMITED) == PARAMETER1_LIMITED)
        result |= (reg == R0);
    if((global_arm_register_limited & PARAMETER2_LIMITED) == PARAMETER2_LIMITED)
        result |= (reg == R1);
    if((global_arm_register_limited & PARAMETER3_LIMITED) == PARAMETER3_LIMITED)
        result |= (reg == R2);
    if((global_arm_register_limited & PARAMETER4_LIMITED) == PARAMETER4_LIMITED)
        result |= (reg == R3);
    return (result != NONE_LIMITED);
}

/**
 * @brief 添加新的限制级别
 * @birth: Created by LGD on 2023-5-4
*/
void add_register_limited(enum _Pick_Arm_Register_Limited limited)
{
    global_arm_register_limited |= limited;
}

/**
 * @brief 移除一个限制级别，如果其本身没有这个限制级别，将忽略
 * @birth: Created by LGD on 2023-5-4
*/
void remove_register_limited(enum _Pick_Arm_Register_Limited limited)
{
    global_arm_register_limited &= ~limited;
}

/**
 * @brief 挑选一个闲置的临时寄存器 这个方法作为底层调用被相当多的方法依赖
 * @update: 2023-5-4 一个全局指示变量将提示现在不应当选择哪些寄存器作为临时寄存器
 * @update: 2023-5-30 如果临时寄存器不够用，借用其他寄存器
*/
unsigned pick_one_free_temp_arm_register()
{
    for(int i=0;i<TEMP_REG_NUM;i++)
    {
        if(!TempARMRegList[i].isAviliable || Is_limited_temp_register(TempARMRegList[i].reg))continue;
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
    for(int i=0;i<TEMP_REG_NUM;i++)
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
**/
void reg2mem(struct _operand reg,struct _operand mem)
{
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
                memory_access_instructions("STR",reg,offMem,NONESUFFIX,false,NONELABEL);
                //归还寄存器
                operand_recycle_temp_register(offReg);
            }
            else
                memory_access_instructions("STR",reg,mem,NONESUFFIX,false,NONELABEL);
        }
        break;
        case VFP:
        {
            vfp_memory_access_instructions("FLDR",reg,mem,FloatTyID);
        }
        break;
    }
}

/**
 * @brief 将数据从内存加载到寄存器，由于相对寻址范围可能解释成多条指令
 * @birth: Created by LGD on 2023-7-17
 * @TODO: 2023-7-22 需要重构
**/
void mem2reg(struct _operand reg,struct _operand mem)
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
            vfp_memory_access_instructions("FSTR",reg,mem,FloatTyID);
        }
        break;
    }
    
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
    if(operand_in_regOrmem(op1) == IN_MEMORY)
        cvtOp1 = operand_load_from_memory(op1,ARM);
    else if(operand_in_regOrmem(op1) == IN_INSTRUCTION)
        cvtOp1 = operand_load_immediate(op1,ARM);
    else
        cvtOp1 = op1;

    if(operand_in_regOrmem(op2) == IN_MEMORY)
        cvtOp2 = operand_load_from_memory(op2,ARM);
    else if(operand_in_regOrmem(op2) == IN_INSTRUCTION)
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

/**
 * @brief 通过中间代码Value的描述确定operand的format
 * @birth: Created by LGD on 20230226
*/
enum _DataFormat valueFindFormat(Value* var)
{
    if(value_is_float(var))
    {
        return IEEE754_32BITS;
    }
    else
    {
        return INTERGER_TWOSCOMPLEMENT;
    }
}

/***
 * @brief tar:int = op:float
 * @birth: Created by LGD on 20230130
*/
void movif(AssembleOperand tar,AssembleOperand op1)
{
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
            temp = operand_pick_temp_register(VFP);
            operand_regInt2Float(op1,temp);
            reg2mem(temp,tar);
        }
        //如果源在内存
        else if(operand_is_in_memory(op1))
        {
            temp = operand_load_to_register(op1,nullop,VFP);
            operand_regInt2Float(temp,temp);
            reg2mem(temp,tar);
        }
        operand_recycle_temp_register(temp);       
    }
}

/***
 * @brief tar:float = op:int
 * @birth: Created by LGD on 20230130
*/
void movfi(AssembleOperand tar,AssembleOperand op1)
{
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
            temp = operand_load_to_register(op1, nullop,VFP);
            operand_regFloat2Int(temp,temp);
            reg2mem(temp,tar);
        }
        operand_recycle_temp_register(temp);       
    }
}

/**
 * @brief movff
 * @birth: Created by LGD on 20230201
 * @todo mem = reg 还可以优化一个语句
 * @update: 2023-4-11 目标数为寄存器时的直接位移
 *          2023-7-18 重构
*/
void movff(AssembleOperand tar,AssembleOperand op1)
{
    AssembleOperand original_op1 = op1;

    //如果target为寄存器
    if(operand_in_regOrmem(tar) == IN_REGISTER)
    {
        operand_load_to_register(op1, tar);
    }
    //如果target在内存
    else
    {
        struct _operand temp = operand_load_to_register(op1, nullop,VFP);
        reg2mem(temp,tar);
        operand_recycle_temp_register(temp);
    }
}

/**
 * @brief movii
 * @birth: Created by LGD on 20230201
 * @update: 2023-4-11 优化了立即数传入寄存器
 *          2023-7-17 当检测两个操作数位置一致时，不作处理
 *          2023-7-18 重构
*/
void movii(AssembleOperand tar,AssembleOperand op1)
{
    //如果tar为寄存器
    if(operand_in_regOrmem(tar) == IN_REGISTER)
        operand_load_to_register(op1, tar);
    //如果tar在内存
    else
    {
        struct _operand temp = operand_load_to_register(op1, nullop,ARM);
        reg2mem(temp,tar);
        operand_recycle_temp_register(temp);
    }
}

/**
 * @brief movini
 * @birth: Created by LGD on 2023-7-11
 * @update: 修复取反操作在存在内存形式时的错误
*/
void movini(AssembleOperand tar,AssembleOperand op1)
{
    AssembleOperand oriOp1 = op1;
    //如果tar为寄存器
    if(operand_in_regOrmem(tar) == IN_REGISTER)
    {
        //取出内存后取反
        if(operand_in_regOrmem(op1) == IN_MEMORY)
        {
            op1 = operand_load_from_memory(op1,ARM);
        }
        //立即数直接取反
        else if(operand_in_regOrmem(op1) == IN_INSTRUCTION)
        {
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
        if(operand_in_regOrmem(op1) == IN_MEMORY)
        {
            op1 = operand_load_from_memory(op1,ARM);
            general_data_processing_instructions(MVN,op1,nullop,op1,NONESUFFIX,false);
        }
        else if(operand_in_regOrmem(op1) == IN_INSTRUCTION)
        {
            op1.oprendVal = -op1.oprendVal;
            op1 = operand_load_immediate(op1,ARM);
        }

        reg2mem(op1,tar);

        if(operand_in_regOrmem(oriOp1) == IN_MEMORY ||
        (operand_in_regOrmem(oriOp1) == IN_INSTRUCTION))
            operand_recycle_temp_register(op1);
    }
}

/**
 * @brief 数据与数据之间的传递，包含隐式转换，支持所有内存、立即数和寄存器类型
 * @birth: Created by LGD on 2023-7-23
*/
void mov(struct _operand tar,struct _operand src)
{
    if(operand_get_format(src) == operand_get_format(tar) == INTERGER_TWOSCOMPLEMENT)
        movii(tar,src);
    else if(operand_get_format(src) == operand_get_format(tar) == IEEE754_32BITS)
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
    if(operand_in_regOrmem(tar) == IN_REGISTER)
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
 * @brief 双目运算 整数浮点混合双目运算
 * @birth: Created by LGD on 20230226
 * @update: 20230227 添加了对寄存器的回收
*/
 BinaryOperand binaryOpfi(AssembleOperand op1,AssembleOperand op2)
 {
    AssembleOperand tarOp;
    AssembleOperand cvtOp1;
    AssembleOperand cvtOp2;
    if(operand_in_regOrmem(op1) == IN_MEMORY)
        cvtOp1 = operand_load_from_memory(op1,VFP);
    else if(operand_in_regOrmem(op1) == IN_INSTRUCTION)
        cvtOp1 = operand_load_immediate(op1,VFP);
    else
    {
        switch(register_type(cvtOp1.oprendVal))
        {
            case ARM:
                cvtOp1 = operand_float_deliver(op1,false);
            break;
            case VFP:
                cvtOp1 = op1;
            break;
        }
    } 
    if(operand_in_regOrmem(op2) == IN_MEMORY)
        cvtOp2 = operand_load_from_memory(op2,VFP);
    else if(operand_in_regOrmem(op2) == IN_INSTRUCTION)
        cvtOp2 = operand_load_immediate(op2,VFP);
    else
    {
        switch(register_type(cvtOp2.oprendVal))
        {
            case ARM:
                cvtOp2 = operand_float_deliver(op2,false);
            break;
            case VFP:
                cvtOp2 = op2;
            break;
        }
    }
    BinaryOperand binaryOp = {cvtOp1,cvtOp2};
    return binaryOp;
 }

/**
 * @brief 双目运算 浮点运算
 * @birth: Created by LGD on 20230226
*/
BinaryOperand binaryOpff(AssembleOperand op1,AssembleOperand op2)
{
    if(operand_in_regOrmem(op1) == IN_MEMORY)
        op1 = operand_load_from_memory(op1,VFP);
    else if(operand_in_regOrmem(op1) == IN_INSTRUCTION)
        op1 = operand_load_immediate(op1,VFP);

    if(operand_in_regOrmem(op2) == IN_MEMORY)
        op2 = operand_load_from_memory(op2,VFP);
    else if(operand_in_regOrmem(op2) == IN_INSTRUCTION)
        op2 = operand_load_immediate(op2,VFP);

    BinaryOperand binaryOp = {op1,op2};
    return binaryOp;
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
    if(opernad_is_immediate(op1) && !opernad_is_immediate(op2))
    {
        op2 = operandConvert(op2,ARM,false,IN_MEMORY);
        general_data_processing_instructions(RSB,
            middleOp,op2,op1,NONESUFFIX,false);
    }
    //第1个操作数不是立即数，第2个操作数可以是任何数，使用SUB指令
    else{
        op1 = operandConvert(op1,ARM,false,IN_MEMORY | IN_INSTRUCTION);
        op2 = operandConvert(op2,ARM,false,IN_MEMORY | IN_INSTRUCTION);
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
 * @brief 这个方法专用于加载目标操作数
 *        规则如下：
 *          左侧变量在内存中时：
 *              挑选一个与结果类型匹配的寄存器
 *          左侧变量在寄存器中时：
 *              若双目运算结果与左侧变量类型不同，挑选一个与结果类型匹配的寄存器
 *              若双目运算结果与左侧变量类型相同，则直传寄存器
 * @author created by LGD on 20230113
 * @update: 20230122 添加了operandType来区分赋值语句和算术语句的操作数个数
*/
void ins_target_operand_load_in_register(Instruction* this,AssembleOperand* op,int operandType)
{

    RegorMem rom = get_variable_place_by_order(this,0);
    int offset;
    RegisterOrder tempReg;
    switch(rom)
    {
        case IN_MEMORY:

            if(ins_operand_is_float(this,operandType))
            {
                op->addrMode = REGISTER_DIRECT;
                op->oprendVal = pick_one_free_vfp_register();
            }
            else
            {
                op->addrMode = REGISTER_DIRECT;
                op->oprendVal = pick_one_free_temp_arm_register();
            }
            break;
        case IN_INSTRUCTION:
            assert(0 && "TARGET_VARIABLE CANNOT BE IN INSTRUCTION");
            break;
        case IN_REGISTER:
            //在寄存器中
            if((ins_operand_is_float(this,operandType) && value_get_type(ins_get_operand(this,0)) == FloatTyID) ||
                (!ins_operand_is_float(this,operandType) && value_get_type(ins_get_operand(this,0)) == IntegerTyID))
            {
                //结果与目标变量寄存器类型相同 直传即可
                op->addrMode = REGISTER_DIRECT;
                op->oprendVal = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,TARGET_OPERAND));
            }
            else 
            {
                //结果与目标变量寄存器类型不同 给出一个与结果类型相同的寄存器
                op->addrMode = REGISTER_DIRECT;
                if(ins_operand_is_float(this,operandType))
                    op->oprendVal = pick_one_free_vfp_register();
                else
                    op->oprendVal = pick_one_free_temp_arm_register();
            }
            break;
    }
}

/**
 * @brief 这个方法分析指令的第i个操作数，若变量在内存中，产生一个访存指令后返回临时寄存器,若在寄存器和指令中，返回对应的寄存器或常数
 * @author created by LGD on 20221225
 * @return 若使用了临时寄存器，返回寄存器的编号，否则返回-1
*/
size_t variable_load_in_register(Instruction* this,Value* op,AssembleOperand* assem_op)
{

    RegorMem rom = get_variable_place(this,op);
    bool use_temp_reg;
    int offset;
    RegisterOrder tempReg;
    switch(rom)
    {
        case IN_MEMORY:
            /* 第1/2操作数在内存时，先取到临时寄存器 */
            
            offset = get_variable_register_order_or_memory_offset_test(this,op);
            switch(value_get_type(op))
            {
                case IntegerTyID:
                    tempReg = pick_one_free_temp_arm_register();
                case FloatTyID:
                    tempReg = pick_one_free_vfp_register();
            }
            
            AssembleOperand accessOpInMem[2];
            accessOpInMem[0].oprendVal = tempReg;
            accessOpInMem[0].addrMode = REGISTER_DIRECT;

            accessOpInMem[1].oprendVal = SP;
            accessOpInMem[1].addrMode = REGISTER_INDIRECT_WITH_OFFSET;
            accessOpInMem[1].addtion = offset;
            switch(value_get_type(op))
            {
                case IntegerTyID:
                    memory_access_instructions("LDR",accessOpInMem[0],accessOpInMem[1],NONESUFFIX,false,NONELABEL);
                    break;
                case FloatTyID:
                    vfp_memory_access_instructions("FLDR",accessOpInMem[0],accessOpInMem[1],FloatTyID);
                    break;
            }     
            
            assem_op->addrMode = REGISTER_DIRECT;
            assem_op->oprendVal = tempReg;
            use_temp_reg = true;
            break;
        case IN_INSTRUCTION:
            //在指令中
            assem_op->addrMode = IMMEDIATE;
            //将立即数（应该在value->pdata中）
            assem_op->oprendVal = op_get_constant(op);

            use_temp_reg = false;
            break;
        default:
            //在寄存器中
            assem_op->addrMode = REGISTER_DIRECT;
            //将寄存器编号赋给operandVal
            assem_op->oprendVal = get_variable_register_order_or_memory_offset_test(this,op);

            use_temp_reg = true;
            break;
    }

    if(use_temp_reg)
        return assem_op->oprendVal;
    else
        return -1;
}

void variable_storage_back(Instruction* this,int i,int order)
{
    // @brief:这个方法分析指令的第i个操作数，若原先在内存中，将其放回内存中
    //        若在寄存器中，则不管
    // @birth:Created by LGD on 20221225
    AssembleOperand accessOpInMem[2];

    RegorMem rom = get_variable_place_by_order(this,i);
    if(rom == IN_MEMORY)
    {
        accessOpInMem[0].oprendVal = order;
        accessOpInMem[0].addrMode = REGISTER_DIRECT;
        accessOpInMem[1].oprendVal = SP;
        accessOpInMem[1].addrMode = REGISTER_INDIRECT_WITH_OFFSET;
        accessOpInMem[1].addtion = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,0));
        switch(value_get_type(ins_get_operand(this,i)))
        {
            case IntegerTyID:
                memory_access_instructions("STR",accessOpInMem[0],accessOpInMem[1],NONESUFFIX,false,NONELABEL);
                break;
            case FloatTyID:
                vfp_memory_access_instructions("FLDR",accessOpInMem[0],accessOpInMem[1],FloatTyID);
                break;
        }
        
    }

}

/**
 * @brief 将变量存储回原来的位置
 *          若在内存中，将最终结果存到内存
 *          若在寄存器，将最终结果存在寄存器，若本身在指定寄存器则忽略
 *          若类型不符，则将类型转正确后再存
 * @author Created by LGD on 20230113
*/
void variable_storage_back_new(Instruction* this,int i,RegisterOrder order)
{
    
    switch(get_variable_place_by_order(this,i))
    {
        //变量最终存放在内存中
        case IN_MEMORY:
            if(ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == VFP)
            {
                //变量为浮点数 并且 已存放在浮点寄存器中
                //float = float
                //说明此时目标数已是IEEE754形式，直接存放到内存即可
                AssembleOperand mem;
                AssembleOperand reg;
                mem.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
                mem.oprendVal = SP;
                mem.addtion = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));
                reg.addrMode = REGISTER_DIRECT;
                reg.oprendVal = order;
                vfp_memory_access_instructions("FST",reg,mem,FloatTyID);

                general_recycle_temp_register_conditional(this,TARGET_OPERAND,reg);

            }
            else if(ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == ARM)
            {
                //变量为浮点数 但是 存放在普通寄存器
                //float = int
                //此时转换成IEEE754形式再存放到内存
                AssembleOperand op_bvt;
                op_bvt.addrMode = REGISTER_DIRECT;
                op_bvt.oprendVal = pick_one_free_vfp_register();

                AssembleOperand op_in_arm_reg;
                op_in_arm_reg.addrMode = REGISTER_DIRECT;
                op_in_arm_reg.oprendVal = order;
                fmrs_and_fmsr_instruction("FMSR",op_in_arm_reg,op_bvt,FloatTyID);

                //回收临时arm寄存器
                recycle_temp_arm_register(op_in_arm_reg.oprendVal);

                AssembleOperand op_avt;
                op_avt.addrMode = REGISTER_DIRECT;
                op_avt.oprendVal = pick_one_free_vfp_register();

                //整型转为浮点型
                fsito_and_fuito_instruction("FSITO",op_avt,op_bvt,FloatTyID);
                //回收临时vfp寄存器
                recycle_vfp_register(op_bvt.oprendVal);

                AssembleOperand mem;
                mem.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
                mem.oprendVal = SP;
                mem.addtion = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));

                vfp_memory_access_instructions("FST",op_avt,mem,FloatTyID);

                general_recycle_temp_register_conditional(this,TARGET_OPERAND,op_avt);
            }
            else if(!ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == VFP)
            {
                //变量为整型 但是 存放在浮点寄存器
                //int = float
                //此时转换成补码整型再存放到内存
                AssembleOperand op_bvt;
                op_bvt.addrMode = REGISTER_DIRECT;
                op_bvt.oprendVal = order;
                AssembleOperand op_avt;
                op_avt.addrMode = REGISTER_DIRECT;
                op_avt.oprendVal = pick_one_free_vfp_register();

                ftosi_and_ftout_instruction("FTOSI",op_avt,op_bvt,FloatTyID);
                recycle_vfp_register(op_bvt.oprendVal);

                AssembleOperand mem;
                mem.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
                mem.oprendVal = SP;
                mem.addtion = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));

                vfp_memory_access_instructions("FST",op_avt,mem,FloatTyID);

                //general_recycle_temp_register(this,TARGET_OPERAND,op_avt);
                recycle_vfp_register(op_avt.oprendVal);
            }
            else if(!ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == ARM)
            {
                //变量为整型 且 存放在arm寄存器
                //int = int
                AssembleOperand mem;
                AssembleOperand reg;
                reg.oprendVal = order;
                reg.addrMode = REGISTER_DIRECT;
                mem.oprendVal = SP;
                mem.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
                mem.addtion = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));

                memory_access_instructions("STR",reg,mem,NONESUFFIX,false,NONELABEL);

                general_recycle_temp_register_conditional(this,TARGET_OPERAND,reg);
            }
            
        break;
        //变量最终存放在寄存器中，
        case IN_REGISTER:
            if(ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == VFP)
            {
                //变量为浮点数 并且 已存放在浮点寄存器中
                //float = float
                //此时按照优解，双目运算的目标寄存器应当就是变量存放的位置
                assert(order == get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i)));
            }
            else if(ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == ARM)
            {
                //变量为浮点数 但是 存放在普通寄存器
                //float = int
                //此时转换成IEEE754形式再存放到指定寄存器
                AssembleOperand op_bvt;
                op_bvt.addrMode = REGISTER_DIRECT;
                op_bvt.oprendVal = pick_one_free_vfp_register();

                AssembleOperand op_in_arm_reg;
                op_in_arm_reg.addrMode = REGISTER_DIRECT;
                op_in_arm_reg.oprendVal = order;
                fmrs_and_fmsr_instruction("FMSR",op_in_arm_reg,op_bvt,FloatTyID);

                //回收临时arm寄存器
                recycle_temp_arm_register(op_in_arm_reg.oprendVal);

                AssembleOperand op_avt;
                op_avt.addrMode = REGISTER_DIRECT;
                op_avt.oprendVal = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));

                //整型转为浮点型 并存储
                fsito_and_fuito_instruction("FSITO",op_avt,op_bvt,FloatTyID);
            } 
            else if(!ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == VFP)
            {
                //变量为整型 但是 存放在浮点寄存器
                //int = float
                //此时转换成补码整型
                AssembleOperand op_bvt;
                op_bvt.addrMode = REGISTER_DIRECT;
                op_bvt.oprendVal = order;
                AssembleOperand op_avt;
                op_avt.addrMode = REGISTER_DIRECT;
                op_avt.oprendVal = pick_one_free_vfp_register();

                ftosi_and_ftout_instruction("FTOSI",op_avt,op_bvt,FloatTyID);
                recycle_vfp_register(op_bvt.oprendVal);

                //将整型变量存回arm寄存器
                AssembleOperand op_origin;
                op_origin.addrMode = REGISTER_DIRECT;
                op_origin.oprendVal = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));

                fmrs_and_fmsr_instruction("FMRS",op_origin,op_avt,FloatTyID);
            }
            else if(!ins_operand_is_float(this,TARGET_OPERAND) && register_type(order) == ARM)
            {
                //变量为整型 且 存放在arm寄存器
                //int = int
                assert(order == get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i)));
            }
        break;
    }

}

/**
 * @brief 这个方法分析指令的第i个操作数，若变量在内存中，产生一个访存指令后返回临时寄存器
 *        若在寄存器和指令中，返回对应的寄存器或常数
 * @param regType 指定使用arm寄存器还是vfp
 * @return 若使用了临时寄存器，返回寄存器的编号，否则返回-1
 * @author created by LGD on 20221225
 * @update  20230112 新增指定用通用还是浮点寄存器
 *          20220109 当选取的是目的操作数时，不再访存
 *          20220103 offset报错,raspberry OS 和编译器的奇怪问题 遂将声明提前
*/
size_t ins_variable_load_in_register(Instruction* this,int i,ARMorVFP regType,AssembleOperand* op)
{

    RegorMem rom = get_variable_place_by_order(this,i);
    bool use_temp_reg;
    int offset;
    RegisterOrder tempReg;
    switch(rom)
    {
        case IN_MEMORY:

            /* 第1/2操作数在内存时，先取到临时寄存器 */
            offset = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));
            switch(regType)
            {
                case ARM:
                    tempReg = pick_one_free_temp_arm_register();
                    op->addrMode = REGISTER_DIRECT;
                    op->oprendVal = tempReg;
                    use_temp_reg = true;

                    //若为目的操作数，直接选取一个临时寄存器即可
                    if(i==0)return tempReg;

                    AssembleOperand accessOpInMem[2];
                    accessOpInMem[0].oprendVal = tempReg;
                    accessOpInMem[0].addrMode = REGISTER_DIRECT;

                    accessOpInMem[1].oprendVal = SP;
                    accessOpInMem[1].addrMode = REGISTER_INDIRECT_WITH_OFFSET;
                    accessOpInMem[1].addtion = offset;      
                    memory_access_instructions("LDR",accessOpInMem[0],accessOpInMem[1],NONESUFFIX,false,NONELABEL);
                break;
                case VFP:
                {
                    AssembleOperand sm;
                    AssembleOperand mem;
                    AssembleOperand fd;
                    tempReg = pick_one_free_vfp_register();
                    op->addrMode = REGISTER_DIRECT;
                    op->oprendVal = tempReg;
                    //若为目的操作数，直接选取一个临时寄存器即可
                    if(i==0)return tempReg;

                    switch(value_get_type(ins_get_operand(this,i)))
                    {
                        case FloatTyID:
                            sm.addrMode = REGISTER_DIRECT;
                            sm.oprendVal = tempReg;

                            mem.oprendVal = SP;
                            mem.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
                            mem.addtion = offset;   

                            vfp_memory_access_instructions("FLDR",sm,mem,FloatTyID);
                            break;
                        case IntegerTyID:
                            //若为整数还要经过一次类型转换
                            fd.addrMode = REGISTER_DIRECT;
                            fd.oprendVal = tempReg;

                            sm.oprendVal = pick_one_free_vfp_register();
                            sm.addrMode = REGISTER_DIRECT;

                            mem.oprendVal = SP;
                            mem.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
                            mem.addtion = offset;      
                            vfp_memory_access_instructions("FLDR",sm,mem,FloatTyID);
                            
                            fsito_and_fuito_instruction("FSITO",fd,sm,FloatTyID);
                            //归还第一次访存的浮点寄存器 
                            recycle_vfp_register(sm.oprendVal);
                            break;
                    }                  
                }
                break; 
            }
        break;
        case IN_INSTRUCTION:
            //在指令中
            op->addrMode = IMMEDIATE;
            //将立即数（应该在value->pdata中）
            op->oprendVal = ins_get_constant(this,i);

            use_temp_reg = false;
        break;
        case IN_REGISTER:
            //在寄存器中
            if(value_get_type(ins_get_operand(this,i)) == IntegerTyID && regType == VFP)
            {
                AssembleOperand origin_arm_reg; //arm
                AssembleOperand op_bcvt; //vfp
                origin_arm_reg.addrMode = REGISTER_DIRECT;
                origin_arm_reg.oprendVal = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));
                op_bcvt.addrMode = REGISTER_DIRECT;
                op_bcvt.oprendVal = pick_one_free_vfp_register();
                // 整型但需要类型提升为浮点型 先转存到浮点寄存器
                fmrs_and_fmsr_instruction("FMSR",origin_arm_reg,op_bcvt,FloatTyID);
                recycle_temp_arm_register(origin_arm_reg.oprendVal);

                AssembleOperand op_acvt;
                op_acvt.addrMode = REGISTER_DIRECT;
                op_acvt.oprendVal = pick_one_free_vfp_register();
                //转换为浮点型
                fsito_and_fuito_instruction("FSITO",op_acvt,op_bcvt,FloatTyID);
                recycle_vfp_register(op_bcvt.oprendVal);
                op->addrMode = REGISTER_DIRECT;
                op->oprendVal = op_acvt.oprendVal;
            }
            else if ((value_get_type(ins_get_operand(this,i)) == IntegerTyID && regType == ARM) || 
                        (value_get_type(ins_get_operand(this,i)) == FloatTyID && regType == VFP))
            {
                op->addrMode = REGISTER_DIRECT;
                //将寄存器编号赋给operandVal
                op->oprendVal = get_variable_register_order_or_memory_offset_test(this,ins_get_operand(this,i));
            }
            use_temp_reg = true;
        break;
    }

    if(use_temp_reg)
        return op->oprendVal;
    else
        return -1;
} 

