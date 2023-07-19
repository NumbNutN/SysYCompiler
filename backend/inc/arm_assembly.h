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

extern TempReg TempARMRegList[TEMP_REG_NUM];
extern TempReg TempVFPRegList[TEMP_REG_NUM];
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
size_t ins_variable_load_in_register(Instruction* this,int i,ARMorVFP regType,struct _operand* op);

/**
 * @brief 这个方法分析指令的第i个操作数，若变量在内存中，产生一个访存指令后返回临时寄存器,若在寄存器和指令中，返回对应的寄存器或常数
 * @author created by LGD on 20221225
 * @return 若使用了临时寄存器，返回寄存器的编号，否则返回-1
*/
size_t variable_load_in_register(Instruction* this,Value* op,struct _operand* assem_op);

void variable_storage_back(Instruction* this,int i,int order);

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
*/
void add_register_limited(enum _Pick_Arm_Register_Limited limited);
/**
 * @brief 移除一个限制级别，如果其本身没有这个限制级别，将忽略
 * @birth: Created by LGD on 2023-5-4
*/
void remove_register_limited(enum _Pick_Arm_Register_Limited limited);

/**
 * @brief 判断一个变量是否是浮点数
 * @author Created by LGD on 20230113
*/
bool variable_is_float(Value* var);
/**
 * @brief 判断一个指令的操作数是否是浮点数
*/
bool ins_operand_is_float(Instruction* this,int opType);

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
void ins_target_operand_load_in_register(Instruction* this,struct _operand* op,int operandType);
/**
 * @brief 将变量存储回原来的位置
 *          若在内存中，将最终结果存到内存
 *          若在寄存器，将最终结果存在寄存器，若本身在指定寄存器则忽略
 *          若类型不符，则将类型转正确后再存
 * @author Created by LGD on 20230113
*/
void variable_storage_back_new(Instruction* this,int i,RegisterOrder order);


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
 * @brief 双目运算 双整型
 * @birth: Created by LGD on 20230226
*/
 BinaryOperand binaryOpii(struct _operand op1,struct _operand op2);
 /**
 * @brief 双目运算 整数浮点混合双目运算
 * @birth: Created by LGD on 20230226
*/
 BinaryOperand binaryOpfi(struct _operand op1,struct _operand op2);
 /**
 * @brief 双目运算 浮点运算
 * @birth: Created by LGD on 20230226
*/
BinaryOperand binaryOpff(struct _operand op1,struct _operand op2);

/**
@brief:完成一次整数的相加
@birth:Created by LGD on 2023-5-29
*/
void addiii(struct _operand tarOp,struct _operand op1,struct _operand op2);

#endif