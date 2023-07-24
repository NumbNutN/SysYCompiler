#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "Pass.h"
#include "config.h"

extern const int REGISTER_NUM;
typedef enum _LOCATION { ALLOC_R4 =1, ALLOC_R5,ALLOC_R6,ALLOC_R8,ALLOC_R9,ALLOC_R10,ALLOC_R11,MEMORY } LOCATION;
extern char *location_string[];



#include "instruction.h"

#define  Traverse_Instruction_Via_Function \
  for (int i = 0; i < self_cfg->node_num; i++) { \
    int iter_num = 0;   \
    while (iter_num <   \
           ListSize((self_cfg->node_set)[i]->bblock_head->inst_list)) { \
      Instruction *element = NULL;  \
      ListGetAt((self_cfg->node_set)[i]->bblock_head->inst_list, iter_num,  \
                &element);  


int ins_get_opCode(Instruction* this);
char* ins_get_label(Instruction* this);
char* ins_get_tarLabel(Instruction* this);

/**
 * @brief 获取条件调整语句的标号
 * @birth: Created by LGD on 2023-5-29
*/
char* ins_get_tarLabel_Conditional(Instruction* this,bool cond);

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



// size_t get_function_stackSize();
// size_t get_function_currentDistributeSize();
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


/**
 * @brief 翻译为局部数组分配地址空间的指令
 * @birth:Created by LGD on 2023-5-2
 * @update: 2023-5-22 如果操作数是形式参数，语句将调整数组的基址和FP的相对偏移
 *          2023-5-29 考虑了指针在内存的情况
 *          2023-7-20 全局变量作参数allocate时，无需矫正
*/
void translate_allocate_instruction(Instruction* this,HashMap* map);

/**
 * @brief 翻译FuncEndOp
 * @birth: Created by LGD on 2023-7-23
*/
void translate_funcEnd_instruction();


/**
 * @brief 判断一个标号是不是叫entry 这是为了跳过zzq设置的函数入口entry标号
 * @author Created by LGD on 20230109
*/
bool label_is_entry(Instruction* label_ins);

/**
 * @brief 判断一个Value是否是全局的
 * @birth: Created by LGD on 2023-7-20
*/
bool value_is_global(Value* var);

/**
 * @brief 判断当前变量是否是数组类型
 * @birth: Created by LGD on 2023-7-24
*/
bool value_is_array(Value* val);

/**
 * @brief 判断一个变量是否是浮点数
 * @author Created by LGD on 20230113
 * @update: 2023-7-20 如果在数组中，也可以判断其是否是浮点数
*/
bool value_is_float(Value* var);

/**
 * @brief 判断一个指针指向空间是否是浮点数据
 * @birth: Created by lGD on 2023-7-24
*/
bool value_mem_item_is_float(Value* var);

/**
 * @brief 返回指针类型Value元素的format
 * @birth: Created by LGD on 2023-7-24
*/
enum _DataFormat value_get_elemFormat(Value* var);

/**
 * @brief 通过中间代码Value的描述确定operand的format
 * @birth: Created by LGD on 20230226
*/
enum _DataFormat valueFindFormat(Value* var);

/**
 * @brief 将立即数Value的数值返回，返回结果永远为unsigned 64
 *          当为32位有/无符号整型时返回数值
 *          当为IEEE754 32位浮点数时以浮点记法的64位（高32位为0）返回
 * @birth: Created by LGD on 2023-7-22
**/
uint64_t value_getConstant(Value* val);

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

/**
@brief:翻译全局变量定义链表
@birth:Created by LGD on 2023-5-29
*/
void translate_global_variable_list(List* this);

/**
 * @brief 将当前zzq寄存器分配表转换为变量信息表
 * @param myMap 变量信息表，它并不是一个未初始化的表，而应当是一个已经存储了所有变量名的表
 * @birth: Created by LGD on 2023-3-26
*/
// HashMap* interface_cvt_zzq_register_allocate_map_to_variable_info_map(HashMap* zzqMap,HashMap* myMap);

/**
 * @brief 从指令中返回步长
 * @birth: Created by LGD on 2023-5-1
*/
int ins_getelementptr_get_step_long(Instruction* this);

/**
 * @brief 获取函数的参数个数
 * @birth: Created by LGD on 2023-7-17
*/
size_t func_get_param_numer(Function* func);


/**
 * @brief 每次翻译新函数前要执行的初始化
 * @birth:Created by LGD on 2023-5-9
*/
void InitBeforeFunction();


/**
 * @brief 获取局部变量域的大小
 * @birth: Created by LGD on 2023-7-17
**/
#ifdef COUNT_STACK_SIZE_VIA_TRAVERSAL_INS_LIST
size_t getLocalVariableSize(ALGraph* self_cfg);
#elif defined COUNT_STACK_SIZE_VIA_TRAVERSAL_MAP
size_t getLocalVariableSize(HashMap* varMap);
#endif

/**
 * @brief 将为所有数组分配空间并将基地址（绝对的）传递给它们的指针
 * @birth: Created by LGD on 2023-7-20
*/
void attribute_and_load_array_base_address(Function *handle_func,HashMap* map);

/**
 * @brief 为指令链安插变量信息表
 * @birth: Created by LGD on 2023-7-20
*/
void insert_variable_map(Function *handle_func,HashMap* map);

/**
 * @brief 设置当前所有参数的初始位置
 * @birth: Created by LGD on 2023-7-17
*/
void set_param_origin_place(HashMap* varMap,size_t param_number);


/**
 * @brief 打印一句中端代码信息
 * @birth: Created by LGD on 2023-7-16
*/
void print_ins(Instruction *element);
#endif