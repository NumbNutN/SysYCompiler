/**
 * @brief   [只用于后端，临时使用]在无寄存器的情况下，遍历每一个变量，并为其分配空间
 * @author  Created by LGD on 20221218
 * @todo    根据Value的数据类型安排等量的栈空间,目前暂定统一为4个字节
 * @update  Latest:20221229 为StackSize和currentdistributeSize的访问和设置抽象化
*/
VarInfo* set_in_memory(Value* var)
{
    set_function_currentDistributeSize(get_function_currentDistributeSize()+4);

    VarInfo* varInfo;
    varInfo = (VarInfo*)malloc(sizeof(VarInfo));
    varInfo->ori.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
    varInfo->ori.oprendVal = SP;
    varInfo->ori.addtion = get_function_stackSize() - get_function_currentDistributeSize();
    return varInfo;
}

/**
 * @brief 为一个变量分配空间，不产生新的VarSpace变量
 * @author Created by LGD on 20230109
*/
void set_in_memory_new(Value* var,VarInfo** var_info)
{
    set_function_currentDistributeSize(get_function_currentDistributeSize()+4);
    (*var_info)->ori.addrMode = REGISTER_INDIRECT_WITH_OFFSET;
    (*var_info)->ori.oprendVal = SP;
    (*var_info)->ori.addtion = get_function_stackSize() - get_function_currentDistributeSize();
}

/**
 * @brief 为一个函数的变量信息表map中所有的变量分配空间
 * @author Created by LGD on 20230109
*/
void spilled_all_into_memory(HashMap* map)
{    
    Value* var;
    VarInfo* var_info;
    HashMap_foreach(map,var,var_info)
    {
        VarInfo* space = set_in_memory(NULL);
        variable_map_insert_pair(map,var,space);
    }
}