#ifndef _OPERAND_H
#define _OPERAND_H

#include "instruction.h"

typedef enum _DataFormat
{
    INTERGER_TWOSCOMPLEMENT,
    IEEE754_32BITS
} DataFormat;

typedef enum _AddrMode
{
    NONE_ADDRMODE,      //留空符
    IMMEDIATE,          //立即寻址     MOV R0, #15
    DIRECT,             //直接寻址     LDR R0, MEM
                        //            LDR R0, [3102H]
    REGISTER_DIRECT,    //寄存器寻址   MOV R0, R1
    REGISTER_INDIRECT,   //寄存器间接寻址 LDR R0, [R1]
    REGISTER_INDIRECT_WITH_OFFSET,     //前变址
    REGISTER_INDIRECT_PRE_INCREMENTING, //自动变址
    REGISTER_INDIRECT_POST_INCREMENTING, //后变址

    //不是寻址方式 作为操作数的标记
    TARGET_LABEL, //B/BL/BX 语句的标号           //20221202     
    PP            //PUSH/POP 语句的标号 

} AddrMode;

#define FLOATING_POINT_REG_BASE 20
typedef enum _RegisterOrder
{
    IN_MEM = -1,  //内存
    R0,R1,R2,R3,R4,R5,R6,R7,R8,      //通用寄存器
    R9_SB,R10_SL,R11_FP,R12_IP,      //分组寄存器
    R13,                             //堆栈指针，最多允许六个不同的堆栈空间                           
    R14,                              //链接寄存器，子程序调用保存返回地址
    R15,                              //(R15)
    CPSR,
    SPSR,

    S0 = FLOATING_POINT_REG_BASE,
    S1,S2,S3,S4,S5,S6,S7,S8,S9,S10,
    S11,S12,S13,S14,S15,S16,S17,S18,S19,S20,
    S21,S22,S23,S24,S25,S26,S27,S28,S29,S30,
    S31,S32,

    SP = R13,
    LR = R14,
    PC = R15

} RegisterOrder;

#define FP R7

typedef enum _ARMorVFP
{
    ARM,
    VFP
} ARMorVFP;

typedef struct _operand
{
    //定义了操作数的寻址方式
    enum _AddrMode addrMode;
    //定义了操作数的值
    uint64_t oprendVal;         //unsigned 的大小不足以存放指针 20221202
    //附加内容
    unsigned addtion;
    //定义当前存储的数据的格式
    DataFormat format;


} AssembleOperand;

/**
 * @brief 这个结构体用于双目运算的返回结果
*/
typedef struct _BinaryOperand
{
    struct _operand op1;
    struct _operand op2;

} BinaryOperand;

extern struct _operand immedOp;
extern struct _operand r027[8];
extern struct _operand sp;
extern struct _operand lr;
extern struct _operand fp;
extern struct _operand pc;
extern struct _operand sp_indicate_offset;

extern struct _operand trueOp;
extern struct _operand falseOp;

/**
 * @brief 依据Value* 返回 operand
 * @birth: Created by LGD on 2023-3-16
*/
AssembleOperand ValuetoOperand(Instruction* this,Value* var);

/**
 * @brief 将Instruction中的变量转换为operand格式的方法
 * @birth: Created by LGD on 20230130
 * @todo 反思一下为什么一个函数要调用这么多复杂的形参呢？
*/
AssembleOperand toOperand(Instruction* this,int i);


/**
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_load_in_mem_throw(AssembleOperand op);

/**
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器,这次，你可以自定义用什么寄存器加载了
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_load_in_mem(AssembleOperand op,ARMorVFP type);

/**
 * @brief 把暂存器存器再封装一层
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_pick_temp_register(ARMorVFP type);

/**
 * @brief 进行一次无用的封装，现在可以以operand为参数归还临时寄存器
 * @birth: Created by LGD on 20230130
 * @update: 20230226 归还寄存器应当依据其寄存器编号而非其数据编码类型
*/
void operand_recycle_temp_register(AssembleOperand tempReg);




/**
 * @brief 封装直传函数，可以自行判断寄存器是什么类型
 * @param src 源寄存器
 * @param recycleSrc 是否回收源寄存器
 * @birth: Created by LGD on 20230201
*/
AssembleOperand operand_float_deliver(AssembleOperand src,bool recycleSrc);

/**
 * @brief 封装转换函数，可以自行判断res是什么数据类型
 * @param src 源寄存器
 * @param recycleSrc 是否回收源寄存器
 * 
 * @param pickTempReg 是否申请一个临时寄存器，如果选否，需要为第四函数赋值，同时返回值将为nullop 否则赋nullop
 * @param res 目标寄存器
 * @birth: Created by LGD on 20230201
*/
AssembleOperand operand_float_convert(AssembleOperand src,bool recycleSrc);

/**
 * @brief 将立即数用FLD伪指令读取到临时寄存器中，LDR / FLD通用
 * @birth: Created by LGD on 20230202
*/
AssembleOperand operand_ldr_immed(AssembleOperand src,ARMorVFP type);

/**
 * @brief 判断一个operand是否在指令中
 * @birth: Created by LGD on 20230328
*/
bool opernad_is_in_instruction(AssembleOperand op);

#endif