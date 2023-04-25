#ifndef _CREATE_TEST_EXAMPLE_H
#define _CREATE_TEST_EXAMPLE_H

#include "cds.h"
#include "value.h"
/**
 * @brief 为变量设定活跃变量区间
 * @param start 以FuncLabelOP为起始0
*/
void create_active_interval_for_variable(List* this,Value* var,int order,int start,int end);

#endif