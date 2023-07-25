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

/**
 * @brief 数组字面量初始化表
 * @birth: Created by ZZQ on 2023-7-20
*/
extern HashMap *global_array_init_hashmap;


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
        case NotOP:
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
        case FuncEndOP:
            translate_funcEnd_instruction();
        break;
        case ParamOP:
            translate_param_instructions(this);
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
        case LabelOP:
            translate_label(this);
        break;

    }
}


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
 * @brief 翻译函数调用
 * @birth: Created by LGD on 2023-7-25
*/
void translate_function_call(List* this,Instruction* p1)
{
    //创建子链表
    List* funcCallList = ListInit();
    ListPushBack(funcCallList, p1);
    Instruction* p;
    TAC_OP opCode;
    do {
        ListNext(this, &p);
        opCode = ins_get_opCode(p);
        if((opCode == ParamOP) || (opCode == CallWithReturnValueOP) || (opCode == CallOP))
            ListPushBack(funcCallList, p);
        if((opCode == CallOP) && !strcmp(ins_get_tarLabel(p),"putfloat"))
            procudre_call_strategy = FP_HARD;
        else if((opCode == CallOP) && strcmp(ins_get_tarLabel(p),"putfloat"))
            procudre_call_strategy = FP_SOFT;
    } while(opCode == ParamOP);
    //得知参数传递策略后，从paramOp开始重新翻译
    Instruction* procedureP;
    ListFirst(funcCallList, false);
    while(ListNext(funcCallList, &procedureP)){
        check_before_translate(procedureP);
        translate_IR_test(procedureP);
    }
}
/**
 * @biref:遍历所有三地址并翻译
 * @update:20220103 对链表进行初始化
 *         2023-5-12 order用于标注当前Instruction
 *         2023-5-14 当遇到参数传递时，倒序遍历
 *         2023-5-29 添加临时寄存器归还检测
 *         2023-7-25 遇到paramOp需要向前遍历
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
        //以ParamOp开始，代表对函数的翻译开始
        if(ins_get_opCode(p) == ParamOP)
        {
            translate_function_call(this,p);
            continue;
        }
        if(check_before_translate(p) == false)continue;
        translate_IR_test(p);
        currentInstruction = p;
        detect_temp_register_status();
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
            case AllocateOP:
                translate_global_allocate_instruction(p);
            break;
            case StoreOP:
                translate_global_store_instruction(p);
            break;
            default:
                assert(false && "暂不支持对全局变量的其他操作");
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
 * @brief 判断一个标号是不是叫entry 这是为了跳过zzq设置的函数入口entry标号
 * @author Created by LGD on 20230109
 * @update: 2023-3-28 使用get_left_value基本方法
*/
bool label_is_entry(Instruction* label_ins)
{
    return !strcmp(ins_get_assign_left_value(label_ins)->name,"entry");
}


/**
 * @brief 判断一个Value是否是全局的
 * @birth: Created by LGD on 2023-7-20
*/
bool value_is_global(Value* var)
{
    return (bool)var->IsGlobalVar;
}

/**
 * @brief 判断当前变量是否是数组类型
 * @birth: Created by LGD on 2023-7-24
*/
bool value_is_array(Value* val)
{
    return value_get_type(val) == ArrayTyID;
}

/**
 * @brief 判断一个变量是否是浮点数
 * @author Created by LGD on 20230113
 * @update: 2023-7-20 如果在数组中，也可以判断其是否是浮点数
 *          2023-7-24 支持判断参数是否是浮点数
 *          2023-7-24 对于数组类型，一律返回false
 *          2023-7-25 修正对参数类型判断的错误
 *          2023-7-25 支持判断返回值是否是浮点数
*/
bool value_is_float(Value* var)
{
    if(value_get_type(var) == FloatTyID || value_get_type(var) == ImmediateFloatTyID)return true;
    else if(value_get_type(var) == ArrayTyID)return false;
    else if(value_get_type(var) == ParamTyID){
        if(var->pdata->param_pdata.param_type == FloatTyID) return true;
        return false;
    }
    else if(value_get_type(var) == ReturnTyID){
        if(var->pdata->return_pdata.return_type == FloatTyID) return true;
        return false;
    }
    else return false;
}

/**
 * @brief 判断一个指针指向空间是否是浮点数据
 * @birth: Created by lGD on 2023-7-24
*/
bool value_mem_item_is_float(Value* var)
{
    assert( (value_is_array(var) ||
            value_is_global(var)) && 
            "current value is not a pointer");
    
    if(value_is_array(var)){
        if(var->pdata->array_pdata.array_type == FloatTyID) return true;
        return false;
    }
    if(value_is_global(var)){
        //全局数组
        if(value_is_array(var)){
            if(var->pdata->array_pdata.array_type == FloatTyID) return true;
            return false;
        }
        //全局变量
        else{
            if(value_get_type(var) == FloatTyID) return true;
            return false;
        }
    }
}

/**
 * @brief 通过中间代码Value的描述确定operand的format
 * @birth: Created by LGD on 20230226
*/
enum _DataFormat valueFindFormat(Value* var)
{
    if(value_is_float(var)) return IEEE754_32BITS;
    else return INTERGER_TWOSCOMPLEMENT;
}

/**
 * @brief 返回指针类型Value元素的format
 * @birth: Created by LGD on 2023-7-24
*/
enum _DataFormat value_get_elemFormat(Value* var)
{
    if(value_mem_item_is_float(var))return IEEE754_32BITS;
    else return INTERGER_TWOSCOMPLEMENT;
}

/**
 * @brief 将立即数Value的数值返回，返回结果永远为unsigned 64
 *          当为32位有/无符号整型时返回数值
 *          当为IEEE754 32位浮点数时以浮点记法的64位（高32位为0）返回
 * @birth: Created by LGD on 2023-7-22
**/
uint64_t value_getConstant(Value* val)
{
    if(value_is_float(val))return new_float2IEEE75432BITS(val->pdata->var_pdata.fVal);
    else return val->pdata->var_pdata.iVal;
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
    if(ins_get_opCode(element) == CallOP || ins_get_opCode(element) == CallWithReturnValueOP)
        printf("\t%s",ins_get_tarLabel(element));
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
 * @brief 每次翻译新函数前要执行的初始化
 * @birth:Created by LGD on 2023-5-9
*/
void InitBeforeFunction()
{
    //初始化用于整型的临时寄存器
    Init_arm_tempReg();
    //初始化浮点临时寄存器
    Free_Vps_Register_Init();
    //初始化栈帧状态字
    memset(&currentPF,0,sizeof(struct _Produce_Frame));
    //重置当前Symbal_table

}

/**
 * @brief 获取局部变量域的大小
 * @birth: Created by LGD on 2023-7-17
**/
#ifdef COUNT_STACK_SIZE_VIA_TRAVERSAL_INS_LIST
size_t getLocalVariableSize(ALGraph* self_cfg)
#elif defined COUNT_STACK_SIZE_VIA_TRAVERSAL_MAP
size_t getLocalVariableSize(HashMap* varMap)
#endif
{
    size_t totalLocalVariableSize = 0;
#ifdef COUNT_STACK_SIZE_VIA_TRAVERSAL_INS_LIST
    for (int i = 0; i < self_cfg->node_num; i++) {
        int iter_num = 0;
        ListFirst((self_cfg->node_set)[i]->bblock_head->inst_list,false);
        totalLocalVariableSize += traverse_list_and_count_total_size_of_var((self_cfg->node_set)[i]->bblock_head->inst_list,0); 
    }
#elif defined COUNT_STACK_SIZE_VIA_TRAVERSAL_MAP
    char* name;
    VarInfo* varInfo;
    HashMap_foreach(varMap, name,varInfo)
    {
        
    }
#endif
    return totalLocalVariableSize;
}

/**
 * @brief 翻译为局部数组分配地址空间的指令
 * @birth:Created by LGD on 2023-5-2
 * @update: 2023-5-22 如果操作数是形式参数，语句将调整数组的基址和FP的相对偏移
 *          2023-5-29 考虑了指针在内存的情况
 *          2023-7-20 全局变量作参数allocate时，无需矫正
*/
void translate_allocate_instruction(Instruction* this,HashMap* map)
{
    Value* var = ins_get_assign_left_value(this);
    char* name = var->name;
    //如果是作为参数的数组，什么都不用做
    if(name_is_parameter(name))
        return;
    //如果是全局数组，什么都不用做
    if(value_get_type(var) == ArrayTyID && value_is_global(var))
        return;
    //如果当前是局部数组，在这里分配单元
    else if(value_get_type(ins_get_assign_left_value(this)) == ArrayTyID){
        bool has_init_literal;
        VarInfo* info = HashMapGet(map,var->name);
        size_t totalSpace = var->pdata->array_pdata.total_member * 4;
        if((has_init_literal = array_init_literal(name,totalSpace,(List*)HashMapGet(global_array_init_hashmap, (void*)name))) == true)
        {
            //构造其初始位置
            struct _operand arrayOri = {.addrMode = LABEL_MARKED_LOCATION,.oprendVal = (int64_t)(void*)name};
            set_var_oriLoc(info,arrayOri);
        }
        //为数组也分配空间
        int arrayOffset = request_new_local_variable_memory_units(this->user.value.pdata->array_pdata.total_member*4);
        printf("数组%s分配了地址%d\n",var->name,arrayOffset);
        //构造立即数操作数
        struct _operand arrOff = operand_create_immediate_op(arrayOffset,INTERGER_TWOSCOMPLEMENT);
        //获取局部数组当前地址指针（是寄存器或内存，在之前分配）
        struct _operand addrPtr = info->current;
        //和FP相加构成绝对地址
        addiii(addrPtr, fp, arrOff);

        //在数据段构造字面量
        if(has_init_literal)
        {
            //传递目的地址
            movii(r023_int[0], info->current);
            add_register_limited(PARAMETER1_LIMITED);
            //传递源地址 //伪指令获得
            pseudo_ldr("LDR",r1,info->ori);
            add_register_limited(PARAMETER2_LIMITED);
            //传递尺寸
            struct _operand size = operand_create_immediate_op(totalSpace,INTERGER_TWOSCOMPLEMENT);
            movii(r023_int[2],size);
            add_register_limited(PARAMETER3_LIMITED);
            
            //若存在字面量，将当前字面量首地址拷贝到当前地址处
            branch_instructions("memcpy", "L", false, NONELABEL);

            //解除寄存器限制
            remove_register_limited(PARAMETER1_LIMITED | PARAMETER2_LIMITED | PARAMETER3_LIMITED);

            info->ori = info->current;
        }
        //对于没有字面量的局部数组，应当清空内存
        else{
            //传递目的地址
            movii(r0, info->current);
            add_register_limited(PARAMETER1_LIMITED);
            //传递清0值
            struct _operand setValue = operand_create_immediate_op(0,INTERGER_TWOSCOMPLEMENT);
            movii(r1,setValue);
            add_register_limited(PARAMETER2_LIMITED);
            //传递尺寸
            struct _operand size = operand_create_immediate_op(totalSpace,INTERGER_TWOSCOMPLEMENT);
            movii(r2,size);
            add_register_limited(PARAMETER3_LIMITED);

            //不存在字面量，清干净内存
            branch_instructions("memset", "L", false, NONELABEL);

            //解除寄存器限制
            remove_register_limited(PARAMETER1_LIMITED | PARAMETER2_LIMITED | PARAMETER3_LIMITED);

            info->ori = info->current;
        }
    }
}

/**
 * @brief 翻译FuncEndOp
 * @birth: Created by LGD on 2023-7-23
*/
void translate_funcEnd_instruction()
{
    //恢复SP
    reset_sp_value(false);
    //恢复现场
    bash_push_pop_instruction_list("POP",currentPF.used_reg);
    //退出函数
    bash_push_pop_instruction("POP",&fp,&pc,END);
}


/**
 * @brief 将为所有数组分配空间并将基地址（绝对的）传递给它们的指针
 * @birth: Created by LGD on 2023-7-20
*/
void attribute_and_load_array_base_address(Function *handle_func,HashMap* map)
{
  //不再有 所有的指令都分配了变量信息表 的前提
  ALGraph *self_cfg = handle_func->self_cfg;
  for (int i = 0; i < self_cfg->node_num; i++) {
    int iter_num = 0;
    List* list = (self_cfg->node_set)[i]->bblock_head->inst_list;
    ListFirst(list,false);
    Instruction* p;
    ListFirst(list,false);
    ListNext(list,&p);
    do
    {
        if(ins_get_opCode(p) == AllocateOP)
            translate_allocate_instruction(p,map);
    }while(ListNext(list,&p) && ins_get_opCode(p)!=FuncLabelOP);
  }
}

/**
 * @brief 为指令链安插变量信息表
 * @birth: Created by LGD on 2023-7-20
*/
void insert_variable_map(Function *handle_func,HashMap* map)
{
    ALGraph *self_cfg = handle_func->self_cfg;
    Instruction* p;
    //第二次function遍历，为每一句Instruction安插一个map
    for (int i = 0; i < self_cfg->node_num; i++) {
        int iter_num = 0;
        ListFirst((self_cfg->node_set)[i]->bblock_head->inst_list,false);
        while(ListNext((self_cfg->node_set)[i]->bblock_head->inst_list,&p))
        ins_shallowSet_varMap(p,map);
  }
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