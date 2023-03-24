#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "instruction.h"

extern Value *return_val;

int ins_get_opCode(Instruction* this);
char* ins_get_label(Instruction* this);
char* ins_get_tarLabel(Instruction* this);
Value* ins_get_assign_left_value(Instruction* this);
struct _Value *ins_get_operand(Instruction* this,int i);
/**
 * @brief 获取操作数的常量值（当操作数确实是常数时）
 * @author Created by LGD on 20220106
*/
int op_get_constant(Value* op);
int64_t ins_get_constant(Instruction* this,int i);

/**
 * @brief 从zzq的return语句中获取op
 * @author Created by LGD on 20220106
*/
Value* get_op_from_return_instruction(Instruction* this);
/**
 * @brief 从zzq的param语句中获取op
 * @author Created by LGD on 20220106
*/
Value* get_op_from_param_instruction(Instruction* this);


size_t get_function_stackSize();
size_t get_function_currentDistributeSize();
void set_function_stackSize(size_t num);
void set_function_currentDistributeSize(size_t num);

size_t traverse_list_and_count_total_size_of_value(List* this,int order);
HashMap* traverse_list_and_get_all_value_into_map(List* this,int order);
void traverse_list_and_set_map_for_each_instruction(List* this,HashMap* map,int order);

/**
 * @brief 这个程序概括了后端的全部过程
 * @author LGD
 * @param (int)this  List结构体指针   
 * 
 */
void translate_object_code(List* this);

/**
 * @brief 优化了构建map的过程，旨在增加全部溢出到内存和寄存器分配代码的协调性
 * @author Created by LGD on 20230109
*/
void translate_object_code_new(List* this);

/**
 * @brief 遍历指令列表初始化
 * @author Created by LGD on 20230109
*/
void traverse_instruction_list_init(List* this);
/**
 * @brief 遍历指令列表并获取下一个指令
 * @author Created by LGD on 20230109
*/
Instruction* get_next_instruction(List* this);

char* from_tac_op_2_str(TAC_OP op);

/**
 * @brief 判断一个变量是不是return_value
*/
bool var_is_return_val(Value* var);
/**
 * @brief 判断一个标号是不是叫entry 这是为了跳过zzq设置的函数入口entry标号
 * @author Created by LGD on 20230109
*/
bool label_is_entry(Instruction* label_ins);
/**
 * @brief 遍历列表到指定编号的函数的FuncLabel位置
 * @author LGD
 * @date 20230109
*/
Instruction* traverse_to_specified_function(List* this,int order);

/**
 * @biref:遍历所有三地址并翻译
 * @update:20220103 对链表进行初始化
*/
size_t traverse_list_and_translate_all_instruction(List* this,int order);
#endif