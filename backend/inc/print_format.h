#ifndef _PRINT_FORMAT_H
#define _PRINT_FORMAT_H
#include "arm_assembly.h"

/**
 * @brief 对单个汇编语句节点的格式化输出
 * @birth: Created by LGD on 20230227
*/
void print_single_assembleNode(assmNode* p);

/**
 * @brief 打印单个数据段节点
 * @brith: Created by LGD on 2023-5-30
 * @update: 2023-7-20 跳过对NONE_CNT的内容的打印
 */
void print_single_data(struct _dataNode *node);

//打印输出
void print_model();

#endif