#ifndef _VARIABLE_MAP_H
#define _VARIABLE_MAP_H

#include "instruction.h"
#include "bblock.h"
#include "cds.h"
#include "operand.h"
#include "config.h"
#include "arm.h"



/**
 * @update 20230306 重构了变量信息节点的内容组织方式
*/
typedef struct _VarInfo
{
    bool isLive;

    struct _operand ori;
    struct _operand current;
    
} VarInfo;

//MUL R0,R0, #2^n
//LSL R0,#n

//MOV R0,#0
//XOR R0,R0

//DO WHILE
//ADD I
//ADD II
// BNE I

//ADDNE I
//ADDNE I

//BL

//bool a = R4 <5
//if(a)
    //


/*===========================================================================*
 *                              Common Method                                *
 *===========================================================================*/

/**
 * @brief   对一个变量信息表进行初始化
 * @author  Created by LGD on 20230110
*/
void variable_map_init(HashMap** map);

/**
 * @brief   为变量状态表插入新的键值对，若键已存在则覆盖值
 * @author  Created by LGD on 20221220
*/
void variable_map_insert_pair(HashMap* map,Value* key,VarInfo* value);

/**
 * @brief   在指定变量状态表中根据指定键Value*获取值
 * @author  Created by LGD on 20230110
*/
VarInfo* variable_map_get_value(HashMap* map,Value* key);

/**
 * @brief 判断一个变量在当前指令中是否是活跃的
*/
bool variable_is_live(Instruction* this,Value* var);


/**
 * @brief   将指定的变量信息表赋给指令，这是浅拷贝
 * @author  Created by LGD on 20221218
*/
void ins_shallowSet_varMap(Instruction* this,HashMap* map);

/**
 * @brief   将指定的变量信息表赋给指令，这是深拷贝，会将map中所有的变量信息值内存上的拷贝
 * @author  Created by LGD on 20221218
*/
void ins_deepSet_varMap(Instruction* this,HashMap* map);

/**
 * @brief   遍历变量信息表一次并返回一个键
 * @author  Created by LGD on 20230110
 * @update: 2023-3-26 添加名字和Value*查表的选项
*/
#ifdef VARIABLE_MAP_BASE_ON_VALUE_ADDRESS 
Value*  
#elif defined VARIABLE_MAP_BASE_ON_NAME
char*   
#else 
#error "需要指定至少一种变量信息表的键形式"
#endif
traverse_variable_map_get_key(HashMap* map);


#ifdef VARIABLE_MAP_BASE_ON_NAME
/**
 * @brief 依据变量名设定变量在指定map的栈帧偏移
 * @birth: Created by LGD on 2023-3-26
*/
void set_variable_stack_offset_by_name(HashMap* map,char* name,size_t offset);
/**
 * @brief 在map中设置变量当前的寄存器编号
 * @birth: Created by LGD on 20230305
*/
void set_variable_register_order_by_name(HashMap* map,char* name,RegisterOrder reg_order);
#endif

/**
 * @brief   遍历变量信息表一次并返回一个键
 * @author  Created by LGD on 20230110
*/
VarInfo* traverse_variable_map_get_value(HashMap* map);

/**
 * @brief   打印一个函数的变量状态表
 * @param   order 函数的序号
 * @param   print_info 若为假，仅打印活跃信息；若为真，将打印变量的全部信息，包括存储空间和偏移量
*/
void print_list_info_map(List* this,int order,bool print_info);

/**
 * @brief  打印单个变量信息表
 * @birth: Created by LGD on 20230306
*/
void print_single_info_map(List* this,int order,bool print_info);



/**
 * @brief 提供了一个sc风格的宏的遍历方法
 *        使用举例：
 *          Traverse_Variable_Map_Init （在一个函数里只需调用一次）
 *          Value* k;VarInfo* v;
 *          HashMap_foreach(map,k,v)
 *          {
 *              ...
 *          }
*/

#define HashMap_foreach(map,k,v)  \
    HashMapFirst(map);                  \
    for(Pair* pair_ptr = HashMapNext(map);  \
        (pair_ptr != NULL) && (k = pair_ptr->key) && (v = pair_ptr->value);    \
        pair_ptr = HashMapNext(map))


#define Inst_List_foreach(list,p,order)   \
    p = traverse_to_specified_function(list,order); \
    while(ListNext(list,&p) && ins_get_opCode(p)!=FuncLabelOP)

// #define Inst_List_foreach(list,p,order)   \
//     for(p = traverse_to_specified_function(list,order);
//         p != NULL && )
//     while(ListNext(list,&p) && ins_get_opCode(p)!=FuncLabelOP)

/*===========================================================================*
 *                             Back End Use Part                             *
 *===========================================================================*/

/**
 * @brief 将变量信息表拷贝给当前指令，但是仅拷贝存储位置和偏移（不拷贝活跃信息）
 * @author Created by LGD on 20230109
*/
void ins_cpy_varinfo_space(Instruction* this,HashMap* map);



/**
 * @brief 获取操作数的位置，如果是变量将从对应指令变量信息表中查表，如果是常数直接返回IN_INSTRUCTION
 * @author Created by LGD on 20220106
*/
RegorMem get_variable_place(Instruction* this,Value* op);
/**
 * @brief 返回对应变量的位置枚举值（指令、寄存器或内存）
 * @author Created by LGD on ??
*/ 
RegorMem get_variable_place_by_order(Instruction* this,int i);


/**
 * @brief   根据变量状态表获取寄存器编号
 * @author  Created by LGD on 20230305
*/
size_t get_variable_register_order(Instruction* this,Value* var);
/**
 * @brief   根据变量状态表获取寄存器编号
 * @author  Created by LGD on 20230305
*/
size_t get_variable_register_order_by_order(Instruction* this,int i);


/**
 * @brief   根据变量状态表获取相对栈帧偏移
 * @author  Created by LGD on 20230305
*/
size_t get_variable_stack_offset(Instruction* this,Value* var);
/**
 * @brief   根据变量状态表获取相对栈帧偏移
 * @author  Created by LGD on 20230305
*/
size_t get_variable_stack_offset_by_order(Instruction* this,int i);


/**
 * @brief   根据变量状态表获取内存偏移值或寄存器编号
 * @author  Created by LGD on 20221214
*/
size_t get_variable_register_order_or_memory_offset_test(Instruction* this,Value* var);




/**
 * @brief 设置变量在当前指令的变量状态表中的位置
 * @author Created by LGD on 20230108
*/
void set_variable_place(Instruction* this,Value* var,RegorMem place);
/**
 * @brief 设置变量在当前指令的变量状态表中的具体的偏移值
 * @author Created by LGD on 20230108
*/
void set_variable_register_order_or_memory_offset_test(Instruction* this,Value* var,int order);
/**
 * @brief 设置变量当前的寄存器编号
 * @birth: Created by LGD on 20230305
*/
void ins_set_variable_register_order_by_order(Instruction* this,int i,RegisterOrder reg_order);
/**
 * @brief 设置变量当前的寄存器编号
 * @birth: Created by LGD on 20230305
*/
void ins_set_variable_register_order(Instruction* this,Value* var,RegisterOrder reg_order);
/**
 * @brief 设置变量在当前指令的栈帧偏移
 * @birth: Created by LGD on 20230305
*/
void ins_set_variable_stack_offset(Instruction* this,Value* var,size_t offset);
/**
 * @brief 设置变量在指定map的栈帧偏移
 * @birth: Created by LGD on 2023-3-26
*/
void set_variable_stack_offset(HashMap* map,Value* var,size_t offset);
/**
 * @brief 设置变量在当前指令的栈帧偏移
 * @birth: Created by LGD on 20230305
*/
void ins_set_variable_stack_offset_by_order(Instruction* this,int i,size_t offset);
/**
 * @brief 获取变量原始存放的操作数单元
 * @birth: Created by LGD on 20230305
*/
AssembleOperand get_variable_origin_operand(Instruction* this,Value* var);
/**
 * @brief 获取变量目前存放的操作数单元
 * @birth: Created by LGD on 20230305
*/
AssembleOperand get_variable_current_operand(Instruction* this,Value* var);

/**
 * @brief 重置变量的VarSpace信息
 * @author Created by LGD on 20230109
*/
void reset_var_info(VarInfo* var_info);
/**
 * @brief 重置变量的VarSpace信息
 * @author Created by LGD on 20230109
*/
void ins_reset_var_info(Instruction* this,Value* var);
/**
 * @brief 根据指定的序号遍历对应的函数，将变量左值全部装入变量信息表，并初始化其信息，同时计算变量的总大小
 * @author Created by LGD on 20230109
 * @param this 指令链表
 * @param order 要遍历第几个函数，以FuncLabelOP作为分割依据
 * @param map 传入一个map的指针，填装满的map将返回
 * @return 计算的栈帧大小
*/
size_t traverse_list_and_count_total_size_of_var(List* this,int order);
/**
 * @brief 整合了开辟栈空间和寄存器分配
 * @birth: Created by LGD on 2023-5-3
*/
HashMap* traverse_list_and_allocate_for_variable(List* this,HashMap* zzqMap,HashMap** myMap);
/**
 * @brief 为一个函数的变量信息表map中所有的变量分配空间
 * @author Created by LGD on 20230109
*/
void spilled_all_into_memory(HashMap* map);

/**
 * @brief 将存储位置枚举类型转化为字符串
 * @author LGD
 * @date 20230109
*/
char* RegOrMem_2_str(RegorMem place);

/**
 * @brief 由于仅通过operand判断需不需要临时寄存器需要额外的归类方法
 * @birth: Created by LGD on 20230130
*/
RegorMem judge_operand_in_RegOrMem(AssembleOperand op);

/**
 * @brief   [只用于后端，临时使用]在无寄存器的情况下，遍历每一个变量，并为其分配空间
 * @author  Created by LGD on 20221218
 * @todo    根据Value的数据类型安排等量的栈空间,目前暂定统一为4个字节
*/
VarInfo* set_in_memory(Value* var);

/**
 * @brief 将寻址方式转换为字符串
*/
char* AddrMode_2_str(enum _AddrMode addrMode);

extern char *op_string[];
/**记录当前函数的栈总容量*/
extern size_t stackSize;
/**记录当前已经分配了多少容量*/
extern size_t currentDistributeSize;

#endif