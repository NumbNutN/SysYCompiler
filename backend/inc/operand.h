#ifndef _OPERAND_H
#define _OPERAND_H

#include "instruction.h"


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
 * @brief AssembleOperand 将内存中的操作数加载到临时寄存器,这次，你可以自定义用什么寄存器加载了
 * @birth: Created by LGD on 20230130
*/
AssembleOperand operand_load_from_memory(AssembleOperand op,ARMorVFP type);

/**
 * @brief 
 * @param op 需要读取到寄存器的operand
 * @param type 读取到的寄存器类型
 * @update: Created by LGD on 2023-4-11
*/
AssembleOperand operand_load_from_memory_to_spcified_register(AssembleOperand op,ARMorVFP type,AssembleOperand dst);

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
AssembleOperand operand_load_immediate(AssembleOperand src,ARMorVFP type);
/**
 * @brief 立即数读取到指令寄存器
 * @update:Created by LGD on 2023-4-11
*/
AssembleOperand operand_load_immediate_to_specified_register(AssembleOperand src,ARMorVFP type,AssembleOperand dst);
/**
 * @brief 判断一个operand是否在指令中
 * @birth: Created by LGD on 20230328
*/
bool opernad_is_in_instruction(AssembleOperand op);
/**
 * @brief 判断一个operand是否在内存中
 * @birth: Created by LGD on 2023-4-24
*/
bool operand_is_in_memory(AssembleOperand op);
/**
 * @brief 判断一个operand是否在寄存器中
 * @birth: Created by LGD on 2023-4-24
*/
bool operand_is_in_register(AssembleOperand op);

/**
 * @brief 将操作数取到一个寄存器中，或者其他定制化需求
 * @param op 需要被加载到其他位置的操作数，如果第四个参数置为0，则视为全部情况加载
 * @param mask 为1时表示第四个参数为屏蔽选项，及当前操作数处于这些位置时不加载到寄存器
 *             为0时表示第四个参数为选择选项，及当前操作数处于这些位置时要加载到寄存器
 *             选项包括 IN_MEMORY IN_REGISTER IN_INSTRUCTION 这些选项可以或在一起
 * @param rom 见第三个参数的描述
 * @birth: Created by LGD on 2023-4-24
*/
struct _operand operandConvert(AssembleOperand op,enum _ARMorVFP aov,bool mask,enum _RegorMem rom);

/**
 * @brief 设置操作数的移位操作
 * @birth: Created by LGD on 2023-4-20
*/
void operand_set_shift(AssembleOperand* Rm,enum SHIFT_WAY shiftWay,size_t shiftNum);


/**
 * @brief 获取操作数的立即数
 * @birth: Created by LGD on 2023-4-18
*/
size_t operand_get_immediate(AssembleOperand op);

#endif