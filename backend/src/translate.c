#include "translate.h"
#include "arm.h"
#include "arm_assembly.h"
#include "variable_map.h"
#include "interface_zzq.h"
#include "dependency.h"
#include "memory_manager.h"
#include "operand.h"

#include "optimize.h"
#include "enum_2_str.h"
#include "math.h"

/* Global Variable */
AssembleOperand nullop;
Instruction* functionEntrance = NULL;
/* Global Variable End */

/* Extern Global Variable*/
extern HashMap* func_hashMap;
/* Extern Global Variable End*/

/**
 * @brief 当前翻译指令
 * @birth:Created by LGD on 2023-5-29
*/
Instruction* currentInstruction = NULL;

/**
* @brief 翻译前的钩子，false表面当前指令的翻译应当终止
* @birth: Created by LGD on 2023-7-9
* @update: 2023-7-16 修正了store语句检查目标操作数失败的BUG
*           2023-7-20 添加对CallWithReturnOp的考虑
*/
bool check_before_translate(Instruction* this)
{
#ifdef DEBUG_MODE
    //打印当前中端代码信息
    print_ins(this);
#endif
    enum _Instruction_Type ins_type = VALID_INSTRUCTION;
    //检查该指令的操作数是否均分配
    if(ins_get_operand_num(this) >= 2)
    {
        if(operand_is_unallocated(toOperand(this, SECOND_OPERAND)))
            ins_type = INVALID_INSTRUCTION;
    }
    if(ins_get_operand_num(this)>= 1)
    {        
        //
        if(operand_is_unallocated(toOperand(this, FIRST_OPERAND)))
            ins_type = INVALID_INSTRUCTION;

        //当前指令是storeop，则目的操作数是不存在的
        if(ins_get_opCode(this) == StoreOP)
            return true;
        //当前例程没有义务为参数分配空间，因此分配查询是无效的
        if(name_is_parameter(ins_get_assign_left_value(this)->name))
            return true;

        //检查目标操作数是否有分配内存或寄存器
        if((ins_get_opCode(this) != GotoWithConditionOP) && 
            (ins_get_opCode(this) != ReturnOP))
            if(operand_is_unallocated(toOperand(this, TARGET_OPERAND)))
                ins_type = INVALID_INSTRUCTION;
    }
    // if(ins_get_operand_num(this) == 0)
    // {
    //     if(ins_get_opCode(this) == CallWithReturnValueOP)
    //         if(operand_is_unallocated(toOperand(this, TARGET_OPERAND)))
    //             ins_type = INVALID_INSTRUCTION;
    // }
    if(ins_type == INVALID_INSTRUCTION)
    {
#ifdef GEN_UNDEF
        //生成一条未定义指令
        undefined();
#endif
        return false;
    }
    return true;
}

/**
 * @brief 翻译一个函数入口三地址代码，进行的操作包括入栈LR,CPSR，开辟栈区
 * @birth: Created by LGD on 20221212
 * @update: 202230316 简化了代码，按照aapcs规范调用子程序
 *          20221212 新入栈：CPSR(考虑中)
 *          20221212 加入对Label的判断
 *          20221225 Label汇编语句生成方法内部判定label是否存在
 *          20221225 修改了stackSize的获取方式
*/
/**
 * 参照sysy运行时库
 * 000001c8 <putch>:
 1c8:   b580            push    {r7, lr}
 1ca:   b082            sub     sp, #8
 1cc:   af00            add     r7, sp, #0
 1ce:   6078            str     r0, [r7, #4]
 1d0:   6878            ldr     r0, [r7, #4]
 1d2:   f7ff fffe       bl      0 <putchar>
 1d6:   bf00            nop
 1d8:   3708            adds    r7, #8
 1da:   46bd            mov     sp, r7
 1dc:   bd80            pop     {r7, pc}
 1de:   bf00            nop
*/
void translate_function_entrance(Instruction* this)
{
    //第一步：创建函数标号  
    Label(ins_get_label(this));
    
    //第二步 堆栈LR寄存器和R7
    push_pop_instructions("PUSH",fp);
    push_pop_instructions("PUSH",lr);
    
    // //第二步，SP自减栈空间
    // immedOp.oprendVal = get_function_stackSize();
    // general_data_processing_instructions("SUB",sp,sp,immedOp,NONESUFFIX,false,NONELABEL);

    //第四步 设定当前函数的各项参数
    //set_stack_frame_status(2,20);
}

/**
 * @brief 翻译函数的出口Instruction，主要任务包括SP的自减以销毁栈区，弹出LR赋给PC，将返回值交给变量
 * @birth: created by LGD on 20221208
 * @update: 202230316 简化了代码，按照aapcs规范调用子程序
 *          20221219 更新了获取偏移量的位置(通过哈希表VarMap获取)
 *          20221211 带返回值的call，将会获取接收返回值的变量将R0赋给它
 *          20221224 当函数结束执行完后，函数入口全局变量应当被销毁
 *          20221224 函数的出口不应当关注返回值由谁接收，应当由call指令关注
 *          20221225 修改了stackSize的获取方式
*/
void translate_function_end(Instruction* this)
{
     
    // immedOp.oprendVal = get_function_stackSize();
    // general_data_processing_instructions("ADD",sp,sp,immedOp,NONESUFFIX,false,NONELABEL);
    //第一步，SP自加栈空间

    push_pop_instructions("POP",fp);
    push_pop_instructions("POP",pc);
    //第二步 把LR寄存器弹出给PC
}



/**
 * @birth: Created by LGD on 2023-3-16
*/
size_t func_get_param_number()
{
    return currentPF.param_counter++;
}

/**
 * @brief 刷新参数个数
 * @birth: Created by LGD on 2023-5-10
*/
size_t flush_param_number()
{
    currentPF.param_counter=0;
}

/**
 * @brief 翻译传递参数的指令
 * @author Created by LGD on 2023-3-16
 * @update:2023-5-14 考虑参数在内存和为立即数的情况
 * @update:2023-5-30 添加寄存器限制
 *         2023-7-28 修改了对add_register_limited的调用方式
 *         2023-7-29 修复浮点参数错误的栈帧指针自减方法
*/
void translate_param_instructions(Instruction* this)
{
    //依据存储位置转换为operand类型
    AssembleOperand param_op = toOperand(this,FIRST_OPERAND);
    //获取当前传递参数序号
    size_t passed_param_number = get_parameter_idx_by_name(ins_get_assign_left_value(this)->name);

    if(procudre_call_strategy == FP_SOFT)
    {
        //当前传入的变量是其余情况（全局变量 全局数组 全局数组的解引用 局部数组 局部变量）
        //当参数为变量或立即数时，形式参数要判断是否需要类型提升
        if(value_is_float(ins_get_operand(this, TARGET_OPERAND)))
        {
            if(passed_param_number <= 3)
            {
                mov(r023_float[passed_param_number],param_op);
                //寄存器限制
                add_register_limited(passed_param_number);
            }
            else{
                mov(param_push_op_float,param_op);
                subiii(sp,sp,operand_create_immediate_op(0, INTERGER_TWOSCOMPLEMENT));
            }
        }
        else{
            //小于等于4个则直接丢R0-R3
            if(passed_param_number <= 3)
            {
                mov(r023_int[passed_param_number],param_op);
                //寄存器限制
                add_register_limited(passed_param_number);
            }
            else
                //堆入栈顶
                mov(param_push_op_interger,param_op);
        }
    }
    else if(procudre_call_strategy == FP_HARD)
    {
        //当前传入的变量是其余情况（全局变量 全局数组 全局数组的解引用 局部数组 局部变量）
        //当参数为变量或立即数时，形式参数要判断是否需要类型提升
        if(value_is_float(ins_get_operand(this, TARGET_OPERAND)))
        {
            if(passed_param_number <= 3)
            {
                mov(s023_float[passed_param_number],param_op);
                //寄存器限制
                add_register_limited(passed_param_number);
            }
            else{
                mov(param_push_op_float,param_op);
                subii(sp,operand_create_immediate_op(0, INTERGER_TWOSCOMPLEMENT));
            }
        }
        else assert(false && "current FP_HARD strategy does not support interger for parameter");
    }
}

/**
 * @brief 无返回值调用
 * @birth: Created by LGD on 2023-3-16
 * @update:2023-5-30 取消寄存器限制
 *          2023-6-6  添加函数返回
*/
void translate_call_instructions(Instruction* this)
{
    flush_param_number();
    //获取跳转标签
    char* label = ins_get_tarLabel(this);
    //执行跳转
    branch_instructions(label,"L",false,NONELABEL);

    //取消寄存器限制
    remove_register_limited(R0);
    remove_register_limited(R1);
    remove_register_limited(R2);
    remove_register_limited(R3);
    //第二步 确保栈顶恢复到栈帧的位置
    general_data_processing_instructions_extend(MOV,NONESUFFIX,false,sp,fp,nullop);

    //定义函数已执行，这用于重置参数传递的状态
    passed_param_number = 0;
}

/**
 * @brief 翻译调用函数并接收返回值的指令
 * @author Created by LGD on 20221224
 * @update: 20230316 添加了一些对进程调用状态的维护
 *          20230114 添加了对浮点数和隐式转换的支持
 *          20221225 封装后使函数更精简
 *          20221225 添加了将变量重新放回内存的支持
 *          20221224 翻译时接收返回值的变量调整为赋值号左侧的变量
 *          20221224 设计有误，不需要存储当前的函数入口
 *          2023-5-30 取消寄存器限制
 *          2023-6-6  添加函数返回
*/
void translate_call_with_return_value_instructions(Instruction* this)
{
    flush_param_number();
    //第一步 跳转至对应函数的标号
    char* tarLabel = ins_get_tarLabel(this);

    Value* val = ins_get_operand(this, TARGET_OPERAND);

    branch_instructions(tarLabel,"L",false,NONELABEL);

    //取消寄存器限制
    remove_register_limited(R0);
    remove_register_limited(R1);
    remove_register_limited(R2);
    remove_register_limited(R3);
    add_register_limited(R0);

    //第二步 确保栈顶恢复到栈帧的位置
    general_data_processing_instructions_extend(MOV,NONESUFFIX,false,sp,fp,nullop);
    
    //第三步 回程将R0赋给指定变量
    //根据 The Base Procedure Call Standard
    //32位 (4字节 1字长)的数据 （包括sysy的整型和浮点型数据） 均通过R0 传回

    struct _operand returnVal = toOperand(this,TARGET_OPERAND);
    //如果返回值无效，则无需翻译

    if(!operand_is_unallocated(returnVal))
    {
        if(!strcmp(ins_get_tarLabel(this),"getfloat"))
            mov(returnVal,returnFloatOp);
        else
        {
            if(value_is_float(val))
                movff(returnVal,returnFloatSoftOp);
            else
                movii(returnVal,returnIntOp);
        }
    }
    remove_register_limited(R0);
    //定义函数已执行，这用于重置参数传递的状态
    passed_param_number = 0;
}


/**
 * @brief 完成变量的位置转移
*/
void variable_place_shift(Instruction* this,Value* var,AssembleOperand cur)
{
    
}

/**
 * @brief 翻译乘法的移位语句
 * @birth: Created by LGD on 2023-8-20
*/
void translate_mul_lsl(Instruction* this,struct _LSL_FEATURE feat){
    struct _operand unimmed;
    struct _operand middleOp;
    struct _operand tarOp = toOperand(this,TARGET_OPERAND);
    if(feat.op_idx == 1)
        unimmed = toOperand(this,SECOND_OPERAND);
    else if(feat.op_idx == 2)
        unimmed = toOperand(this,FIRST_OPERAND);

    
    switch(feat.feature){
        case LSL_ZERO:
        {
            struct _operand immed = operand_create_immediate_op(0, INTERGER_TWOSCOMPLEMENT);
            movii(unimmed,immed);
        }
        break;
        case _2_N:
        {
            operand_set_shift(&unimmed,LSL,feat.b1);
            movii(tarOp,unimmed);
        }
        break;
        case _2_N_ADD_1:
        {
            struct _operand temp = unimmed;
            operand_set_shift(&temp,LSL,feat.b1);
            addiii(tarOp, unimmed, temp);
        }
        break;
        case _2_M_N_2_N_1:
        {
            struct _operand temp = unimmed;
            uint8_t min = feat.b1>feat.b2 ? feat.b2 : feat.b1;
            uint8_t max = feat.b1>feat.b2 ? feat.b1 : feat.b2;

            operand_set_shift(&temp,LSL,min);
            movii(unimmed,temp);

            operand_set_shift(&temp,LSL,max - min);
            addiii(tarOp,unimmed,temp);
        }
        break;
    }
}

/**
 * @brief 翻译除法优化语句
 * @birth: Created by LGD on 2023-8-20
*/
void translate_div_2(Instruction* this)
{
    struct _operand divisor = toOperand(this,SECOND_OPERAND);
    struct _operand divided = toOperand(this,FIRST_OPERAND);
    struct _operand target = toOperand(this,TARGET_OPERAND);
    struct _operand temp = divided;
    int8_t shiftTime = number_get_shift_time(operand_get_immediate(divisor));
    operand_set_shift(&temp,LSR,shiftTime);
    movii(target,temp);

}

/**
 * @brief 翻译所有的双目赋值运算,但是采用add + mov 的新方法
 * @birth: Created by LGD on 20230226
 * @update: 20230306 新的变量位置切换
 *          2023-4-10 添加除法运算
*/
void translate_binary_expression_binary_and_assign(Instruction* this)
{   
    TAC_OP opCode = ins_get_opCode(this);
    //乘法左移优化
    // if(ins_mul_2_lsl_trigger(this))
    // {
    //     ins_mul_2_lsl(this);
    //     return;
    // }
    
    if(opCode == SubOP)
    {
        translate_sub(this);
        return;
    }

    //Mul Optimize
    struct _LSL_FEATURE feat;
    if(number_is_lsl_trigger(this,&feat))
    {
        translate_mul_lsl(this,feat);
        return;
    }

    //Div Optimize
    if(div_optimize_trigger(this))
    {
        translate_div_2(this);
        return;
    }
        
    struct _operand op1,op2,srcOp1,srcOp2;
    op1 = srcOp1 = toOperand(this,FIRST_OPERAND);
    op2 = srcOp2 = toOperand(this,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(this,TARGET_OPERAND);
    AssembleOperand middleOp;

    //如果使用了除法，由于在一个Instruction内完成传参，需要限制r0和r1的访问权限
    if((opCode == DivOP) || (opCode == ModOP))
    {
        add_register_limited(R0);
        add_register_limited(R1);
    }

    //双目中有其一是浮点数
    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        //寄存器 内存 立即数 都转换
        op1 = operandConvert(srcOp1,VFP,0,0);
        op2 = operandConvert(srcOp2,VFP,0,0);

        if(!ins_operand_is_float(this,FIRST_OPERAND) )
            operand_r2r_cvt(op1, op1);
        if(!ins_operand_is_float(this,SECOND_OPERAND))
            operand_r2r_cvt(op2, op2);
        
        middleOp = operand_pick_temp_register(VFP);
        middleOp.format = IEEE754_32BITS;

        switch(ins_get_opCode(this))
        {
            case AddOP:
                fadd_and_fsub_instruction("FADD",
                    middleOp,op1,op2,FloatTyID);
            break;
            case MulOP:
                fadd_and_fsub_instruction("FMUL",
                    middleOp,op1,op2,FloatTyID);
            break;
            case DivOP:
                fadd_and_fsub_instruction("FDIV",
                    middleOp,op1,op2,FloatTyID);
            break;
        }
    }
    else
    {   
        if(!operand_is_in_register(srcOp1))
            op1 = operand_load_to_register(srcOp1, nullop, ARM);
        if(!operand_is_in_register(srcOp2))
            op2 = operand_load_to_register(srcOp2, nullop, ARM);


        switch(ins_get_opCode(this))
        {
            case AddOP:
                middleOp = operand_pick_temp_register(ARM);
                middleOp.format = INTERGER_TWOSCOMPLEMENT;
                general_data_processing_instructions(ADD,
                    middleOp,op1,op2,NONESUFFIX,false);
            break;
            case SubOP:
                middleOp = operand_pick_temp_register(ARM);
                middleOp.format = INTERGER_TWOSCOMPLEMENT;
                general_data_processing_instructions(SUB,
                    middleOp,op1,op2,NONESUFFIX,false);
            break;
            case MulOP:
                middleOp = operand_pick_temp_register(ARM);
                middleOp.format = INTERGER_TWOSCOMPLEMENT;
                general_data_processing_instructions(MUL,
                    middleOp,op1,op2,NONESUFFIX,false);
            break;
#ifdef USE_DIV_ABI
            case DivOP:
                middleOp = r023_int[0];
                movii(r023_int[0],op1);
                movii(r023_int[1],op2);
                branch_instructions_test("__aeabi_idiv","L",false,NONELABEL);
            break;
            case ModOP:
                middleOp = r023_int[1];
                movii(r023_int[0],op1);
                movii(r023_int[1],op2);
                branch_instructions_test("__aeabi_idivmod","L",false,NONELABEL);
            break;
#else
            assert("未实现不调用库的除法");
#endif  
        }
    }

    
    //释放第一、二操作数
    general_recycle_temp_register_conditional(this,FIRST_OPERAND,op1);
    general_recycle_temp_register_conditional(this,SECOND_OPERAND,op2);

    //中间量传给目标变量
    if(ins_operand_is_float(this,TARGET_OPERAND))
    {
        if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
            movff(tarOp,middleOp);
        else
            movfi(tarOp,middleOp);
    }
    else
    {
        if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
            movif(tarOp,middleOp);
        else
            movii(tarOp,middleOp);
    }

    //如果使用了除法，由于在一个Instruction内完成传参，需要限制r0和r1的访问权限
    if((opCode == DivOP) || (opCode == ModOP))
    {
        remove_register_limited(R0);
        remove_register_limited(R1);
    }

    //释放中间操作数
    operand_recycle_temp_register(middleOp);
}

/**
 * @brief 采用add + mov 的减法翻译
 * @birth: Created by LGD on 2023-4-24
 * @update: 2023-7-11 提供对两个立即数减法运算的支持
*/
void translate_sub(Instruction* this)
{   
    struct _operand oriOp1,op1,oriOp2,op2;
    oriOp1 = op1 = toOperand(this,FIRST_OPERAND);
    oriOp2 = op2 = toOperand(this,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(this,TARGET_OPERAND);
    AssembleOperand middleOp;

    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {

        //寄存器 内存 立即数 都转换
        op1 = operandConvert(oriOp1,VFP,0,0);
        op2 = operandConvert(oriOp2,VFP,0,0);
        
        if(!ins_operand_is_float(this,FIRST_OPERAND) )
            operand_r2r_cvt(op1, op1);
        if(!ins_operand_is_float(this,SECOND_OPERAND))
            operand_r2r_cvt(op2, op2);
        
        middleOp = operand_pick_temp_register(VFP);
        middleOp.format = IEEE754_32BITS;

        fadd_and_fsub_instruction("FSUB",
            middleOp,op1,op2,FloatTyID);

        //释放第一、二操作数
        general_recycle_temp_register_conditional(this,FIRST_OPERAND,op1);
        general_recycle_temp_register_conditional(this,SECOND_OPERAND,op2);
    }
    else
    {   
        middleOp = operand_pick_temp_register(ARM);
        middleOp.format = INTERGER_TWOSCOMPLEMENT;

#ifndef ALLOW_TWO_IMMEDIATE
        assert(!(opernad_is_immediate(op1) && opernad_is_immediate(op2)) && "减法中两个操作数都是立即数是不允许的");
#endif
        //如果第1个操作数为立即数，第2个不是，使用RSB指令
        if(operand_is_immediate(op1) && !operand_is_immediate(op2))
        {
            //判断第一立即数是否合法
            if(!check_immediate_valid(op1.oprendVal))
                op1 = operand_load_to_register(oriOp1, nullop, ARM);
            op2 = operandConvert(op2,ARM,false,IN_STACK_SECTION);
            general_data_processing_instructions(RSB,
                middleOp,op2,op1,NONESUFFIX,false);
        }
        //第1个操作数不是立即数，第2个操作数可以是任何数，使用SUB指令
        else{
            op1 = operandConvert(op1,ARM,false,IN_STACK_SECTION | IN_INSTRUCTION);
            op2 = operandConvert(op2,ARM,false,IN_STACK_SECTION | IN_INSTRUCTION);
            general_data_processing_instructions(SUB,
                middleOp,op1,op2,NONESUFFIX,false);
        }

        //释放第一、二操作数
        general_recycle_temp_register_conditional(this,FIRST_OPERAND,op1);
        general_recycle_temp_register_conditional(this,SECOND_OPERAND,op2);
    }

    //中间量传给目标变量
    if(ins_operand_is_float(this,TARGET_OPERAND))
    {
        if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
            movff(tarOp,middleOp);
        else
            movfi(tarOp,middleOp);
    }
    else
    {
        if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
            movif(tarOp,middleOp);
        else
            movii(tarOp,middleOp);
    }
    //释放中间操作数
    operand_recycle_temp_register(middleOp);
}

/**
 * @brief 拆出数组的地址
 * @birth: Created by LGD on 2023-5-1
*/
void translate_getelementptr_instruction(Instruction* this)
{
    struct _operand tarOp = toOperand(this,TARGET_OPERAND);
    struct _operand arrBaseOri = toOperand(this,FIRST_OPERAND);
    struct _operand idx = toOperand(this,SECOND_OPERAND);
    struct _operand step_long = operand_create_immediate_op(ins_getelementptr_get_step_long(this),INTERGER_TWOSCOMPLEMENT);
    struct _operand arrBase = arrBaseOri;
    //解引用的对象是全局数组 局部数组或者全局数组的解引用
    arrBase = operand_load_to_register(arrBaseOri,nullop,ARM);


    idx = operand_load_to_register(idx,nullop,ARM);
    //使用乘加指令
    step_long = operand_load_to_register(step_long,nullop,ARM);
    struct _operand middleOp = operand_pick_temp_register(ARM);
    middleOp.format = INTERGER_TWOSCOMPLEMENT;
    //乘加指令，且tarOp不会作为立即数，没必要从内存加载
    //middleOp = operand_load_to_register(tarOp,nullop);

    //乘加计算目标地址
    general_data_processing_instructions_extend(MLA,NONESUFFIX,false,middleOp,step_long,idx,arrBase,nullop);
    
    general_recycle_temp_register_conditional(this,FIRST_OPERAND,arrBase);
    general_recycle_temp_register_conditional(this,SECOND_OPERAND,idx);
    //回收step_long 步长
    recycle_temp_arm_register(step_long.oprendVal);

    //送入变量位置
    if(!operand_is_same(tarOp,middleOp))
    {
        movii(tarOp,middleOp);
        general_recycle_temp_register_conditional(this,TARGET_OPERAND,middleOp);
    }
    

}

/**
 * @brief 翻译将数据回存到一个指针指向的位置的指令
 * @birth: Created by LGD on 2023-5-3
 * @update: 2023-5-29 添加对全局变量的回写操作
 *          2023-7-20 添加对全局数组的回写操作
*/
void translate_store_instruction(Instruction* this)
{
    //被存储的操作数字段
    struct _operand stored_elem = toOperand(this,SECOND_OPERAND);
    //存放位置的操作数字段，往往是寄存器或以FP相对寻址的内存，存放绝对地址
    //其format或point2format应该没有什么意义
    struct _operand addr = toOperand(this,FIRST_OPERAND);

    Value* addrValue = ins_get_operand(this, FIRST_OPERAND);
    //当对全局变量操作时
    if(value_is_global(addrValue) && !value_is_array(addrValue))
    {
        //申请存放地址的临时寄存器
        struct _operand addrLoadedReg = operand_pick_temp_register(ARM);
        //取全局变量标号指示的内存单元
        pseudo_ldr("LDR",addrLoadedReg,addr);

        //构造间址
        struct _operand addressingOp = operand_Create_indirect_addressing(
            addrLoadedReg.oprendVal, 
            value_get_elemFormat(ins_get_operand(this,FIRST_OPERAND)));

        //将数据送入内存单元
        mov(addressingOp,stored_elem);
        //归还临时地址寄存器
        operand_recycle_temp_register(addrLoadedReg);
    }
    //当对局部数组 或 全局数组的解引用指针（最后一层引用） 操作时
    else if((value_get_type(ins_get_operand(this, FIRST_OPERAND)) == ArrayTyID))
    {
        //封装间接寻址，根据地址空间数据类型确定point2format
        struct _operand storeMem = operand_Create_indirect_addressing(
            addr.oprendVal,
            value_get_elemFormat(ins_get_operand(this,FIRST_OPERAND)));
        //执行store
        mov(storeMem,stored_elem);
    }
}

/**
 * @brief 翻译将数据从一个指针的指向位置取出
 * @birth: Created by LGD on 2023-5-4
 * @update:  2023-7-20 添加对全局数组的加载操作
*/
void translate_load_instruction(Instruction* this)
{
    struct _operand loaded_target= toOperand(this,TARGET_OPERAND);
    struct _operand addr = toOperand(this,FIRST_OPERAND);
    //当全局变量操作时
    if((value_is_global(ins_get_operand(this, FIRST_OPERAND))) &&
       (ins_get_operand(this, FIRST_OPERAND)->VTy->TID != ArrayTyID))
    {
        //申请存放地址的临时寄存器
        struct _operand addrLoadedReg = operand_pick_temp_register(ARM);
        //取全局变量标号指示的内存单元
        pseudo_ldr("LDR",addrLoadedReg,addr);
        //构造间址
        struct _operand addressingOp = operand_Create_indirect_addressing(
            addrLoadedReg.oprendVal, 
            value_get_elemFormat(ins_get_operand(this,FIRST_OPERAND)));
        //将数据取出至目标
        mov(loaded_target,addressingOp);
        //归还临时地址寄存器
        operand_recycle_temp_register(addrLoadedReg);
    }
    //当对局部数组 或 全局数组的解引用指针（最后一层引用） 操作时
    else if((value_get_type(ins_get_operand(this, FIRST_OPERAND)) == ArrayTyID))
    {
        //封装间接寻址
        struct _operand loadMem = operand_Create_indirect_addressing(
            addr.oprendVal,
            value_get_elemFormat(ins_get_operand(this,FIRST_OPERAND)));
        //执行load
        mov(loaded_target,loadMem);
    }    
}

/**
 * @brief 翻译值传的三地址代码 a = b
 * @author created by LGD on 20221208
 * @update  latest 20230123 添加了对第一二操作数的寄存器回收机制
 *          20230114 添加了对整型和浮点数及其隐式转换的逻辑
 *          20221225 封装后使函数更精简
 *          20221219 更新了获取偏移量的位置(通过哈希表VarMap获取)
*/
void translate_assign_instructions(Instruction* this)
{

    /* 这个列表记录生成的单目表达式里两个个操作数的 寻址方式*/
    AssembleOperand opList[2];

    opList[FIRST_OPERAND] = toOperand(this,FIRST_OPERAND);
    opList[TARGET_OPERAND] = toOperand(this,TARGET_OPERAND);
    if(ins_operand_is_float(this,FIRST_OPERAND))
    {
        if(!ins_operand_is_float(this,TARGET_OPERAND))
            movif(opList[TARGET_OPERAND],opList[FIRST_OPERAND]);
        else
            movff(opList[TARGET_OPERAND],opList[FIRST_OPERAND]);
    }
    else
    {
        if(ins_operand_is_float(this,TARGET_OPERAND))
            movfi(opList[TARGET_OPERAND],opList[FIRST_OPERAND]);
        else
            movii(opList[TARGET_OPERAND],opList[FIRST_OPERAND]);
    }
}

/**
 * @brief 翻译单目运算
 * @birth: Created by LGD on 2023-7-11
*/
void translate_unary_instructions(Instruction* this){

    struct _operand tarOp = toOperand(this, TARGET_OPERAND);
    struct _operand srcOp = toOperand(this, FIRST_OPERAND);
    struct _operand temp = nullop;
    //取相反数
    if(ins_get_opCode(this) == NegativeOP)
    {   
        //判断整数或浮点
        if(value_is_float(ins_get_operand(this, FIRST_OPERAND)))
        {
            //构造立即数0.0
            struct _operand opZero = operand_create_immediate_op(new_float2IEEE75432BITS(0),IEEE754_32BITS);
            //完成减法运算
            temp = subff(opZero,srcOp);
        }
        else{ 
            //构建立即数0
            struct _operand opZero = operand_create_immediate_op(0,INTERGER_TWOSCOMPLEMENT);
            //完成减法运算
            temp = subii(opZero,srcOp);
        }
        mov(tarOp,temp);
        operand_recycle_temp_register(temp);
    }
    //取逻辑反
    else if(ins_get_opCode(this) == NotOP)
    {
        //构建立即数0
        struct _operand immedFalse = operand_create_immediate_op(0,INTERGER_TWOSCOMPLEMENT);
        cmpii(srcOp,immedFalse);
        //构建立即数1
        struct _operand immedTrue = operand_create_immediate_op(1,INTERGER_TWOSCOMPLEMENT);
        movii(tarOp,immedFalse);
        movCondition(tarOp, immedTrue, EQ);
    }
}

/**
 * @brief 翻译And Or运算
 * @birth: Created by LGD on 2023-7-16
**/
void translate_and_or_instructions(Instruction* this){
    AssembleOperand opList[3];
    
    opList[FIRST_OPERAND] = toOperand(this,FIRST_OPERAND);
    opList[SECOND_OPERAND] = toOperand(this,SECOND_OPERAND);
    opList[TARGET_OPERAND] = toOperand(this,TARGET_OPERAND);

    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        assert(NULL && "暂时不支持浮点比较");
    }

    switch(ins_get_opCode(this))
    {
        case LogicAndOP:
            andiii(opList[TARGET_OPERAND], opList[FIRST_OPERAND], opList[SECOND_OPERAND]);
        break;
        case LogicOrOP:
            oriii(opList[TARGET_OPERAND], opList[FIRST_OPERAND], opList[SECOND_OPERAND]);
        break;
    }
}


/**
 * @brief 翻译双目逻辑运算表达式重构版
 * @birth: Created by LGD on 2023-4-18
 * @update: 2023-7-11 条件语句应该在无条件之下
 *          2023-7-16 重构 条件码的判断位置改变
*/
void translate_logical_binary_instruction_new(Instruction* this)
{
    AssembleOperand opList[3];
    
    opList[FIRST_OPERAND] = toOperand(this,FIRST_OPERAND);
    opList[SECOND_OPERAND] = toOperand(this,SECOND_OPERAND);
    opList[TARGET_OPERAND] = toOperand(this,TARGET_OPERAND);
    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        struct _operand tempOp1,tempOp2;
        tempOp1 = operandConvert(opList[FIRST_OPERAND], VFP, 0, 0);
        tempOp2 = operandConvert(opList[SECOND_OPERAND], VFP, 0, 0);
        if(!ins_operand_is_float(this, FIRST_OPERAND))
            operand_regInt2Float(tempOp1,tempOp1);
        if(!ins_operand_is_float(this, SECOND_OPERAND))
            operand_regInt2Float(tempOp2,tempOp2);
        cmpff(tempOp1,tempOp2);
        //释放可能的中间寄存器
        if(!operand_is_same(opList[FIRST_OPERAND], tempOp1))
            operand_recycle_temp_register(tempOp1);
        if(!operand_is_same(opList[SECOND_OPERAND], tempOp2))
            operand_recycle_temp_register(tempOp2);
    }
    else
        cmpii(opList[FIRST_OPERAND],opList[SECOND_OPERAND]);
    
    assert(opList[TARGET_OPERAND].format == INTERGER_TWOSCOMPLEMENT && "要求布尔类型为补码形式");
    movii(opList[TARGET_OPERAND],falseOp);
    switch(ins_get_opCode(this))
    {
        case GreatEqualOP:
            movCondition(opList[TARGET_OPERAND],trueOp,GE);
        break;
        case GreatThanOP:
            movCondition(opList[TARGET_OPERAND],trueOp,GT);
        break;
        case LessEqualOP:
            movCondition(opList[TARGET_OPERAND],trueOp,LE);
        break;        
        case LessThanOP:
            movCondition(opList[TARGET_OPERAND],trueOp,LT);
        break;
        case EqualOP:
            movCondition(opList[TARGET_OPERAND],trueOp,EQ);
        break;
        case NotEqualOP:
            movCondition(opList[TARGET_OPERAND],trueOp,NE);
        break;
    }
}

/**
 * @brief 翻译只包含一个bool变量作为条件的跳转指令
 * @author Created by LGD on 20221225
 * @update: 2023-5-29 重写goto_instruction
 *          2023-7-11 区分GotoOp和GotoOp_WithCond
 *          2023-7-23 支持浮点数作为条件
*/
void translate_goto_instruction_test_bool(Instruction* this)
{
    if(goto_is_conditional(ins_get_opCode(this)))
    {
        Value* cond = ins_get_operand(this, FIRST_OPERAND);
        AssembleOperand op = toOperand(this,FIRST_OPERAND);
        if(value_is_float(cond))
            cmpff(op, floatZeroOp);
        else
            cmpii(op,falseOp);
        branch_instructions_test(ins_get_tarLabel_Conditional(this,false),"EQ",false,NONELABEL);
        branch_instructions_test(ins_get_tarLabel_Conditional(this,true),NONESUFFIX,false,NONELABEL);
    }
    else{
        branch_instructions_test(ins_get_tarLabel_Conditional(this,true),NONESUFFIX,false,NONELABEL);
    }
}


#ifdef OPEN_FUNCTION_WITH_RETURN_VALUE
/**
 * @brief 翻译返回语句
 * @birth: Created by LGD on 2022-12-11
 * @update:2022-12-25 封装后使函数更精简
 *         2023-5-4 重写
 *         2023-6-6 添加返回函数恢复栈帧的语句
 *         2023-7-25 根据子进程调用策略选择不同的寄存器
*/
void translate_return_instructions(Instruction* this)
{
    //无返回值时
    if(user_get_operand_use(&(this->user),0) == NULL)
    {
        resume_spill_area(false);
        //恢复现场
        bash_push_pop_instruction_list("POP",currentPF.used_reg);
        //退出函数
        bash_push_pop_instruction("POP",&fp,&pc,END);
        return;
    }
    //获取要返回的操作数
    struct _operand returnOperand = toOperand(this,FIRST_OPERAND);
    //确定返回值接收策略
    if(!strcmp(currentPF.func_name,"getfloat"))
        mov(returnFloatOp,returnOperand);
    else
    {
        if(value_is_float(ins_get_operand(this, TARGET_OPERAND)))
            mov(returnFloatSoftOp,returnOperand);
        else
            mov(returnIntOp,returnOperand);
    }

    resume_spill_area(false);
    //恢复现场
    bash_push_pop_instruction_list("POP",currentPF.used_reg);
    //退出函数
    bash_push_pop_instruction("POP",&fp,&pc,END);
}
#endif

/**
 * @brief 翻译Label指令 zzq专用
 * @date 20220103
 * @update:2023-5-9 entry 转译为main
*/
void translate_label(Instruction* this)
{
    if(label_is_entry(this))
    {
        Label("entry");
        return;
    }
    Label(ins_get_label(this));
}

/**
 * @brief 翻译函数Label指令 zzq专用
 * @author LGD
 * @date 20230109
*/
void translate_function_label(Instruction* this)
{
    Label(ins_get_label(this));
}
