#ifndef _ARM_ASSEMBLY_H
#define _ARM_ASSEMBLY_H

#include "instruction.h"
#include "type.h"
#include "value.h"
#include <assert.h>
#include "arm.h"

#include "dependency.h"
void initDlist();
//assmNode* Add(Instruction* ins);

extern TempReg TempARMRegList[];
extern TempReg TempVFPRegList[TEMP_VFP_REG_NUM];

/**
 * @brief 浮点临时寄存器获取三件套
*/
void Free_Vps_Register_Init();
RegisterOrder pick_one_free_vfp_register();
void recycle_vfp_register(RegisterOrder reg);

/**
 * @brief arm临时寄存器获取三件套
*/
void Init_arm_tempReg();
unsigned pick_one_free_temp_arm_register();
void recycle_temp_arm_register(int reg);

bool goto_is_conditional(TAC_OP op);
/*******************************************/
/*             限制级别的寄存器             */
/*  这些方法将对所有获取临时寄存器的行为生效  */
/*******************************************/
/**
 * @brief 判断当前寄存器是否是限制级别寄存器
 * @birth: Created by LGD on 2023-5-4
*/
bool Is_limited_temp_register(RegisterOrder reg);

/**
 * @brief 添加新的限制级别
 * @birth: Created by LGD on 2023-5-4
 * @update: 2023-7-28 现在您每次调用仅能限制一个寄存器
*/
void add_register_limited(RegisterOrder limitedReg);

/**
 * @brief 根据参数个数限制
 * @birth: Created by LGD on 2023-7-28
*/
void add_parameter_limited(size_t regNum);

/**
 * @brief 移除一个限制级别，如果其本身没有这个限制级别，将忽略
 * @birth: Created by LGD on 2023-5-4
 * @update: 2023-7-28 现在您每次调用仅能限制一个寄存器
*/
void remove_register_limited(RegisterOrder limitedReg);

/**
 * @brief 判断一个指令的操作数是否是浮点数
*/
bool ins_operand_is_float(Instruction* this,int opType);

/**
 * @brief 统一为整数和浮点数变量归还寄存器
*/
void general_recycle_temp_register(Instruction* this,int i,struct _operand op);

/**
 * @brief 回收寄存器的特殊情况，在操作数取自内存或者发生了隐式类型转换
 * @param specificOperand 可选 FIRST_OPERAND SECOND_OPERAND
 * @birth: Created by LGD on 20230227
*/
void general_recycle_temp_register_conditional(Instruction* this,int specificOperand,struct _operand recycleRegister);

/**
 * @brief 返回当前寄存器的类型
 * @return 两类 arm寄存器 或vfp浮点寄存器
 * @author Created by LGD on 20230113
*/
ARMorVFP register_type(RegisterOrder reg);


/**
 * @brief 将Instruction中的变量转换为operand格式的方法
 * @birth: Created by LGD on 20230130
 * @todo 反思一下为什么一个函数要调用这么多复杂的形参呢？
*/
struct _operand toOperand(Instruction* this,int i);


/**
 * @brief 将数据从寄存器溢出到内存，由于相对寻址范围可能解释成多条指令
 * @birth: Created by LGD on 2023-7-17
**/
void reg2mem(struct _operand reg,struct _operand mem);


/**
 * @brief 将数据从内存加载到寄存器，由于相对寻址范围可能解释成多条指令
 * @birth: Created by LGD on 2023-7-17
**/
void mem2reg(struct _operand reg,struct _operand mem);

/***
 * tar:int = op:float
 * @birth: Created by LGD on 20230130
*/
void movif(struct _operand tar,struct _operand op1);
/***
 * tar:float = op:int
 * @birth: Created by LGD on 20230130
*/
void movfi(struct _operand tar,struct _operand op1);
/**
 * @brief movff
 * @birth: Created by LGD on 20230201
*/
void movff(struct _operand tar,struct _operand op1);
/**
 * @brief movii
 * @birth: Created by LGD on 20230201
*/
void movii(struct _operand tar,struct _operand op1);

/**
 * @brief 数据与数据之间的传递，包含隐式转换，支持所有内存、立即数和寄存器类型
 * @birth: Created by LGD on 2023-7-23
*/
void mov(struct _operand tar,struct _operand src);

/**
 * @brief movini
 * @birth: Created by LGD on 2023-7-11
*/
void movini(AssembleOperand tar,AssembleOperand op1);

/**
 * @brief movCondition
 * @birth: Created by LGD on 20230201
*/
void movCondition(struct _operand tar,struct _operand op1,enum _Suffix cond);
/**
 * @brief cmpii
 * @birth: Created by LGD on 2023-4-4
 * @todo 更改回收寄存器的方式
*/
void cmpii(struct _operand tar,struct _operand op1);

/**
 * @brief cmpff 比较两个浮点数
 * @birth: Created by LGD on 2023-7-22
*/
void cmpff(struct _operand op1,struct _operand op2);

/**
 * @brief 完成一次整数相减，并把运算结果返回到一个临时的寄存器中
 * @birth: Created by LGD on 2023-7-23
*/
struct _operand subii(struct _operand op1,struct _operand op2);

/**
 * @brief 取两个数相与的结果
 * @birth: Created by LGD on 2023-7-16
**/
void andiii(AssembleOperand tar,AssembleOperand op1,AssembleOperand op2);
/**
 * @brief 取两个数相或的结果
 * @birth: Created by LGD on 2023-7-16
**/
void oriii(AssembleOperand tar,AssembleOperand op1,AssembleOperand op2);

/**
 * @brief 完成一次整数的相减
 * @birth: Created by LGD on 2023-7-22
*/
void subiii(struct _operand tarOp,struct _operand op1,struct _operand op2);

/**
 * @brief 完成一次浮点相减，并把运算结果返回到一个生成的寄存器中
 * @birth: Created by LGD on 2023-7-23
*/
struct _operand subff(struct _operand op1,struct _operand op2);

/**
 * @brief 实现三个IEEE754浮点数的减法，寄存器不需要VFP，不提供格式转换
 * @update: Created by LGD on 2023-7-23
*/
void subfff(struct _operand tarOp,struct _operand op1,struct _operand op2);

/**
 * @brief 双目运算 双整型
 * @birth: Created by LGD on 20230226
*/
 BinaryOperand binaryOpii(struct _operand op1,struct _operand op2);


/**
@brief:完成一次整数的相加
@birth:Created by LGD on 2023-5-29
*/
void addiii(struct _operand tarOp,struct _operand op1,struct _operand op2);

/**
 * @brief 为代码添加文字池以避免文字池太远的问题
 * @birth: Created by LGD on 2023-7-27
*/
void add_interal_pool();

#endif