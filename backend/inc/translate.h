#include "instruction.h"
#include "config.h"

extern Instruction* currentInstruction;

enum _Instruction_Type{
    VALID_INSTRUCTION,
    INVALID_INSTRUCTION
};


/**
* @brief 翻译前的钩子，false表面当前指令的翻译应当终止
* @birth: Created by LGD on 2023-7-9
*/
bool check_before_translate(Instruction* this);

void translate_add(Instruction* ins);

void translate_IR(Instruction* this);
//三地址指令翻译
void translate_binary_expression(Instruction* ins);

/**
 * @brief 翻译所有的双目赋值运算,但是采用add + mov 的新方法
 * @birth: Created by LGD on 20230226
 * @update: 20230306 新的变量位置切换
 *          2023-4-10 添加除法运算
*/
void translate_binary_expression_binary_and_assign(Instruction* this);

void translate_goto_instruction(Instruction* this);

/**
 * @brief 翻译只包含一个bool变量作为条件的跳转指令
 * @author Created by LGD on 20221225
 * @update: 2023-5-29 重写goto_instruction
 *          2023-7-11 区分GotoOp和GotoOp_WithCond
*/
void translate_goto_instruction_test_bool(Instruction* this);

/**
 * @brief 翻译单目运算
 * @birth: Created by LGD on 2023-7-11
*/
void translate_unary_instructions(Instruction* this);

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
 * @birth: Created by LGD on 2023-4-18
 * @update: 2023-7-11 条件语句应该在无条件之下
 *          2023-7-16 重构 条件码的判断位置改变
*/
void translate_logical_binary_instruction_new(Instruction* this);

/**
 * @brief 翻译And Or运算
 * @birth: Created by LGD on 2023-7-16
**/
void translate_and_or_instructions(Instruction* this);

void translate_assign_instructions(Instruction* this);

/**
 * @brief 拆出数组的地址
 * @birth: Created by LGD on 2023-5-1
*/
void translate_getelementptr_instruction(Instruction* this);
/**
 * @brief 翻译将数据回存到一个指针指向的位置的指令
 * @birth: Created by LGD on 2023-5-3
*/
void translate_store_instruction(Instruction* this);
/**
 * @brief 翻译将数据从一个指针的指向位置取出
 * @birth: Created by LGD on 2023-5-4
*/
void translate_load_instruction(Instruction* this);


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
void translate_call_instructions(Instruction* this);

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
void translate_call_with_return_value_instructions(Instruction* this);

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

/**************************************************************/
/*                          全局变量                         */
/***************************************************************/

/**
 * @brief 翻译allocate语句
 * @birth: Created by LGD on 2023-7-16
**/
void translate_global_allocate_instruction(Instruction* this);

/**
 * @brief:翻译对全局变量的赋值
 * @birth:Created by LGD on 2023-5-29
*/
void translate_global_store_instruction(Instruction* this);