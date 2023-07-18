#include "interface_zzq.h"

#include "instruction.h"
#include "function.h"

#include "variable_map.h"
#include "arm.h"
#include "arm_assembly.h"
#include "dependency.h"
#include <assert.h>
#include "config.h"
#include "translate.h"
#include "memory_manager.h"

#include "Pass.h"
#include "debug.h"

/* Extern Global Variable */
extern bool Open_Register_Allocation;
extern HashMap* func_hashMap;
/* Extern Global Variable End*/


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
 * @update: 2023-5-16 functionLabel通过全局查表给出
 * @update: 2023-5-29 Goto 和 GotoCondition修改了获取目标标号的位置
*/
char* ins_get_tarLabel(Instruction* this)
{
    Value* functionValue;
    assert(this->opcode != GotoOP || this->opcode != GotoWithConditionOP || this->opcode != CallOP || this->opcode != CallWithReturnValueOP);
    switch(ins_get_opCode(this))
    {
        
        case CallOP:
        case CallWithReturnValueOP:
        
            functionValue = (Value*)HashMapGet(func_hashMap,ins_get_assign_left_value(this)->name);
            return functionValue->name;

        case GotoOP:
        case GotoWithConditionOP:
            return ins_get_assign_left_value(this)->pdata->func_call_pdata.name;   //条件跳转时，只需要跳转至正确，错误顺序执行
    }
}

/**
 * @brief 获取条件调整语句的标号
 * @birth: Created by LGD on 2023-5-29
 * @update: 2023-7-11 不负责识别Opcode来判断true or false
*/
char* ins_get_tarLabel_Conditional(Instruction* this,bool cond)
{
    assert(this->opcode != GotoOP || this->opcode != GotoWithConditionOP);
    if(cond)
        return ins_get_assign_left_value(this)->pdata->condition_goto.true_goto_location->name;
    else
        return ins_get_assign_left_value(this)->pdata->condition_goto.false_goto_location->name;
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
// Value* get_op_from_return_instruction(Instruction* this)
// {
//     return this->user.value.pdata->return_pdata.return_value;
// }
#endif



void translate_IR_test(struct _Instruction* this)
{
    // @brief:这个方法通过判断Instruction的指令来执行对应的翻译方法
    // @birth:Created by LGD on 20221224
    // @update:Latest 20221224:添加对比较运算符的翻译支持
    TAC_OP opCode = ins_get_opCode(this);
    switch(opCode)
    {
#ifdef OPEN_TRANSLATE_BINARY
        case AddOP:
        case SubOP:
        case MulOP:
        case DivOP:
        case ModOP:
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
        case LogicAndOP:
        case LogicOrOP:
            translate_and_or_instructions(this);
        break;
#endif
        case NegativeOP:
            translate_unary_instructions(this);
        break;
        case GotoWithConditionOP:
            translate_goto_instruction_test_bool(this);
        break;
        case GotoOP:
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
        case ParamOP:
            translate_param_instructions(this);
        break;
        case FuncEndOP:
            //translate_function_end(this);
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
        case GetelementptrOP:
            translate_getelementptr_instruction(this);
        break;
        case StoreOP:
            translate_store_instruction(this);
        break;
        case LoadOP:
            translate_load_instruction(this);
        break;
        case AllocateOP:
            translate_allocate_instruction(this);
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
 *         2023-5-12 order用于标注当前Instruction
 *         2023-5-14 当遇到参数传递时，倒序遍历
 *         2023-5-29 添加临时寄存器归还检测
*/
size_t traverse_list_and_translate_all_instruction(List* this,int order)
{
    Instruction* p;
    ListFirst(this,false);
    ListNext(this,&p);
    while(order!=0)
    {
        ListNext(this,&p);
        --order;
    }
    do
    {
        if(check_before_translate(p) == false)continue;
        translate_IR_test(p);
        currentInstruction = p;
        //detect_temp_register_status();
    }while(ListNext(this,&p) && ins_get_opCode(p)!=FuncLabelOP);

}

/**
@brief:翻译全局变量定义链表
@birth:Created by LGD on 2023-5-29
*/
void translate_global_variable_list(List* this)
{
    Instruction* p = NULL;
    ListFirst(this,false);
    while(ListNext(this,&p))
    {
        switch(p->opcode)
        {
            case StoreOP:
                translate_global_store_instruction(p);
            break;
            case AllocateOP:
                translate_global_allocate_instruction(p);
            break;
            default:
            break;
        }
    };
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
 *          2023-5-3 
*/
// HashMap* interface_cvt_zzq_register_allocate_map_to_variable_info_map(HashMap* zzqMap,HashMap* myMap)
// {
//     HashMapFirst(zzqMap);
//     Pair* pair_ptr;
//     while((pair_ptr = HashMapNext(zzqMap)) != NULL)
//     {
//         if(*((enum _LOCATION*)pair_ptr->value) == MEMORY)
//         {
//             //为该变量（名）进行物理地址映射
//             int offset = request_new_local_variable_memory_unit();
//             //TODO 为什么是size_t
//             set_variable_stack_offset_by_name(myMap,pair_ptr->key,offset);
//             //打印分配结果
//             printf("%s分配了地址%d\n",pair_ptr->key,offset);
//         }
//         else
//         {
//             printf("正在为变量%s分配寄存器\n",pair_ptr->key);
//             RegisterOrder reg_order = request_new_allocable_register();
//             //为该变量(名)创建寄存器映射
//             set_variable_register_order_by_name(myMap,pair_ptr->key,reg_order);
//             //打印分配结果
//             printf("%s分配了寄存器%d\n",pair_ptr->key,reg_order);
//         }
//     }

// }


/**
 * @brief 从指令中返回步长
 * @birth: Created by LGD on 2023-5-1
 * @update: 2023-7-16 步长应当为数组维度*元素的大小
 * @TODO
*/
int ins_getelementptr_get_step_long(Instruction* this)
{
    return ins_get_operand(this,1)->pdata->array_pdata.step_long * 4;
}

/**
 * @brief 打印一句中端代码信息
 * @birth: Created by LGD on 2023-7-16
*/
void print_ins(Instruction *element)
{
        // 打印出每条instruction的res的名字信息
    printf("%9s\t     %d: %20s ", "", ((Instruction *)element)->ins_id,
           op_string[((Instruction *)element)->opcode]);
    printf("\t%25s ", ((Value *)element)->name);

    if (((Instruction *)element)->opcode == PhiFuncOp) {
      printf("\tsize: %d ",
             HashMapSize(((Value *)element)->pdata->phi_func_pdata.phi_value));
      Pair *ptr_pair;
      HashMapFirst(((Value *)element)->pdata->phi_func_pdata.phi_value);
      while ((ptr_pair = HashMapNext(
                  ((Value *)element)->pdata->phi_func_pdata.phi_value)) !=
             NULL) {
        printf("\tbblock: %s value: %s, ", (char *)(ptr_pair->key),
               ((Value *)ptr_pair->value)->name);
      }
    } else if (((Instruction *)element)->user.num_oprands == 2) {
      printf("\t%10s, %10s",
             user_get_operand_use(((User *)element), 0)->Val->name,
             user_get_operand_use(((User *)element), 1)->Val->name);
    } else if (((Instruction *)element)->user.num_oprands == 1) {
      printf("\t%10s", user_get_operand_use(((User *)element), 0)->Val->name);
    } else if (((Instruction *)element)->user.num_oprands == 0) {
      printf("\t%10s", "null");
    }
    printf("\n");
}

/**
 * @brief 获取函数的参数个数
 * @birth: Created by LGD on 2023-7-17
*/
size_t func_get_param_numer(Function* func)
{
    return (size_t)func->label->pdata->symtab_func_pdata.param_num;
}

/**
 * @brief 获取局部变量域的大小
 * @birth: Created by LGD on 2023-7-17
**/
size_t getLocalVariableSize(ALGraph* self_cfg)
{
    size_t totalLocalVariableSize = 0;
    for (int i = 0; i < self_cfg->node_num; i++) {
        int iter_num = 0;
        ListFirst((self_cfg->node_set)[i]->bblock_head->inst_list,false);
        totalLocalVariableSize += traverse_list_and_count_total_size_of_var((self_cfg->node_set)[i]->bblock_head->inst_list,0); 
    }
    return totalLocalVariableSize;
}

/**
 * @brief 设置当前所有参数的初始位置
 * @warning 所有的参数应当已经初始化VarInfo信息块，未分配寄存器除外
 * @birth: Created by LGD on 2023-7-17
*/
void set_param_origin_place(HashMap* varMap,size_t param_number)
{
    char paramName[16] = {0};
    VarInfo* varInfo;
    for(int i=0;i<param_number;++i)
    {
        sprintf(paramName,"param%d",i);
        if(!HashMapContain(varMap, paramName)){
            //当前参数没有分配寄存器
            continue;
        }
        varInfo = HashMapGet(varMap, paramName);
        
        //HashMapPut(varMap, strdup(paramName), varInfo);
        if(i <= 3){
            set_var_oriLoc(varInfo, r027[i]);
        }
        else{
            fp_indicate_offset.addtion = get_param_stack_offset_by_idx(i);
            set_var_oriLoc(varInfo, fp_indicate_offset);
        }
    }
}