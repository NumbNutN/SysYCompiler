#ifndef _ARM_H
#define _ARM_H
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "type.h"
#include "operand.h"

extern int CntAssemble;

typedef enum _AssemInstruction
{
    ADD,     //加法指令
    SUB,
    MUL,     //32位乘法指令
    MLA,     //32位乘加指令
    UMULL,   //64位无符号乘法
    UMLAL,   //64位无符号乘加
    DIV,
} AssemInstruction;



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


typedef struct _assemNode
{
    //操作码
    char opCode[4];
    //操作数
    AssembleOperand op[3];
    //定义了操作数的数量
    unsigned op_len;

    //定义了助记符                  //20221202
    char suffix[3];
    //是否要影响标记位
    bool symbol;
    //定义了语句前标号
    char label[10];

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

//ARM指令
assmNode* memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,char* suffix,bool symbol,char* label);

#define END -1LL
/**
 * @brief 变长的push尝试
 * @birth: Created by LGD on 2023-4-9
*/
void bash_push_pop_instruction(char* opcode,...);
void push_pop_instructions(char* opcode,AssembleOperand reg);
assmNode* general_data_processing_instructions(char* opCode,AssembleOperand tar,AssembleOperand op1,AssembleOperand op2,char* suffix,bool symbol,char* label);
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
 * @brief 对单个汇编语句节点的格式化输出
 * @birth: Created by LGD on 20230227
*/
void print_single_assembleNode(assmNode* p);
//打印输出
void print_model();

#endif