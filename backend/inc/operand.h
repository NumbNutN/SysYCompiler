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
 * @update: 2023-7-23 支持浮点立即数的创建（IEEE754格式）
*/
struct _operand operand_create_immediate_op(uint32_t immd,enum _DataFormat format);

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
 * @brief 创建一个寄存器直接寻址的操作数
 * @birth: Created by LGD on 2023-7-24
*/
struct _operand operand_create_register_direct(RegisterOrder reg,enum _DataFormat format);

/**
 * @brief 创建一个寄存器间接寻址操作数
 * @birth: Created by LGD on 2023-7-20
 * @update: 2023-7-24 需要指定间址指向的空间存放的数据类型
*/
struct _operand operand_Create_indirect_addressing(RegisterOrder reg,enum _DataFormat point2format);
/**
 * @brief 从内存或者将立即数加载到一个寄存器中，将操作数加载到指定寄存器，该方法不负责检查该寄存器是否被使用
 * @param tarOp 如果指定为一个register operand，加载到该operand；若指定为nullop，选择一个operand
 * @birth: Created by LGD on 2023-5-1
*/
struct _operand operand_load_to_register(AssembleOperand srcOp,AssembleOperand tarOp,...);

/**
 * @brief 比较两个操作数是否一致
*/
bool operand_is_same(struct _operand dst,struct _operand src);

/**
 * @brief 由于仅通过operand判断需不需要临时寄存器需要额外的归类方法
 * @birth: Created by LGD on 20230130
*/
RegorMem operand_in_regOrmem(AssembleOperand op);

/**
 * @brief 判断一个操作数是否是空指针
 * @birth: Created by LGD on 2023-5-2
*/
bool operand_is_none(AssembleOperand op);
/**
 * @brief 判断一个寄存器属于R4-R12(不包括R7)
 * @birth: Created by LGD on 2023-5-13
*/
bool operand_is_via_r4212(struct _operand reg);

/**
 * @brief 获取操作数的编码格式
 * @birth: Created by LGD on 2023-7-23
*/
enum _DataFormat operand_get_format(struct _operand op);

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
struct _operand operand_load_from_memory_to_spcified_register(struct _operand op,struct _operand dst);

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
 * @brief 获取操作数的寄存器类型
 * @birth: Created by LGD on 2023-7-18
*/
enum _ARMorVFP operand_get_regType(struct _operand op);



/**
 * @brief 封装直传函数，可以自行判断寄存器是什么类型
 * @param src 源寄存器
 * @param recycleSrc 是否回收源寄存器
 * @birth: Created by LGD on 20230201
*/
struct _operand operand_float_deliver(struct _operand src,bool recycleSrc);

/**
 * @brief 将浮点数从寄存器转换为整数
 * @birth: Created by LGD on 2023-7-19
 * @update: 2023-7-19 不再支持申请临时寄存器
*/
AssembleOperand operand_regFloat2Int(AssembleOperand src,struct _operand tar);
/**
 * @brief 将整数从寄存器转换为浮点数
 * @birth: Created by LGD on 2023-7-19
 * @update: 2023-7-19 不再支持申请临时寄存器
*/
AssembleOperand operand_regInt2Float(AssembleOperand src,struct _operand tar);

/**
 * @brief 封装转换函数，判断src的格式确定转换格式
 * @param src 源寄存器
 * @param tar 目标寄存器，如果留空，则选取临时寄存器，同时需要设置第三个参数确定寄存器类型
 * @param pickTempReg 是否申请一个临时寄存器，如果选否，需要为第四函数赋值，同时返回值将为nullop 否则赋nullop

 * @birth: Created by LGD on 20230201
*/
struct _operand operand_r2r_cvt(struct _operand src,struct _operand tar,...);

/**
 * @brief 将源转换格式后送入目标寄存器
 * @birth: Created by LGD on 2023-7-19
*/
struct _operand operand_load_to_reg_cvt(struct _operand src,struct _operand tar,...);
/**
 * @brief 将立即数用FLD伪指令读取到临时寄存器中，LDR / FLD通用
 * @birth: Created by LGD on 20230202
*/
struct _operand operand_load_immediate(struct _operand src,enum _ARMorVFP type);
/**
 * @brief 立即数读取到指令寄存器
 * @update:Created by LGD on 2023-4-11
 *         2023-7-18 删去不必要的参数type
*/
AssembleOperand operand_load_immediate_to_specified_register(AssembleOperand src,AssembleOperand dst);
/**
 * @brief 判断一个operand是否在指令中
 * @birth: Created by LGD on 20230328
*/
bool operand_is_immediate(struct _operand op);
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
* @brief 判断当前操作数是否未分配
* @brith: Created by LGD on 2023-7-9
*/
bool operand_is_unallocated(AssembleOperand op);

/**
 * @brief 判断一个操作数是否是合法立即数
 * @birth:Created by LGD on 2023-7-18
*/
bool operand_check_immed_valid(struct _operand op);
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

/**
 *@brief 这个方法可以更改操作数的寻址方式
 *@birth: Created by LGD on 2023-5-29
*/
void operand_change_addressing_mode(struct _operand* op,AddrMode addrMode);




#endif