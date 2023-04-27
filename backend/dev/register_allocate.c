#include "arm_assembly.h"
#include "sc_map.h"
#include "variable_map.h"
#include "cds.h"
#include "interface_zzq.h"
#include "memory_manager.h"
#include "register_allocate.h"
#include "value.h"


struct sc_map_64* active_interval_len_map;
size_t current_point = 0;
RegisterOrder common_reg[COMMON_REGISTER_NUM] = {R4,R5,R6,R7,R8};
RegisterOrder vps_common_reg[VPS_COMMON_REGISTER_NUM] = {
    S4,S5,S6,S7,S8,S9,S10,
    S11,S12,S13,S14,S15,S16,S17,S18,S19,S20,
    S21,S22,S23,S24,S25,S26,S27,S28,S29,S30,S31
};

Stack* common_reg_list;
Stack* vps_common_reg_list;

#define EMPTY -1

/**
 * @brief 初始化通用寄存器分配栈
 * @author Created by LGD on 20230107
*/
void init_arm_common_register()
{
    common_reg_list = StackInit();

    for(int i=0;i<COMMON_REGISTER_NUM;i++)
    {
        StackPush(common_reg_list,common_reg[i]);
    }
}

/**
 * @brief 选取一个空闲的通用寄存器，若仍有空闲的寄存器返回对应寄存器编号，若没有则返回EMPTY
 * @return 寄存器编号 或 EMPTY
 * @author Created by LGD on 20230108
*/
int Pick_a_free_arm_common_register()
{
    void* reg;
    bool not_empty;
    not_empty = StackTop(common_reg_list,&reg);
        
    if(not_empty)
    {
        StackPop(common_reg_list);
        return (int)reg;
    }
    else
        return EMPTY;
}

/**
 * @brief 收回一个通用寄存器到空闲列表中
 * @param reg 通用寄存器编号
*/
void recycle_a_arm_common_register(int reg)
{
    StackPush(common_reg_list,(void*)reg);
}


/**
 * @brief   浮点通用寄存器三件套
 * @author  Created by LGD on 20230112
*/
void init_vfp_common_register()
{
    vps_common_reg_list = StackInit();

    for(int i=0;i<VPS_COMMON_REGISTER_NUM;i++)
    {
        StackPush(vps_common_reg_list,vps_common_reg[i]);
    }
}

RegisterOrder Pick_a_free_vfp_common_register()
{
    void* reg;
    if(StackTop(common_reg_list,&reg))
    {
        StackPop(common_reg_list);
        return (RegisterOrder)reg;
    }
    else
        return EMPTY;
}

void recycle_a_vfp_common_register(RegisterOrder reg)
{
    StackPush(vps_common_reg_list,(void*)reg);
}

RegisterOrder pick_a_common_register(ARMorVFP type)
{
    switch(type)
    {
        case ARM:
            return Pick_a_free_arm_common_register();
        case VFP:
            return Pick_a_free_vfp_common_register();
    }
}

void recycle_a_common_register(RegisterOrder reg)
{
    switch(register_type(reg))
    {
        case ARM:
            return recycle_a_arm_common_register(reg);
        case VFP:
            return recycle_a_vfp_common_register(reg);
    }
}

/**
 * @brief 初始化活跃区间长度哈希表
 * @author LGD
 * @date 20230109
*/
void init_active_interval_len_map()
{
    active_interval_len_map = (struct sc_map_64*)malloc(sizeof(struct sc_map_64));
    sc_map_init_64(active_interval_len_map,0,0);
}

/**
 * @brief 依据给定函数单位生成各个变量的活跃区间长度表
 * @author LGD
 * @date 20230109
*/
void create_active_interval_len_map(List* this,int order)
{
    Instruction* cur = traverse_to_specified_function(this,order);
    Instruction* next;
    size_t cnt = 0;
    next = get_next_instruction(this);
    do{
        Value* key;
        uint64_t length;
        ++cnt;
        HashMap_foreach(cur->map,key,length)
        {
            if(((VarInfo*)variable_map_get_value(next->map,key))->isLive == false &&
               ((VarInfo*)variable_map_get_value(cur->map,key))->isLive == true)
                    sc_map_put_64(active_interval_len_map,key,cnt);
        }
        cur = next;
    }while((next = get_next_instruction(this)) != NULL && ins_get_opCode(next)!=FuncLabelOP);


}

size_t get_active_interval_len(Value* op)
{
    return sc_map_get_64(active_interval_len_map,op) - current_point;
}



/**
 * @brief 判断一个寄存器是否是通用寄存器
 * @author Created by LGD on 20220109
 * @update 20230112 增加了浮点寄存器的判定
*/
bool is_a_common_reg(RegisterOrder reg)
{
    for(int i=0;i<COMMON_REGISTER_NUM;i++)
        if(reg == common_reg[i])
            return true;
    for(int i=0;i<VPS_COMMON_REGISTER_NUM;i++)
        if(reg == vps_common_reg[i])
            return true;
    return false;
}


/**
 * @brief   将所有活跃信息为false的变量都调整为“未分配”，在寄存器中的变量将其寄存器回收
 * @author  Created by LGD on 20230109
 * @update  20230111 添加在内存中的变量回收内存的判定
 *          20230305 根据VarInfo新的设计修改
*/
void expire_old_interval(Instruction* this)
{
    Value* var;
    HashMapFirst(this->map);
    while((var = traverse_variable_map_get_key(this->map))!=NULL)
    {
        if(!variable_is_live(this,var))
        {
            if(get_variable_place(this,var) == IN_REGISTER && is_a_common_reg(get_variable_register_order(this,var)))
            {
                recycle_a_common_register(get_variable_register_order(this,var));
            }
            else if(get_variable_place(this,var) == IN_MEMORY)
            {
                return_a_memoryUnit(get_variable_stack_offset(this,var));
            }
            reset_var_info(variable_map_get_value(this->map,var));
        }
    }
}


/**
 * @brief 选取一块未被使用的栈区给变量
*/
size_t liner_scan_reg_allocation_pick_memory(Value* op)
{
    return pick_a_free_memoryUnit();
}

/**
 * @brief 按照存储位置类型获取所有变量中活跃区间最长的
 * @param type 可选项包括：IN_MEMORY IN_REGISTER UNALLOCATED，如果要多选，将选项或运算后作为参数 
 * @author Created by LGD on 20220106
 * @update 20230112 增加了对浮点变量的支持
 *         20230305 根据VarInfo新的设计修改
*/
Value* find_max_len(Instruction* this,RegorMem place,TypeID type)
{
    Value* key;
    VarInfo* value;
    Value* max_len_var = NULL;
    size_t max_len = 0;
    HashMap_foreach(this->map,key,value)
    {
        if(((get_variable_place(this,key)&place) == get_variable_place(this,key)) && 
             variable_is_live(this,key)                                           && 
             value_get_type(key) == type)
        {
            if(get_active_interval_len(key)>max_len)
            {
                max_len = get_active_interval_len(key);
                max_len_var = key;
            }

        }
    }
    return max_len_var;
}

/**
 * @brief 按照存储位置类型获取所有变量中活跃区间最短的
 * @param type 可选项包括：IN_MEMORY IN_REGISTER UNALLOCATED，如果要多选，将选项或运算后作为参数 
 * @author Created by LGD on 20220106
 * @update 20230112 增加了数据类型的判定
 *         20230305 根据VarInfo新的设计修改
*/
Value* find_min_len(Instruction* this,RegorMem place,TypeID type)
{
    Value* key;
    VarInfo* value;
    Value* min_len_var = NULL;
    size_t min_len = 999;
    size_t len;
    HashMap_foreach(this->map,key,value)
    {
        if((get_variable_place(this,key)&place) == get_variable_place(this,key) && 
            variable_is_live(this,key) && 
            value_get_type(key) == type)
        {
            if((len = get_active_interval_len(key))<min_len)
            {
                min_len = get_active_interval_len(key);
                min_len_var = key;
            }
        }
    }
    return min_len_var;
}

/**
 * @brief 将所有的空闲通用寄存器分配给指定存储类型的变量中活跃区间最短的
 * @param place 可选项包括：IN_MEMORY IN_REGISTER UNALLOCATED，如果要多选，将选项或运算后作为参数 
 * @author Created by LGD on 20220108
 * @update 20230112 增加了对浮点数变量的支持
 *         20230305 根据VarInfo新的设计修改
*/
void allocate_all_free_register_for_shortest(Instruction* this,RegorMem place)
{
    int reg;
    Value* var;
    VarInfo* space;
    while((var = find_min_len(this,place,IntegerTyID)) != NULL)
    {
        if((reg = Pick_a_free_arm_common_register())== -1)return;
        space = variable_map_get_value(this->map,var);

        set_variable_place(this,var,IN_REGISTER);
        ins_set_variable_register_order(this,var,reg);
    }
    while((var = find_min_len(this,place,FloatTyID)) != NULL)
    {
        if((reg = Pick_a_free_vfp_common_register())== -1)return;
        space = variable_map_get_value(this->map,var);

        set_variable_place(this,var,IN_REGISTER);
        ins_set_variable_register_order(this,var,reg);
    }
}

/**
 * @brief 返回指定类型的变量在当前指令中的数量
 * @param type 类型可选 IN_MEMORY IN_REGISTER UNALLOCATED
 * @author Created by LGD on 20230111
 * @update: 20230305 根据VarInfo新的设计修改
*/
size_t cnt_specified_interval_number(Instruction* this,RegorMem type)
{
    Value* var;
    VarInfo* var_info;
    size_t cnt = 0;
    HashMap_foreach(this->map,var,var_info)
    {
        if((get_variable_place(this,var)&type) ==get_variable_place(this,var))
            ++cnt;
    }
}


/**
 * @brief 返回活跃/不活跃变量在当前指令中的数量
 * @author Created by LGD on 20230111
 * @update: 20230305 根据VarInfo新的设计修改
*/
size_t cnt_live_variable_number(Instruction* this,bool is_live)
{
    Value* var;
    VarInfo* var_info;
    size_t cnt = 0;
    HashMap_foreach(this->map,var,var_info)
    {
        if(variable_is_live(this,var) == is_live)
            ++cnt;
    }
    return cnt;
}

//计算公式：        active_number = spilled_number + allocated_in_reg_number

/**
 * @brief 对代码执行线性扫描寄存器分配算法
 * @author Created by LGD on 20220109
 * @update 20230112 支持新的浮点数据
 *         20230305 明确用set_variable_register_order来分配寄存器
*/
void LinearScanregisterAllocation(Instruction* this)
{
    Value* key;
    VarInfo* value;

    //Step 1: 回收本次指令所有结束的活跃区间的寄存器
    expire_old_interval(this);

    //Step 2: 将内存和未分配的变量中最小的依次放入空闲的通用寄存器中
    allocate_all_free_register_for_shortest(this,IN_MEMORY | UNALLOCATED);

    //Step 3: 找出寄存器中最大的和未分配中最小的比较
    for(;;)
    {
        Value* longest_var_in_reg = find_max_len(this,IN_REGISTER,IntegerTyID);
        Value* shortest_var_unallocated = find_min_len(this,UNALLOCATED,IntegerTyID);
        if(longest_var_in_reg == NULL)break;
        if(shortest_var_unallocated == NULL)break;
        if(get_active_interval_len(longest_var_in_reg) > get_active_interval_len(shortest_var_unallocated))
        {
            set_variable_place(this,longest_var_in_reg,UNALLOCATED);
            set_variable_place(this,shortest_var_unallocated,IN_REGISTER);
            ins_set_variable_register_order(this,shortest_var_unallocated,get_variable_register_order(this,longest_var_in_reg));
        }
        else
            break;
    }

    for(;;)
    {
        Value* longest_var_in_reg = find_max_len(this,IN_REGISTER,FloatTyID);
        Value* shortest_var_unallocated = find_min_len(this,UNALLOCATED,FloatTyID);
        if(longest_var_in_reg == NULL)break;
        if(shortest_var_unallocated == NULL)break;
        if(get_active_interval_len(longest_var_in_reg) > get_active_interval_len(shortest_var_unallocated))
        {
            set_variable_place(this,longest_var_in_reg,UNALLOCATED);
            set_variable_place(this,shortest_var_unallocated,IN_REGISTER);
            ins_set_variable_register_order(this,shortest_var_unallocated,get_variable_register_order(this,longest_var_in_reg));
        }
        else
            break;
    }

    //Step 4: 若仍有未分配的变量，全部溢出到内存
    size_t offset;
    HashMap_foreach(this->map,key,value)
    {
        if(variable_is_live(this,key) && get_variable_place(this,key) == UNALLOCATED)
        {
            offset = liner_scan_reg_allocation_pick_memory(key);
            set_variable_place(this,key,IN_MEMORY);
            ins_set_variable_stack_offset(this,key,offset);
        }
    }
}

/**
 * @brief 线性扫描寄存器分配单元，这会对整个函数单元执行寄存器分配
 * @update 20230110 添加判定，下一条指令是funclabelOp则不拷贝
 *         20230112 添加对浮点数通用寄存器的支持
*/
void liner_scan_allocation_unit(List* this,int order)
{
    Instruction* cur_ins,*next_ins;
    //初始化通用寄存器堆栈
    init_arm_common_register();
    init_vfp_common_register();

    cur_ins = traverse_to_specified_function(this,order);

    do{
        //为当前指令做线性扫描法
        LinearScanregisterAllocation(cur_ins);
        //获取下一条指令
        next_ins = get_next_instruction(this);
        if(ins_get_opCode(next_ins)==FuncLabelOP)break;
        //将当前指令的map的位置和偏移值拷贝给下一条指令
        ins_cpy_varinfo_space(next_ins,cur_ins->map);
        //轮到下一条指令
        cur_ins = next_ins;
    }while(cur_ins!=NULL && ins_get_opCode(cur_ins)!=FuncLabelOP);

}

/**
 * @brief   获取整个指令链表中活跃变量同时最多的情况 活跃变量的个数
 * @author  Created by LGD on 20230111
*/
size_t traverse_inst_list_and_count_the_largest_lived_variable(List* this,int order)
{
    Instruction* p;
    size_t largest = 0;
    size_t cur;
    Inst_List_foreach(this,p,order)
    {
        cur = cnt_live_variable_number(p,true);
        if(cur > largest)largest = cur;
    }
    return largest;
}

/**
 * @brief 包含  1.为list所有指令初始化map
 *              2.为list构造虚拟的活跃区间
 *              3.对list进行寄存器分配
*/
void reg_allocate(List* this)
{
    //1. 为所有list指令初始化map   所有的变量存入，并将位置信息初始化

    HashMap* map;
    //创建map
    size_t totalSize = traverse_list_and_count_total_size_of_var(this,0,&map);
    //将map赋给所有的指令
    traverse_list_and_set_map_for_each_instruction(this,map,0);

    //2.  TODO 构造虚拟的活跃区间信息

    //3. 进行寄存器分配算法
    liner_scan_allocation_unit(this,0);

    
}

/**
 * @brief   若两个指令变量的存储位置发生了变化，需要插入相应的汇编指令对变量位置进行改变
 * @author  Created by LGD on 20230112
*/
void update_variable_place(Instruction* last,Instruction* now)
{
    Value* var;
    VarInfo* var_last_info;
    VarInfo* var_cur_info;
    AssembleOperand opList[2];
    HashMap_foreach(last->map,var,var_last_info)
    {
        var_cur_info = variable_map_get_value(now,var);
        if(get_variable_place(last,var) == IN_REGISTER && get_variable_place(now,var) == IN_MEMORY)
        {
            //从寄存器溢出到内存中
            opList[0] = get_variable_origin_operand(last,var);
            opList[1] = get_variable_origin_operand(now,var);

            switch(value_get_type(var))
            {
                case IntegerTyID:
                    memory_access_instructions("STR",opList[0],opList[1],NONESUFFIX,false,NONELABEL);
                    break;
                case FloatTyID:
                    vfp_memory_access_instructions("FST",opList[0],opList[1],FloatTyID);
                    break;
            }
        }
        else if(get_variable_place(last,var) == IN_MEMORY && get_variable_place(now,var) == IN_REGISTER)
        {
            //从内存加载到寄存器中
            opList[0] = get_variable_origin_operand(last,var);
            opList[1] = get_variable_origin_operand(now,var);
            switch(value_get_type(var))
            {
                case IntegerTyID:
                    memory_access_instructions("LDR",opList[1],opList[0],NONESUFFIX,false,NONELABEL);
                    break;
                case FloatTyID:
                    vfp_memory_access_instructions("FLD",opList[1],opList[0],FloatTyID);
                    break;
            }
        }
    }
}

