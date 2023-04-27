#ifndef _ARM_H
#define _ARM_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "type.h"

extern int CntAssemble;

typedef enum _ARM_Instruction_Mnemonic
{
    ADD,     //加法指令
    SUB,     //减法
    RSB,     //32位逆向减法
    MUL,     //32位乘法指令
    MLA,     //32位乘加指令

    MOV,
    CMP,

    FADD,
    FSUB,
    FMUL,
    FDIV,

    LDR,
    STR,


    UMULL,   //64位无符号乘法
    UMLAL,   //64位无符号乘加
    DIV,
} ARM_Instruction_Mnemonic;



/**
 * @brief 定义了所有的语句类型
 * @update: 2023-4-9 添加了ASSEM_PUSH_POP_INSTRUCTION
*/
typedef enum
{
    ASSEM_INSTRUCTION,
    ASSEM_PUSH_POP_INSTRUCTION,
    ASSEM_PSEUDO_INSTRUCTION,
    LABEL,
    BI,      //Branch Instructions
    LDR_PSEUDO_INSTRUCTION,
    
} ASSEMBLE_TYPE;


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

//2023-4-20
enum SHIFT_WAY{
    NONE_SHIFT,
    LSL,
    LSR,
    ASR,
    ROR,
    RRX
};

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
    //定义了第二操作数的左移选项  2023-4-20
    enum SHIFT_WAY shiftWay;
    //定义了移位的数量  2023-4-20
    size_t shiftNum;


} AssembleOperand;
//ADD [SP,#4]

typedef struct _assemNode
{
    //操作码
    char opCode[4];
    //操作数
    AssembleOperand op[4];
    //定义了操作数的数量
    unsigned op_len;

    //定义了助记符   2022-12-02
    char suffix[3];
    //是否要影响标记位
    bool symbol;

    //标号节点的标号
    char label[20];

    //mov 扩展 移位位数  2023-4-19
    int lslnum;

    //说明节点是指令/伪指令   //20221203
    ASSEMBLE_TYPE assemType;

    //push pop的扩展   2023-4-9
    AssembleOperand* opList;
    //指向下一个节点
    struct _assemNode* next;

} assmNode;


//链式汇编节点的头节点
extern assmNode* head;
//该指针始终指向当前上一个节点
extern assmNode* prev;

typedef enum _RegorMem
{
    IN_REGISTER     = 1,
    IN_MEMORY       = 2,
    IN_INSTRUCTION  = 4,
    UNALLOCATED     = 8
} RegorMem;

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
extern struct _operand r0;
extern struct _operand r1;

extern struct _operand sp;
extern struct _operand lr;
extern struct _operand fp;
extern struct _operand pc;
extern struct _operand sp_indicate_offset;

extern struct _operand trueOp;
extern struct _operand falseOp;


//arm_assemble
#define NONESUFFIX ""
#define NONELABEL "\t"

#define TARGET_OPERAND 0
#define FIRST_OPERAND 1
#define SECOND_OPERAND 2

#define Rd  TARGET_OPERAND
#define Rn  FIRST_OPERAND
#define Rm  SECOND_OPERAND
#define Ra  3

typedef enum{
    GT,LT,GE,LE,EQ,NE,
    S
} Suffix;


#define FIRST_ARM_REGISTER R0
#define LAST_ARM_REGISTER SPSR
#define FIRST_VFP_REGISTER S0
#define LAST_VFP_REGISTER S32

extern struct _operand nullop;

typedef enum _RecycleCondition
{
    NO_NEED_TO_RECYCLE = 0,
    VARIABLE_IN_MEMORY = 1,
    VARIABLE_LDR_FROM_IMMEDIATE = 2,
    INTERGER_PART_IN_MIX_CALCULATE = 4
} RecycleCondition;


//ARM指令
assmNode* memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,char* suffix,bool symbol,char* label);

#define END -1LL
/**
 * @brief 变长的push尝试
 * @birth: Created by LGD on 2023-4-9
*/
void bash_push_pop_instruction(char* opcode,...);
void push_pop_instructions(char* opcode,AssembleOperand reg);
/**
 * @brief 翻译通用数据传输指令
 * @update: 2023-3-19 根据乘法寄存器的要求对rm和rs互换
 * @update: 2023-4-20 更改了指令助记符的类型   删去了label选项
*/
void general_data_processing_instructions(enum _ARM_Instruction_Mnemonic opCode,AssembleOperand rd,AssembleOperand rn,AssembleOperand second_operand,char* cond,bool symbol);
/**
 * @brief 翻译通用数据传输指令
 * @update: 2023-3-19 根据乘法寄存器的要求对rm和rs互换
 * @update: 2023-4-20 更改了指令助记符的类型   删去了label选项
*/
void general_data_processing_instructions_extend(enum _ARM_Instruction_Mnemonic opCode,char* cond,bool symbol,...);
void branch_instructions(char* tarLabel,char* suffix,bool symbol,char* label);
void branch_instructions_test(char* tarLabel,char* suffix,bool symbol,char* label);

/**
 * @brief   浮点数的访存指令
 * @author  Created by LGD on 20230111
*/
void vfp_memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,TypeID type);
/**
 * @brief 将浮点数转化为有符号或无符号整数  FTOST 转化为有符号整型  FTOUT转化为无符号整型  
 * @param sd 存放整型结果的单精度浮点寄存器
 * @param fm 存放待转换的浮点数的浮点寄存器，其类型必须满足类型后缀
*/
void ftost_and_ftout_instruction(char* opCode,AssembleOperand sd,AssembleOperand fm,TypeID type);
/**
 * @brief 将有符号或无符号整数转化为浮点数, FSITO 将有符号整数转为浮点数  FUITO 将无符号整数转为浮点数
 * @param fd 存放浮点数结果的浮点寄存器，其类型必须满足类型后缀
 * @param sm 存放整数的单精度浮点寄存器
*/
void fsito_and_fuito_instruction(char* opCode,AssembleOperand fd,AssembleOperand sm,TypeID type);
/**
 * @brief 浮点数的加减法运算
 * @author Created by LGD on 20230113
*/
void fadd_and_fsub_instruction(char* opCode,AssembleOperand fd,AssembleOperand fn,AssembleOperand fm,TypeID type);
/**
 * @brief arm寄存器与vfp寄存器之间的相互直传
 *          fmrs 将浮点寄存器直传到arm寄存器
 *          fmsr 将arm寄存器直传到浮点寄存器
 * @param rd arm寄存器
 * @param sn 浮点寄存器
 * @author Created by LGD on 20230113
*/
void fmrs_and_fmsr_instruction(char* opCode,AssembleOperand rd,AssembleOperand sn,TypeID type);
/**
 * @brief 浮点数的CMP指令
 * @author Created by LGD on 20230113
*/
void fcmp_instruction(AssembleOperand fd,AssembleOperand fm,TypeID type);
/**
 * @brief 浮点寄存器VFP之间的传送指令
 *          FABS 取绝对值后传递
 *          FCPY 直传
 *          FNEG 取相反数后传递
 * @param fd 目标操作数
 * @param fm 待直传的操作数
 * @author Created by LGD on 20230114
*/
void fabs_fcpy_and_fneg_instruction(char* opCode,AssembleOperand fd,AssembleOperand fm,TypeID type);

//伪指令
void Label(char* label);

/**
 * @brief FLD伪指令
 * @param opCode LDR
 * @birth: Created by LGD on 20230202
*/
void pseudo_fld(char* opCode,AssembleOperand reg,AssembleOperand immedi,TypeID type);

/**
 * @brief LDR伪指令
 * @param opCode LDR
 * @birth: Created by LGD on 20230202
*/
void pseudo_ldr(char* opCode,AssembleOperand reg,AssembleOperand immedi);

void linkNode(assmNode* now);






/**
 * @brief 返回汇编指令指定的操作数
 * @birth: Created by LGD on 2023-4-18
*/
struct _operand AssemblyNode_get_opernad(struct _assemNode* assemNode,size_t idx);

#endif