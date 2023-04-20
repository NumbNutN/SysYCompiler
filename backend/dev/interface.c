void translate_object_code(List* this)
{
    // @brief: 这个程序概括了后端的全部过程
    // @birth: Created by LGD on 20221229

    //获取所有的函数个数
    size_t num = traverse_list_and_count_the_number_of_function(this);

    for(int i=0;i<num;i++)
    {
        //第一步，遍历一个函数的全部变量并统计栈帧总大小
        size_t stacksize = traverse_list_and_count_total_size_of_value(this,i);

        set_function_stackSize(stacksize);
        set_function_currentDistributeSize(0);

        //第二步，遍历一个函数的所有变量并为他们分配栈帧相对位置并存入map
        HashMap* map = traverse_list_and_get_all_value_into_map(this,i);

        //第三步，将这个map拷贝给该函数的所有指令(这是没有任何优化的处理)
        traverse_list_and_set_map_for_each_instruction(this,map,i);

        //第四步，将所有的指令翻译出来
        traverse_list_and_translate_all_instruction(this,i);

        //第五步，打印输出
        print_model();
    }

}

size_t get_function_stackSize()
{
    // @brief:获取当前函数的栈帧大小
    // @birth:Created by LGD on 20221229
    return stackSize;
}

size_t get_function_currentDistributeSize()
{
    // @brief:获取当前函数已分配的空间大小
    // @birth:Created by LGD on 20221229
    return currentDistributeSize;
}

HashMap* traverse_list_and_get_all_value_into_map(List* this,int order)
{
    // @brief:遍历一个代码块指定数量的指令，并根据操作码为出现的Value生成一个表
    // @param:order:第一个函数块，以FunctionBeginOp 作为入口的标志
    // @birth:Created by LGD on 20221218
    // @update:20221220:添加了对变量是否已经存在在变量哈希表中的判定
    //         20221224:将涉及函数赋值的赋值号左侧的变量视为左值
    HashMap* map;
    variable_map_init(&map);
    Instruction* p;
    
    ListFirst(this,false);
    for(size_t cnt = 0;cnt <= order;cnt++)
    {
        ListNext(this,&p);
        if(ins_get_opCode(p) == FuncLabelOP)
            cnt++;
    }

    Value* val;
    do
    {
        switch(ins_get_opCode(p))
        {
            case AddOP:
            case SubOP:
            case MulOP:
            case DivOP:
            case AssignOP:
            case CallWithReturnValueOP:
            case NotEqualOP:
            case EqualOP:
            case GreatEqualOP:
            case GreatThanOP:
            case LessEqualOP:
            case LessThanOP:
                val = ins_get_assign_left_value(p);
                void* isFound = variable_map_get_value(map,val);
                if(isFound)break; 
                VarInfo* space = set_in_memory(NULL);
                variable_map_insert_pair(map,val,space);
                break;
        }
    }while(ListNext(this,&p));
    return map;
}