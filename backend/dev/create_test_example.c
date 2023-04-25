/**
 * @brief 对指定的变量创建活跃区间始末
*/
#include "create_test_example.h"
#include "interface_zzq.h"
#include "instruction.h"
#include "variable_map.h"
/**
 * @brief 为变量设定活跃变量区间
 * @param start 以FuncLabelOP为起始0
*/
void create_active_interval_for_variable(List* this,Value* var,int order,int start,int end)
{
    Instruction* p = traverse_to_specified_function(this,order);
    VarInfo* var_info;
    int cnt=0;

    do{
        if(start <= cnt && end > cnt)
        {
            var_info = variable_map_get_value(p->map,var);
            var_info->isLive = true;
        }
        p = get_next_instruction(this);
        ++cnt;
    } while(cnt <= end && p!=NULL && ins_get_opCode(p)!=FuncLabelOP);
}