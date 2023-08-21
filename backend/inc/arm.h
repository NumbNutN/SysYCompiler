#ifndef _ARM_H
#define _ARM_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "type.h"

#include "cds.h"
#include "value.h"
#include "instruction.h"

extern int CntAssemble;

#define ARM_WORD_IMMEDIATE_OFFSET_RANGE 4096
typedef enum _ARM_Instruction_Mnemonic
{
    ADD,     //加法指令
    SUB,     //减法
    RSB,     //32位逆向减法
    MUL,     //32位乘法指令
    MLA,     //32位乘加指令

    MOV,
    MVN,
    CMP,

    MOVW,
    MOVT,

    AND,
    ORR,

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
    FMSTAT, /* 2023-7-22 MSR FMSCR, CPSR */
    BI,      //Branch Instructions
    UNDEF,  /*Undefined Instruction 2023-7-9*/
    LDR_PSEUDO_INSTRUCTION,
    POOL, /*2023-7-21 文字池*/
    INSTRUCTION_BOUNDARY    //2023-8-21 定义新的节点
    
} ASSEMBLE_TYPE;


typedef enum _DataFormat
{
    NO_FORMAT,
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
    PP,            //PUSH/POP 语句的标号 
    LABEL_MARKED_LOCATION

} AddrMode;

#define FLOATING_POINT_REG_BASE 20
typedef enum _RegisterOrder
{
    IN_MEM = -1,  //内存
    R0,R1,R2,R3,R4,R5,R6,R7,R8,      //通用寄存器
    R9,R10,R11,R12,
    SB=R9,SL=R10,FP=R11,IP=R12,      //分组寄存器
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

enum _OffsetType{
    NONE_OFFSET,
    OFFSET_IMMED,
    OFFSET_IN_REGISTER
};

enum _RegStat{
    REG_USED,
    REG_KILLED
};

typedef struct _operand
{
    //定义了操作数的寻址方式
    enum _AddrMode addrMode;
    //定义了操作数的值
    uint64_t oprendVal;         //unsigned 的大小不足以存放指针 20221202
    //附加内容
    unsigned addtion;
    //定义了变址类型
    enum _OffsetType offsetType;
    //定义当前寄存器值或立即数的格式
    DataFormat format;
    //定义当前间址内存空间的数据格式 2023-7-24
    DataFormat point2spaceFormat;
    //定义了第二操作数的左移选项  2023-4-20
    enum SHIFT_WAY shiftWay;
    //定义了移位的数量  2023-4-20
    size_t shiftNum;

    //2023-8-21 标注寄存器状态
    enum _RegStat regStat;

} AssembleOperand;


/**************************************************************/
/*                          Section                           */
/***************************************************************/

/**
@brief:定义了Section
@birth:Created by LGD on 2023-5-29
*/
enum Section{
    NONESECTION,
    CODE,
    DATA,
    BSS
};

enum AsDirectiveType{
    TYPE,
    GLOBAL
};

extern enum Section currentSection;

void change_currentSection(enum Section c);

struct _AsDirective{
    enum AsDirectiveType type;
    char* label;
    struct _AsDirective* next;
};

extern struct _AsDirective* asList;
extern struct _AsDirective* asPrev;

/**************************************************************/
/*                      Data Section                          */
/***************************************************************/

extern List* dataList;
extern struct _dataNode* bssList;
extern struct _dataNode* dataPrev;
extern struct _dataNode* bssPrev;

enum _Data_Expression{
    NONE_CNT,
    DOT_LONG,
    DOT_ZERO
};

typedef struct _dataNode{
    //标签
    char* label;
    //声明词
    enum _Data_Expression dExp;
    //内容
    uint32_t content;
    //下一个数据段节点
    struct _dataNode* next;
} DataNode;

/**
 * @brief 只在数据段打印标签
 * @birth: Created by LGD on 2023-7-20
*/
void data_label(char* name);

/**
 * @brief: .long表达式
 * @birth:Created by LGD on 2023-5-29
 * @update: long指令被设计为可替换的
*/
void dot_long_expression(char* name,struct _operand expr,bool replacale);
/**
 * @brief: .zero表达式
 * @birth:Created by LGD on 2023-5-29a
 * @update: 2023-7-20 必须指定空间大小
 *          2023-8-14 现在需要指令定义的段
*/
void dot_zero_expression(enum Section sec,char* name,size_t space);
/**
 * @brief 翻译数组字面量
 * @return 为1代表当前数组有初始化，为0则否
 * @birth: Created by LGD on 2023-7-20
 * @update: 2023-7-21 返回布尔值判断是否有字面量
 *          2023-7-21 数组的最后空白部分也要补齐
 *          2023-8-9 为了区分数组的类型 数组字面量初始化需要提供value
*/
bool array_init_literal(Value* var,size_t total_space,List* literalList);

/****************************************************************/
/*                           Other                             */
/***************************************************************/
/**
 * @brief As伪指令-set function
 * @birth: Created by LGD on 2023-6-6
*/
void as_set_function_type(char* name);

/**************************************************************/
/*                      Code Section                          */
/***************************************************************/

typedef struct _assemNode
{
    //操作码
    char opCode[16];
    //操作数
    AssembleOperand op[4];
    //定义了操作数的数量
    unsigned op_len;

    //定义了条件码   2022-12-02
    char suffix[8];
    //是否要影响标记位
    bool symbol;

    //标号节点的标号
    char label[128];

    //mov 扩展 移位位数  2023-4-19
    int lslnum;

    //说明节点是指令/伪指令   //20221203
    ASSEMBLE_TYPE assemType;

    //push pop的扩展   2023-4-9
    AssembleOperand* opList;
    //指向下一个节点
    struct _assemNode* next;

    //指向上一个节点  2023-8-21
    struct _assemNode* past;

    //2023-8-21  boundary 扩展
    int8_t addtion;

    //2023-8-21 label引用链 扩展
    struct _assemNode* refereence_next;

    //2023-8-21 扩展 当前label的等效label  无则NULL
    char equalLabel[128];

} assmNode;


//链式汇编节点的头节点
extern assmNode* head;
//该指针始终指向当前上一个节点
extern assmNode* prev;
//最后一个节点
extern assmNode* last;

typedef enum _RegorMem
{
    IN_REGISTER     = 1,
    IN_STACK_SECTION       = 2,
    IN_DATA_SEC     = 4,
    IN_INSTRUCTION  = 8,
    IN_LITERAL_POOL = 16,
    UNALLOCATED     = 32
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
extern struct _operand r2;

extern struct _operand sp;
extern struct _operand lr;
extern struct _operand fp;
extern struct _operand pc;
extern struct _operand sp_indicate_offset;
extern struct _operand fp_indicate_offset;
extern struct _operand param_push_op_interger;
extern struct _operand param_push_op_float;

extern struct _operand trueOp;
extern struct _operand falseOp;
extern struct _operand floatZeroOp;

extern struct _operand r023_float[4];
extern struct _operand r023_int[4];
extern struct _operand s023_float[4];

extern struct _operand returnIntOp;
extern struct _operand returnFloatOp;
extern struct _operand returnFloatSoftOp;

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

typedef enum _Suffix{
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
    INTERGER_PART_IN_MIX_CALCULATE = 4,
    REGISTER_ATTRIBUTED_DIFFER_FROM_VARIABLE_REGISTER = 8
} RecycleCondition;


/**
 * @brief 子程序调用策略
 * @birth: Created by LGD on 2023-7-25
*/
enum _Procedure_Call_Strategy {
    FP_SOFT,
    FP_HARD
};
extern enum _Procedure_Call_Strategy procudre_call_strategy;

//ARM指令
assmNode* memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,char* suffix,bool symbol,char* label);

#define END (void*)-1LL
/**
 * @brief 变长的push尝试
 * @birth: Created by LGD on 2023-4-9
*/
void bash_push_pop_instruction(char* opcode,...);
void push_pop_instructions(char* opcode,AssembleOperand reg);
/**
 * @brief 变长的push尝试
 * @birth: Created by LGD on 2023-4-9
*/
void bash_push_pop_instruction_list(char* opcode,struct _operand* regList);
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
 * @brief 将浮点数转化为有符号或无符号整数  FTOSI 转化为有符号整型  FTOUT转化为无符号整型  
 * @param sd 存放整型结果的单精度浮点寄存器
 * @param fm 存放待转换的浮点数的浮点寄存器，其类型必须满足类型后缀
*/
void ftosi_and_ftout_instruction(char* opCode,AssembleOperand sd,AssembleOperand fm,TypeID type);
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

/**
 * @brief 将FMSCR拷贝到CPSR
 * @birth: Created by LGD on 2023-7-22
*/
void fmstat();

//伪指令
void Label(char* label);
/**
* @brief 生成一个未定义指令
* @birth:Created by LGD on 2023-7-9
*/
void undefined();

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

/**
 * @brief 生成一个文字池伪指令
 * @birth: Created by LGD on 2023-7-21
*/
void pool();

void linkNode(assmNode* now);

/**
 * @brief 从代码链中移除节点
 * @birth: Created by LGD on 2023-8-9
*/
void codeRemoveNode(assmNode* node);

/**
 * @brief 返回汇编指令指定的操作数
 * @birth: Created by LGD on 2023-4-18
*/
struct _operand AssemblyNode_get_opernad(struct _assemNode* assemNode,size_t idx);

/**
 * @brief 为arm指令节点进行初始化
 * @birth: Created by LGD on 2023-7-15
 */
assmNode* arm_instruction_node_init();

/**
 * @brief 插入一个边界节点
 * @birth: Created by LGD on 2023-8-21
*/
void instruction_boundary(TAC_OP opCode);

#endif