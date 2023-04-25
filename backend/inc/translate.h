#include "instruction.h"
#include "config.h"

void translate_add(Instruction* ins);

void translate_IR(Instruction* this);
//三地址指令翻译
void translate_binary_expression(Instruction* ins);
void translate_goto_instruction(Instruction* this);


//标号
void Label(char* label);


void translate_goto_instruction_test(Instruction* this);

/**
 * @brief 采用add + mov 的减法翻译
 * @birth: Created by LGD on 2023-4-24
*/
void translate_sub(Instruction* this);
void translate_binary_expression_test(Instruction* ins);


/**
 * @brief 翻译双目逻辑运算表达式重构版
*/
void translate_logical_binary_instruction_new(Instruction* this);

void translate_assign_instructions(Instruction* this);



/**************************************************************/
/*                          子程序调用                          */
/***************************************************************/
/**
 * @brief 翻译Label指令 zzq专用
 * @date 20220103
 * @author LGD
*/
void translate_label(Instruction* this);


/**
 * @brief 翻译函数Label指令 zzq专用
 * @author LGD
 * @date 20230109
*/
void translate_function_label(Instruction* this);

void translate_function_entrance(Instruction* this);
void translate_function_end(Instruction* this);
/**
 * @brief 翻译传递参数的指令
 * @author Created by LGD on 2023-3-16
*/
void translate_param_instructions(Instruction* this);

void translate_return_instructions(Instruction* this);

/**
 * @brief 无返回值调用
 * @birth: Created by LGD on 2023-3-16
*/
translate_call_instructions(Instruction* this);

/**
 * @brief 翻译调用函数并接收返回值的指令
 * @author Created by LGD on 20221224
 * @update: 20230316 添加了一些对进程调用状态的维护
 *          20230114 添加了对浮点数和隐式转换的支持
 *          20221225 封装后使函数更精简
 *          20221225 添加了将变量重新放回内存的支持
 *          20221224 翻译时接收返回值的变量调整为赋值号左侧的变量
 *          20221224 设计有误，不需要存储当前的函数入口   
*/
translate_call_with_return_value_instructions(Instruction* this);

/**
 * @brief 翻译前执行的初始化
 * @birth: Created by LGD on 2023-3-28
*/
void TranslateInit();

#ifdef LLVM_LOAD_AND_STORE_INSERTED

/**
 * @brief 翻译load指令
 * @birth: Created by LGD on 2023-3-10
*/
void translate_load_instruction(Instruction* this);
/**
 * @brief 翻译store指令
 * @birth: Created by LGD on 2023-3-10
*/
void translate_store_instruction(Instruction* this);



#endif