#ifndef REGISTER_ALLOCATE_H
#define REGISTER_ALLOCATE_H

#include "instruction.h"

#define COMMON_REGISTER_NUM 5
#define VPS_COMMON_REGISTER_NUM 28

/**
 * @brief 初始化活跃区间长度哈希表
 * @author LGD
 * @date 20230109
*/
void init_active_interval_len_map();
/**
 * @brief 初始化通用寄存器分配栈
 * @author Created by LGD on 20230107
*/
void init_common_register();
/**
 * @brief   浮点通用寄存器三件套
 * @author  Created by LGD on 20230112
*/
void init_vps_common_register();
/**
 * @brief 对代码执行线性扫描寄存器分配算法
 * @author Created by LGD on 20220109
*/
void LinearScanregisterAllocation(Instruction* this);
/**
 * @brief 线性扫描寄存器分配单元，这会对整个函数单元执行寄存器分配
*/
void liner_scan_allocation_unit(List* this,int order);
/**
 * @brief 依据给定函数单位生成各个变量的活跃区间长度表
 * @author LGD
 * @date 20230109
*/
void create_active_interval_len_map(List* this,int order);

/**
 * @brief   获取整个指令链表中活跃变量同时最多的情况 活跃变量的个数
 * @author  Created by LGD on 20230111
*/
size_t traverse_inst_list_and_count_the_largest_lived_variable(List* this,int order);

/**
 * @brief   若两个指令变量的存储位置发生了变化，需要插入相应的汇编指令对变量位置进行改变
 * @author  Created by LGD on 20230112
*/
void update_variable_place(Instruction* last,Instruction* now);
#endif