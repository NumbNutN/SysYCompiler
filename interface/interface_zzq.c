#include "interface_zzq.h"
#include "instruction.h"
#include "variable_map.h"
#include "arm.h"
#include "arm_assembly.h"
#include "dependency.h"
#include <assert.h>
#include "config.h"
#include "translate.h"
#include "memory_manager.h"

#include "Pass.h"

extern bool Open_Register_Allocation;

int ins_get_opCode(Instruction* this)
{
    // @brief:获取指令的操作码，由zzq的font-end暴露的接口
    // @birth:Created by LGD on 20221228
    return this->opcode;
}

/**
 * @brief 获取Label指令的标签，由zzq的font-end暴露的接口
 * @birth:Created by LGD on 20221228
 * @update: 2023-3-28 调用ins_get_left_value基本方法
*/
char* ins_get_label(Instruction* this)
{

    assert((this->opcode == LabelOP || this->opcode == FuncLabelOP) && "非标签语句");
    return ins_get_assign_left_value(this)->name;
}

/**
 * @brief 获取目标跳转指令，适用于GotoOP,GotoWithConditionOP,CallOP,CallWithReturnValueOP
 * @birth: Created by LGD on 20221229
 * @update: 2023-3-28 调用ins_get_left_value基本方法
*/
char* ins_get_tarLabel(Instruction* this)
{
    assert(this->opcode != GotoOP || this->opcode != GotoWithConditionOP || this->opcode != CallOP || this->opcode != CallWithReturnValueOP);
    switch(ins_get_opCode(this))
    {
        case GotoOP:
        case CallOP:
        case CallWithReturnValueOP:
            return ins_get_assign_left_value(this)->pdata->no_condition_goto.goto_location->name;
        case GotoWithConditionOP:
            return ins_get_assign_left_value(this)->pdata->condition_goto.false_goto_location->name;   //条件跳转时，只需要跳转至正确，错误顺序执行
    }
}

/**
 * @brief 获取赋值号左值
 * @birth: Created by LGD on 20221229
 * @update: 2023-3-26 左值的位置可以条件编译
*/
Value* ins_get_assign_left_value(Instruction* this)
{
#ifdef LEFT_VALUE_IS_OUTSIDE_INSTRUCTION
    return this->user.res;
#elif defined LEFT_VALUE_IS_IN_INSTRUCTION
    return &this->user.value;
#endif
}


/**
 * @brief 按下标取instruction的操作数
 * @birth: Created by LGD on 20221229
 * @update: 2023-3-28 使用get_left_value的基本方法
*/
struct _Value *ins_get_operand(Instruction* this,int i){

    if(i)
    {
        return user_get_operand_use(&(this->user),i-1)->Val;
    }
    else
    {
        return ins_get_assign_left_value(this);
    }
}

/**
 * @brief   当操作数为常数时，获取指令的操作数的常数值
 * @param   [int i]: 1：第一操作数  2：第二操作数
 * @notice: 不包括目标操作数
 * @birth:  Created by LGD on 20221229,由zzq的font-end暴露的接口
 * @update: 20230122 添加了对浮点数转换为IEEE754 32位浮点数格式的支持
 *          20221225 调用ins_get_operand接口代替user_get_operand_use  第一操作数由0变为1，以此类推
*/
int64_t ins_get_constant(Instruction* this,int i)
{
    if(value_get_type(ins_get_operand(this,i)) == ImmediateIntTyID )
        return ins_get_operand(this,i)->pdata->var_pdata.iVal;

    else if(value_get_type(ins_get_operand(this,i)) ==  ImmediateFloatTyID)
        return (int64_t)float_754_binary_code(ins_get_operand(this,i)->pdata->var_pdata.fVal,BITS_32);
}

/**
 * @brief 获取操作数的常量值（当操作数确实是常数时）
 * @author Created by LGD on 20220106
*/
int op_get_constant(Value* op)
{
    return op->pdata->var_pdata.iVal;
}

#ifdef OPEN_FUNCTION_WITH_RETURN_VALUE
/**
 * @brief 从zzq的return语句中获取op
 * @author Created by LGD on 20220106
*/
Value* get_op_from_return_instruction(Instruction* this)
{
    return this->user.value.pdata->return_pdata.return_value;
}
#endif
/**
 * @brief 从zzq的param语句中获取op
 * @author Created by LGD on 20220106
*/
Value* get_op_from_param_instruction(Instruction* this)
{
    return this->user.value.pdata->param_pdata.param_value;
}



void translate_IR_test(struct _Instruction* this)
{
    // @brief:这个方法通过判断Instruction的指令来执行对应的翻译方法
    // @birth:Created by LGD on 20221224
    // @update:Latest 20221224:添加对比较运算符的翻译支持
    switch(ins_get_opCode(this))
    {
#ifdef OPEN_TRANSLATE_BINARY
        case AddOP:
        case SubOP:
        case MulOP:
        case DivOP:
    #ifdef OPEN_LOAD_AND_STORE_BINARY_OPERATE
            translate_binary_expression_test(this);
    #else   
            translate_binary_expression_binary_and_assign(this);
    #endif
            break;
#endif
#ifdef OPEN_TRANSLATE_LOGICAL
        case EqualOP:
        case NotEqualOP:
        case GreatEqualOP:
        case GreatThanOP:
        case LessEqualOP:
        case LessThanOP:
            translate_logical_binary_instruction_new(this);
            break;
#endif
        // case Goto:
        // case Goto_Equal:
        // case Goto_GreatEqual:
        // case Goto_GreatThan:
        // case Goto_LessEqual:
        // case Goto_LessThan:
        // case Goto_NotEqual:
        //     translate_goto_instruction_conditions_in_one(this);
            // break;
        case GotoWithConditionOP:
            translate_goto_instruction_test_bool(this);
        break;
        case FuncLabelOP:
            translate_function_entrance(this);
        break;
        // case ReturnOP:
        //     translate_return_instructions(this);
        // break;
        case ParamOP:
            translate_param_instructions(this);
        break;
        case FuncEndOP:
            translate_function_end(this);
        break;
        case CallOP:
            translate_call_instructions(this);
        break;
        case CallWithReturnValueOP:
            translate_call_with_return_value_instructions(this);
        break;
        case AssignOP:
            translate_assign_instructions(this);
        break;
        case LabelOP:
            translate_label(this);
        break;

    }
}

#ifdef OPEN_REGISTER_ALLOCATION
/**
 * @brief 这个方法通过判断Instruction的指令来执行对应的翻译方法
 *        在寄存器分配开启的情况下
 * @author Created by LGD on 20230112
*/
void translate_IR_open_register_allocation(struct _Instruction* this,struct _Instruction* last)
{
    update_variable_place(last,this);
    switch(ins_get_opCode(this))
    {
        case AddOP:
        case SubOP:
        case MulOP:
        case DivOP:
#ifdef OPEN_LOAD_AND_STORE_BINARY_OPERATE
            translate_binary_expression_test(this);
#endif
            break;
        case EqualOP:
        case NotEqualOP:
        case GreatEqualOP:
        case GreatThanOP:
        case LessEqualOP:
        case LessThanOP:
            translate_logical_binary_instruction(this);
            break;
        case GotoWithConditionOP:
            translate_goto_instruction_test_bool(this);
            break;
        case FuncLabelOP:
            translate_function_entrance(this);
            break;
#ifdef OPEN_FUNCTION_WITH_RETURN_VALUE
        case ReturnOP:
            translate_return_instructions(this);
        break;
#endif
        case FuncEndOP:
            translate_function_end(this);
            break;
        case CallOP:
            translate_call_instructions(this);
            break;
        case CallWithReturnValueOP:
            translate_call_with_return_value_instructions(this);
            break;
        case AssignOP:
            translate_assign_instructions(this);
            break;
        case LabelOP:
            translate_label(this);

    }
}
#endif


size_t traverse_list_and_count_total_size_of_value(List* this,int order)
{
    // @brief:遍历一个代码块指定数量的指令，并根据操作码为出现的Value生成一个表
    // @param:order:第一个函数块，以FunctionBeginOp 作为入口的标志
    // @birth:Created by LGD on 20221218
    // @update:20221220:添加了对变量是否已经存在在变量哈希表中的判定
    //         20221224:将涉及函数赋值的赋值号左侧的变量视为左值
    Instruction* p;
    size_t totalSize = 0;
    
    ListFirst(this,false);
    for(size_t cnt = 0;cnt <= order;)
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
                totalSize += 4;
                break;
        }
    }while(ListNext(this,&p));
    return totalSize;
}

   
/**
 * @brief 遍历一个代码块指定数量的指令，并根据操作码为出现的Value生成一个表
 * @param order 第n个函数块，以FuncLabelOP 作为入口的标志
 * @author LGD
 * @date 20221218
 * @update  20220109 对遍历至指定函数入口的方法进行了封装调用
 *          20221220:添加了对变量是否已经存在在变量哈希表中的判定
 *          20221224:将涉及函数赋值的赋值号左侧的变量视为左值
*/
void traverse_list_and_set_map_for_each_instruction(List* this,HashMap* map,int order)
{
    
    Instruction* p;
    
    p = traverse_to_specified_function(this,order);

    do
    {
        ins_deepSet_varMap(p,map);
    }while(ListNext(this,&p) && ins_get_opCode(p)!=FuncLabelOP);

}

size_t traverse_list_and_count_the_number_of_function(List* this)
{
    // @brief:遍历一个代码块指定数量的指令，并根据操作码为出现的Value生成一个表
    // @param:order:第一个函数块，以FunctionBeginOp 作为入口的标志
    // @birth:Created by LGD on 20221218
    // @update:20221220:添加了对变量是否已经存在在变量哈希表中的判定
    //         20221224:将涉及函数赋值的赋值号左侧的变量视为左值
    Instruction* p;
    size_t cnt = 0;
    
    ListFirst(this,false);
    while(ListNext(this,&p))
    {
        if(ins_get_opCode(p) == FuncLabelOP)
            cnt++;
    }
    return cnt;
}

/**
 * @biref:遍历所有三地址并翻译
 * @update:20220103 对链表进行初始化
*/
size_t traverse_list_and_translate_all_instruction(List* this,int order)
{
    Instruction* p;
    p = traverse_to_specified_function(this,order);

    do
    {
        translate_IR_test(p);
    }while(ListNext(this,&p) && ins_get_opCode(p)!=FuncLabelOP);

}


void set_function_stackSize(size_t num)
{
    // @brief:获取当前函数的栈帧大小
    // @birth:Created by LGD on 20221229
    stackSize = num;
}

void set_function_currentDistributeSize(size_t num)
{
    // @brief:获取当前函数已分配的空间大小
    // @birth:Created by LGD on 20221229
    currentDistributeSize = num;
}

/**
 * @brief 遍历指令列表初始化
 * @author Created by LGD on 20230109
*/
void traverse_instruction_list_init(List* this)
{
    ListFirst(this,false);
}

/**
 * @brief 遍历指令列表并获取下一个指令
 * @author Created by LGD on 20230109
 * @update 20230109 现在当遍历到末尾时返回NULL
*/
Instruction* get_next_instruction(List* this)
{
    Instruction* p;
    if(ListNext(this,&p)==false)return NULL;
    return p;
}


/**
 * @brief 判断一个变量是不是return_value，这是为了处理zzq引入的return_val记号
 * @author Created by LGD on 20230109
*/
bool var_is_return_val(Value* var)
{
    return var == return_val;
}

/**
 * @brief 判断一个标号是不是叫entry 这是为了跳过zzq设置的函数入口entry标号
 * @author Created by LGD on 20230109
 * @update: 2023-3-28 使用get_left_value基本方法
*/
bool label_is_entry(Instruction* label_ins)
{
    return !strcmp(ins_get_assign_left_value(label_ins)->name,"entry");
}


/**
 * @brief 遍历列表到指定编号的函数的FuncLabel位置
 * @return 返回当前遍历到的函数入口的指令地址
 * @author LGD
 * @date 20230109
 * @update: 2023-3-26 更新了新的
*/
Instruction* traverse_to_specified_function(List* this,int order)
{
    Instruction* p;
    traverse_instruction_list_init(this);
    if(order == 0)
    {
        p = get_next_instruction(this);
        return p;
    }
    for(size_t cnt = 0;cnt <= order;)
    {
        p = get_next_instruction(this);
        assert(p && "the function order you request out of range");
        if(ins_get_opCode(p) == FuncLabelOP)
            cnt++;
    }
    return p;
}

/**
 * @brief 将当前zzq寄存器分配表转换为变量信息表
 * @param myMap 变量信息表，它并不是一个未初始化的表，而应当是一个已经存储了所有变量名的表
 * @birth: Created by LGD on 2023-3-26
 * @update: 2023-4-4 不应该判断enum _LOCATION*是否为Memory
*/
HashMap* interface_cvt_zzq_register_allocate_map_to_variable_info_map(HashMap* zzqMap,HashMap* myMap)
{
    HashMapFirst(zzqMap);
    Pair* pair_ptr;
    while((pair_ptr = HashMapNext(zzqMap)) != NULL)
    {
        if(*((enum _LOCATION*)pair_ptr->value) == MEMORY)
        {
            //为该变量（名）进行物理地址映射
            int offset = request_new_local_variable_memory_unit();
            //TODO 为什么是size_t
            set_variable_stack_offset_by_name(myMap,pair_ptr->key,offset);
            //打印分配结果
            printf("%s分配了地址%d\n",pair_ptr->key,offset);
        }
        else
        {
            RegisterOrder reg_order = request_new_allocable_register();
            //为该变量(名)创建寄存器映射
            set_variable_register_order_by_name(myMap,pair_ptr->key,reg_order);
            //打印分配结果
            printf("%s分配了寄存器%d\n",pair_ptr->key,reg_order);
        }
    }

}
