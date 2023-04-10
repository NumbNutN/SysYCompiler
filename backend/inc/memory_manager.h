#ifndef _MEMORY_MANAGER_H
#define _MEMORY_MANAGER_H

#include "cds.h"
#include "operand.h"

typedef struct _MemUnitInfo
{
    int baseAddr;
    bool isUsed;
} MemUnitInfo;

//记录当前函数的栈总容量
extern size_t stackSize;

//当前函数的寄存器映射表
extern HashMap* cur_register_mapping_map;
//当前函数的内存映射表
extern HashMap* cur_memory_mapping_map;


//定义了当前已被处理的参数个数
extern size_t passed_param_number;


/**********************************************/
/*                 内部调用                    */
/**********************************************/

/**
 * @brief   对一个变量信息表进行初始化
 * @author  Created by LGD on 20230110
*/
void vitual_register_map_init(HashMap** map);
/**
 * @brief   对一个虚拟内存映射表进行初始化
 * @author  Created by LGD on 20230110
*/
void vitual_Stack_Memory_map_init(HashMap** map);

/**
 * @brief 申请一块新的局部变量的内存单元
*/
int request_new_local_variable_memory_unit();

/**
 * @brief 使对栈帧指针的更改生效
 * @update: 2023-4-4 将栈顶相对偏移改为栈帧
*/
void update_fp_value();
/**
 * @biref 使对栈顶指针的更改生效
*/
void update_st_value();

/**********************************************/
/*                 外部调用                    */
/**********************************************/

/**
 * @brief 给定虚拟寄存器编号，若有映射，则返回映射的实际寄存器编号；
 *          若未作映射，则尝试申请一个新的寄存器编号
 *          若无法提供更多的寄存器，说明哪里出了问题，甩出错误
 * @birth: Created by LGD on 2023-3-12
*/
RegisterOrder get_virtual_register_mapping(HashMap* map,size_t ViOrder);

/**
 * @brief 给定虚拟内存位置，若有映射，则返回映射的实际内存相对栈帧偏移值；
 *          若未作映射，则尝试申请一个新的内存单元
 *          若无法提供更多的内存，说明哪里出了问题，甩出错误
 * @birth: Created by LGD on 2023-3-12
*/
int get_virtual_Stack_Memory_mapping(HashMap* map,size_t viMem);

/**
 * @brief 申请一块新的参数传递用栈内存单元
 * @birth: Created by LGD on 2023-3-14
*/
int request_new_parameter_stack_memory_unit();

/**
 * @brief 依据局部变量个数和传递参数个数确定两个指针的跳跃位置
 * @birth: Created by LGD on 2023-3-12
*/
void set_stack_frame_status(size_t param_num,size_t local_var_num);

/**
 * @brief 恢复函数的栈帧和栈顶
 * @birth: Created by LGD on 2023-4-4
*/
void reset_stack_frame_status();
















/**
 * @brief 初始化被管理的内存单元
 * @param align 对齐单位
*/
void init_memory_unit(size_t align);
/**
 * @brief 选取一块未使用的内存单元
*/
size_t pick_a_free_memoryUnit();
/**
 * @brief 回收一块内存单元
*/
void return_a_memoryUnit(size_t memAddr);

#endif