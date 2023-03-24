#include <stdarg.h>  //变长参数函数所需的头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Ast.h"
#include "Pass.h"
#include "bblock.h"
#include "c_container_auxiliary.h"
#include "cds.h"
#include "symbol_table.h"

#include "interface_zzq.h"

#include "create_test_example.h"
#include "variable_map.h"
#include "register_allocate.h"
#include "sc_map.h"
#include "memory_manager.h"
#include "input.h"
#include "arm.h"
extern Value *return_val;
extern List *ins_list;
extern struct sc_map_64* active_interval_len_map;
extern size_t stackSize;

/**
 * @brief 一劳永逸的初始化一切
 * @author Created by LGD on 20230112
*/
void back_end_init_all()
{
    //初始化用于整型的临时寄存器
    Init_arm_tempReg();
    //初始化浮点临时寄存器
    Free_Vps_Register_Init();
    //初始化活跃区间长度表
    init_active_interval_len_map();
    //初始化汇编链表
    initDlist();
}



/**
 * @brief   后端部分
 *          前端未提供活跃区间
 *          无寄存器分配
 *          处理所有函数
 * @author Created by LGD on 20230112
*/
void back_end_no_active_no_register_allocation(List* this)
{
    //初始化用于整型的临时寄存器
    Init_arm_tempReg();
    //初始化浮点临时寄存器
    Free_Vps_Register_Init();
    //获取函数的个数
    size_t num = traverse_list_and_count_the_number_of_function(this);
    //初始化链表
    initDlist();  

    for(int i=0;i<num;i++)
    {
        HashMap* map;
        //第一步
        //遍历一个函数的全部变量并统计栈帧总大小  并生成包括所有变量的变量信息表
        size_t stacksize = traverse_list_and_count_total_size_of_var(this,i,&map);

        //设置函数的栈帧大小
        set_function_stackSize(stacksize);
        set_function_currentDistributeSize(0);

        //第二步，遍历一个函数的所有变量并为他们分配栈帧相对位置并存入map
        spilled_all_into_memory(map);

        //第三步，将这个map拷贝给该函数的所有指令(这是没有任何优化的处理)
        traverse_list_and_set_map_for_each_instruction(this,map,i);

        print_single_info_map(this,i,true);

        //第四步，将所有的指令翻译出来
        traverse_list_and_translate_all_instruction(this,i);

    }
    //第五步，打印输出
    //print_model();
}

/**
 * @brief   后端部分
 *          前端未提供活跃区间
 *          寄存器分配
 *          只处理一个函数
 * @author Created by LGD on 20230112
*/
void back_end_no_active_register_allocation(List* list)
{
    HashMap* map;
    Value* var;
    VarInfo* var_info;

    back_end_init_all();
    //将所有变量装填入变量信息表并将计算总栈区大小
    traverse_list_and_count_total_size_of_var(list,0,&map);

    //将变量信息表复制一份给所有指令
    traverse_list_and_set_map_for_each_instruction(list,map,0);

    //为所有变量构建活跃区间
    HashMap_foreach(map,var,var_info)
    {
        if(!strcmp(var->name,"a"))
            create_active_interval_for_variable(list,var,0,0,18);
        if(!strcmp(var->name,"b"))
            create_active_interval_for_variable(list,var,0,0,8);
        if(!strcmp(var->name,"c"))
            create_active_interval_for_variable(list,var,0,0,8);
        if(!strcmp(var->name,"d"))
            create_active_interval_for_variable(list,var,0,0,8);
        if(!strcmp(var->name,"e"))
            create_active_interval_for_variable(list,var,0,0,8);
        if(!strcmp(var->name,"temp1"))
            create_active_interval_for_variable(list,var,0,0,6);
        if(!strcmp(var->name,"temp2"))
            create_active_interval_for_variable(list,var,0,0,9);
        if(!strcmp(var->name,"temp3"))
            create_active_interval_for_variable(list,var,0,0,17);
        if(!strcmp(var->name,"temp4"))
            create_active_interval_for_variable(list,var,0,0,17);
        if(!strcmp(var->name,"temp5"))
            create_active_interval_for_variable(list,var,0,0,17);
        if(!strcmp(var->name,"temp6"))
            create_active_interval_for_variable(list,var,0,0,17);
        if(!strcmp(var->name,"temp7"))
            create_active_interval_for_variable(list,var,0,0,17);
        if(!strcmp(var->name,"temp8"))
            create_active_interval_for_variable(list,var,0,0,17);
        if(!strcmp(var->name,"temp9"))
            create_active_interval_for_variable(list,var,0,0,15);
    }
    print_list_info_map(list,0,false);

    //栈区大小应当为活跃变量数量最多的情况-通用寄存器个数
    stackSize = (traverse_inst_list_and_count_the_largest_lived_variable(list,0) - COMMON_REGISTER_NUM)*4;
    //初始化内存管理器
    init_memory_unit(4);

    //创建区间长度表
    create_active_interval_len_map(list,0);

    sc_map_foreach(active_interval_len_map,var,var_info)
    {
        printf("%s %d\n",var->name,var_info);
    }

    //对函数进行线性扫描
    liner_scan_allocation_unit(list,0);

    //打印线性扫描结果
    print_list_info_map(list,0,true);

    //遍历并翻译指令
    traverse_list_and_translate_all_instruction(list,0);

    //打印输出
    print_model();
}

/**
 * @brief 测试 无寄存器分配 的情况
*/
void test_origin()
{
    AllInit();

    //返回值  当一个变量的name为return_val 我们将其视为返回值
    return_val = (Value *)malloc(sizeof(Value));
    value_init(return_val);
    return_val->name = strdup("return_val");
    return_val->VTy->TID = DefaultTyID;

    //前端 词法分析和中间代码生成入口
    printf("开始遍历\n");
    parser(input2);
    printf("遍历结束\n\n");


    //中间代码格式化输出
    print_ins_pass(ins_list);
    printf("\n\n");


    //后端目标代码生成
    back_end_no_active_no_register_allocation(ins_list);

    ListDeinit(ins_list);

    printf("语句数量：%d\n",CntAssemble);
}


/**
 * @brief 测试线性扫描算法
*/
void test_spilled_interval()
{
    AllInit();


    return_val = (Value *)malloc(sizeof(Value));
    value_init(return_val);
    return_val->name = strdup("return_val");
    return_val->VTy->TID = DefaultTyID;


    //前端语法分析和生成中间代码入口：
    printf("开始遍历\n");
    parser(input4);
    printf("遍历结束\n\n");

  //--------------------------------------

    //中间代码格式化输出形式：
    print_ins_pass(ins_list);

    //后端目标代码生成入口：
    back_end_no_active_register_allocation(ins_list);

    //释放链表
    ListDeinit(ins_list);
}