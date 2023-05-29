#ifndef _OPERAND_H
#define _OPERAND_H

#include "instruction.h"
#include "arm.h"

/**
 * @brief 依据Value* 返回 operand
 * @birth: Created by LGD on 2023-3-16
*/
struct _operand ValuetoOperand(Instruction* this,Value* var);

/**
 * @brief 将Instruction中的变量转换为operand格式的方法
 * @birth: Created by LGD on 20230130
 * @todo 反思一下为什么一个函数要调用这么多复杂的形参呢？
*/
struct _operand toOperand(Instruction* this,int i);


/**
 * @brief 创建一个立即数操作数
 * @birth: Created by LGD on 2023-5-1
*/
struct _operand operand_create_immediate_op(int immd);

/**
 * @brief 创建一个相对FP/SP偏移的寻址方式操作数
 * @birth: Created by LGD on 2023-5-3
*/
struct _operand operand_create_relative_adressing(RegisterOrder SPorFP,enum _OffsetType immedORreg,int offset);
/**
 * @brief 创建一个相对FP/SP偏移的寻址方式操作数
 *        这个方法不负责回收多余的寄存器
 * @birth: Created by LGD on 2023-5-3
*/
struct _operand operand_create2_relative_adressing(RegisterOrder SPorFP,struct _operand offset);

/**
 * @brief 从内存或者将立即数加载到一个寄存器中，将操作数加载到指定寄存器，该方法不负责检查该寄存器是否被使用
 * @param tarOp 如果指定为一个register operand，加载到该operand；若指定为nullop，选择一个operand
 * @birth: Created by LGD on 2023-5-1
*/
struct _operand operand_load_to_register(AssembleOperand srcOp,AssembleOperand tarOp);

/**
 * @brief 比较两个操作数是否一致
*/
bool operand_is_same(struct _operand dst,struct _operand src);

/**
 * @brief 判断一个操作数是否是空指针
 * @birth: Created by LGD on 2023-5-2
*/
bool operand_is_NULL(AssembleOperand op);
/**
 * @brief 判断一个寄存器属于R4-R12(不包括R7)
 * @birth: Created by LGD on 2023-5-13
*/
bool operand_is_via_r4212(struct _operand reg);
/**
 * @brief struct _operand 将内存中的操作数加载到临时寄存器,这次，你可以自定义用什么寄存器加载了
 * @birth: Created by LGD on 20230130
*/
struct _operand operand_load_from_memory(struct _operand op,enum _ARMorVFP type);

/**
 * @brief 将一个操作数加载到指定的寄存器，无论其在任何位置均确保生产合法的指令，且不破坏其他的寄存器
 * @birth: Created by LGD on 2023-5-14
*/
void operand_load_to_specified_register(struct _operand oriOp,struct _operand tarOp);
/**
 * @brief 
 * @param op 需要读取到寄存器的operand
 * @param type 读取到的寄存器类型
 * @update: Created by LGD on 2023-4-11
*/
struct _operand operand_load_from_memory_to_spcified_register(struct _operand op,enum _ARMorVFP type,struct _operand dst);

/**
 * @brief 把暂存器存器再封装一层
 * @birth: Created by LGD on 20230130
*/
struct _operand operand_pick_temp_register(enum _ARMorVFP type);

/**
 * @brief 进行一次无用的封装，现在可以以operand为参数归还临时寄存器
 * @birth: Created by LGD on 20230130
 * @update: 20230226 归还寄存器应当依据其寄存器编号而非其数据编码类型
*/
void operand_recycle_temp_register(struct _operand tempReg);




/**
 * @brief 封装直传函数，可以自行判断寄存器是什么类型
 * @param src 源寄存器
 * @param recycleSrc 是否回收源寄存器
 * @birth: Created by LGD on 20230201
*/
struct _operand operand_float_deliver(struct _operand src,bool recycleSrc);

/**
 * @brief 封装转换函数，可以自行判断res是什么数据类型
 * @param src 源寄存器
 * @param recycleSrc 是否回收源寄存器
 * 
 * @param pickTempReg 是否申请一个临时寄存器，如果选否，需要为第四函数赋值，同时返回值将为nullop 否则赋nullop
 * @param res 目标寄存器
 * @birth: Created by LGD on 20230201
*/
struct _operand operand_float_convert(struct _operand src,bool recycleSrc);

/**
 * @brief 将立即数用FLD伪指令读取到临时寄存器中，LDR / FLD通用
 * @birth: Created by LGD on 20230202
*/
struct _operand operand_load_immediate(struct _operand src,enum _ARMorVFP type);
/**
 * @brief 立即数读取到指令寄存器
 * @update:Created by LGD on 2023-4-11
*/
struct _operand operand_load_immediate_to_specified_register(struct _operand src,enum _ARMorVFP type,struct _operand dst);
/**
 * @brief 判断一个operand是否在指令中
 * @birth: Created by LGD on 20230328
*/
bool opernad_is_immediate(struct _operand op);
/**
 * @brief 判断一个operand是否在内存中
 * @birth: Created by LGD on 2023-4-24
*/
bool operand_is_in_memory(struct _operand op);
/**
 * @brief 判断一个operand是否在寄存器中
 * @birth: Created by LGD on 2023-4-24
*/
bool operand_is_in_register(struct _operand op);

/**
 * @brief 将操作数取到一个寄存器中，或者其他定制化需求
 * @param op 需要被加载到其他位置的操作数，如果第四个参数置为0，则视为全部情况加载
 * @param mask 为1时表示第四个参数为屏蔽选项，及当前操作数处于这些位置时不加载到寄存器
 *             为0时表示第四个参数为选择选项，及当前操作数处于这些位置时要加载到寄存器
 *             选项包括 IN_MEMORY IN_REGISTER IN_INSTRUCTION 这些选项可以或在一起
 * @param rom 见第三个参数的描述
 * @birth: Created by LGD on 2023-4-24
*/
struct _operand operandConvert(struct _operand op,enum _ARMorVFP aov,bool mask,enum _RegorMem rom);

/**
 * @brief 设置操作数的移位操作
 * @birth: Created by LGD on 2023-4-20
*/
void operand_set_shift(struct _operand* rm,enum SHIFT_WAY shiftWay,size_t shiftNum);


/**
 * @brief 获取操作数的立即数
 * @birth: Created by LGD on 2023-4-18
*/
size_t operand_get_immediate(struct _operand op);

#endif