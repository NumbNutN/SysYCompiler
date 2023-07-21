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
*/
void translate_param_instructions(Instruction* this)
{
    //依据存储位置转换为operand类型
    AssembleOperand param_op = toOperand(this,FIRST_OPERAND);
    //获取当前传递参数序号
    size_t passed_param_number = get_parameter_idx_by_name(ins_get_assign_left_value(this)->name);

    //判断当前传入的变量是否是全局数组
    if(value_is_global(ins_get_operand(this,FIRST_OPERAND)))
    {
        //需要将全局标号的地址取出到寄存器后传递
        //小于等于4个则直接丢R0-R3
        if(passed_param_number <= 3)
        {
            pseudo_ldr("LDR",r027[passed_param_number],param_op);
            //寄存器限制
            add_register_limited(1 << passed_param_number);
        }
        //多于4个参数，临时寄存器调出后回收
        else{
            struct _operand temp = operand_pick_temp_register(ARM);
            pseudo_ldr("LDR",temp,param_op);
            movii(param_push_op,temp);
            operand_recycle_temp_register(temp);
        }
    }
    //当前传入的变量是其余情况（全局变量 全局数组的解引用 局部数组 局部变量）
    else{
        //小于等于4个则直接丢R0-R3
        if(passed_param_number <= 3)
        {
            operand_load_to_specified_register(param_op,r027[passed_param_number]);
            //寄存器限制
            add_register_limited(1 << passed_param_number);
        }
        else
            //堆入栈顶
            movii(param_push_op,param_op);
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
    remove_register_limited(PARAMETER1_LIMITED | 
                    PARAMETER2_LIMITED |
                    PARAMETER3_LIMITED |
                    PARAMETER4_LIMITED);
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

    branch_instructions(tarLabel,"L",false,NONELABEL);

    //取消寄存器限制
    remove_register_limited(PARAMETER1_LIMITED | 
                    PARAMETER2_LIMITED |
                    PARAMETER3_LIMITED |
                    PARAMETER4_LIMITED);
    add_register_limited(RETURN_VALUE_LIMITED);

    //第二步 确保栈顶恢复到栈帧的位置
    general_data_processing_instructions_extend(MOV,NONESUFFIX,false,sp,fp,nullop);
    
    remove_register_limited(
                    RETURN_VALUE_LIMITED);
    //第三步 回程将R0赋给指定变量
    //根据 The Base Procedure Call Standard
    //32位 (4字节 1字长)的数据 （包括sysy的整型和浮点型数据） 均通过R0 传回

    struct _operand returnVal = toOperand(this,TARGET_OPERAND);

    //如果返回值无效，则无需翻译
    if(!operand_is_unallocated(returnVal))
        movii(returnVal,r0);

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
 * @brief 翻译所有的双目赋值运算,但是采用add + mov 的新方法
 * @birth: Created by LGD on 20230226
 * @update: 20230306 新的变量位置切换
 *          2023-4-10 添加除法运算
*/
void translate_binary_expression_binary_and_assign(Instruction* this)
{   
    TAC_OP opCode = ins_get_opCode(this);
    //乘法左移优化
    if(ins_mul_2_lsl_trigger(this))
    {
        ins_mul_2_lsl(this);
        return;
    }
    if(opCode == SubOP)
    {
        translate_sub(this);
        return;
    }
    struct _operand op1,op2,srcOp1,srcOp2;
    op1 = srcOp1 = toOperand(this,FIRST_OPERAND);
    op2 = srcOp2 = toOperand(this,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(this,TARGET_OPERAND);

    AssembleOperand middleOp;

    //如果使用了除法，由于在一个Instruction内完成传参，需要限制r0和r1的访问权限
    if((opCode == DivOP) || (opCode == ModOP))
        add_register_limited(PARAMETER1_LIMITED | PARAMETER2_LIMITED);

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
                general_data_processing_instructions(ADD,
                    middleOp,op1,op2,NONESUFFIX,false);
            break;
            case SubOP:
                middleOp = operand_pick_temp_register(ARM);
                general_data_processing_instructions(SUB,
                    middleOp,op1,op2,NONESUFFIX,false);
            break;
            case MulOP:
                middleOp = operand_pick_temp_register(ARM);
                general_data_processing_instructions(MUL,
                    middleOp,op1,op2,NONESUFFIX,false);
            break;
#ifdef USE_DIV_ABI
            case DivOP:
                middleOp = r0;
                movii(r0,op1);
                movii(r1,op2);
                branch_instructions_test("__aeabi_idiv","L",false,NONELABEL);
            break;
            case ModOP:
                middleOp = r1;
                movii(r0,op1);
                movii(r1,op2);
                branch_instructions_test("__aeabi_idivmod","L",false,NONELABEL);
            break;
#else
            assert("未实现不调用库的除法");
#endif  
        }
    }
    //如果使用了除法，由于在一个Instruction内完成传参，需要限制r0和r1的访问权限
    if((opCode == DivOP) || (opCode == ModOP))
        remove_register_limited(PARAMETER1_LIMITED | PARAMETER2_LIMITED);
    
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

        fadd_and_fsub_instruction("FSUB",
            middleOp,op1,op2,FloatTyID);

        //释放第一、二操作数
        general_recycle_temp_register_conditional(this,FIRST_OPERAND,op1);
        general_recycle_temp_register_conditional(this,SECOND_OPERAND,op2);
    }
    else
    {   
        middleOp = operand_pick_temp_register(ARM);

#ifndef ALLOW_TWO_IMMEDIATE
        assert(!(opernad_is_immediate(op1) && opernad_is_immediate(op2)) && "减法中两个操作数都是立即数是不允许的");
#endif
        //如果第1个操作数为立即数，第2个不是，使用RSB指令
        if(opernad_is_immediate(op1) && !opernad_is_immediate(op2))
        {
            op2 = operandConvert(op2,ARM,false,IN_MEMORY);
            general_data_processing_instructions(RSB,
                middleOp,op2,op1,NONESUFFIX,false);
        }
        //第1个操作数不是立即数，第2个操作数可以是任何数，使用SUB指令
        else{
            op1 = operandConvert(op1,ARM,false,IN_MEMORY | IN_INSTRUCTION);
            op2 = operandConvert(op2,ARM,false,IN_MEMORY | IN_INSTRUCTION);
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
    struct _operand step_long = operand_create_immediate_op(ins_getelementptr_get_step_long(this));
    struct _operand arrBase = arrBaseOri;
    //如果解引用的对象是全局数组
    if(value_is_global(ins_get_operand(this, FIRST_OPERAND))){
        //采用临时寄存器作为全局数组的基地址
        arrBase = operand_pick_temp_register(ARM);
        pseudo_ldr("LDR",arrBase,arrBaseOri);
    }
    //解引用的对象是局部数组或者全局数组的解引用
    else
        arrBase = operand_load_to_register(arrBaseOri,nullop,ARM);


    idx = operand_load_to_register(idx,nullop,ARM);
    //使用乘加指令
    step_long = operand_load_to_register(step_long,nullop,ARM);
    struct _operand middleOp = operand_pick_temp_register(ARM);
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
    struct _operand stored_elem = toOperand(this,SECOND_OPERAND);
    struct _operand addr = toOperand(this,FIRST_OPERAND);

    //当对全局变量操作时
    if((value_is_global(ins_get_operand(this, FIRST_OPERAND))) &&
       (ins_get_operand(this, FIRST_OPERAND)->VTy->TID != ArrayTyID))
    {
        //将需要存储的数据加载到寄存器中
        stored_elem = operand_load_to_register(stored_elem,nullop,ARM);
        //申请存放地址的临时寄存器
        struct _operand tempAddrOp = operand_pick_temp_register(ARM);
        //取全局变量标号指示的内存单元
        pseudo_ldr("LDR",tempAddrOp,addr);
        //修改临时寄存器的寻址模式
        operand_change_addressing_mode(&tempAddrOp,REGISTER_INDIRECT);
        //将数据送入内存单元
        //TODO
        reg2mem(stored_elem, tempAddrOp);
        //归还可能的存储数据的临时寄存器
        general_recycle_temp_register_conditional(this,SECOND_OPERAND,stored_elem);
        //归还临时地址寄存器
        operand_recycle_temp_register(tempAddrOp);
    }
    //当对局部数组 或 全局数组的解引用指针（最后一层引用） 操作时
    else if((value_get_type(ins_get_operand(this, FIRST_OPERAND)) == ArrayTyID))
    {
        //将需要存储的数据加载到寄存器中
        stored_elem = operand_load_to_register(stored_elem,nullop,ARM);
        //封装间接寻址
        struct _operand storeMem = operand_Create_indirect_addressing(addr);
        //执行store
        reg2mem(stored_elem, storeMem);
        //归还可能的存储数据的临时寄存器
        general_recycle_temp_register_conditional(this,SECOND_OPERAND,stored_elem);
    }
    //其他情况为对局部变量的操作
    //退化为赋值操作
    else
    {
        /* 这个列表记录生成的单目表达式里两个个操作数的 寻址方式*/
        AssembleOperand opList[2];

        opList[TARGET_OPERAND] = toOperand(this,FIRST_OPERAND);
        opList[FIRST_OPERAND] = toOperand(this,SECOND_OPERAND);
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
        struct _operand tempAddrOp = operand_pick_temp_register(ARM);
        //取全局变量标号指示的内存单元
        pseudo_ldr("LDR",tempAddrOp,addr);
        //修改临时寄存器的寻址方式
        operand_change_addressing_mode(&tempAddrOp,REGISTER_INDIRECT);
        //将数据取出至目标
        movii(loaded_target,tempAddrOp);
        //归还临时地址寄存器
        operand_recycle_temp_register(tempAddrOp);
    }
    //当对局部数组 或 全局数组的解引用指针（最后一层引用） 操作时
    else if((value_get_type(ins_get_operand(this, FIRST_OPERAND)) == ArrayTyID))
    {
        //封装间接寻址
        struct _operand loadMem = operand_Create_indirect_addressing(addr);
        //执行load
        movii(loaded_target,loadMem);

    }
    //其他情况为对局部变量的操作
    //退化为赋值操作
    else
    {
        translate_assign_instructions(this);
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
    if(ins_get_opCode(this) == NegativeOP)
    {   
        AssembleOperand opList[2];
        opList[FIRST_OPERAND] = toOperand(this,FIRST_OPERAND);
        opList[TARGET_OPERAND] = toOperand(this,TARGET_OPERAND);
        movini(opList[TARGET_OPERAND],opList[FIRST_OPERAND]);
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
        assert(NULL && "暂时不支持浮点比较");
    }
    cmpii(opList[FIRST_OPERAND],opList[SECOND_OPERAND]);
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
*/
void translate_goto_instruction_test_bool(Instruction* this)
{

    if(goto_is_conditional(ins_get_opCode(this)))
    {
        AssembleOperand op = toOperand(this,FIRST_OPERAND);
        //有条件时   CMP 不需要cond  不需要S(本身会影响)
        //比较指令
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
 * @update:2023-6-6 添加返回函数恢复栈帧的语句
*/
void translate_return_instructions(Instruction* this)
{

    struct _operand returnOperand = toOperand(this,FIRST_OPERAND);
    movii(r0,returnOperand);
    //当翻译返回语句时，此后R0应当是禁用状态
    //add_register_limited(RETURN_VALUE_LIMITED);
    //为了确保函数正常返回，这里将添加跳出语句
    //恢复当前函数栈帧
    reset_sp_value(false);
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


#ifdef LLVM_LOAD_AND_STORE_INSERTED


size_t ins_get_load_and_store_vitual_address(Instruction* this)
{
    
}

/**
 * @brief 翻译load指令
 * @birth: Created by LGD on 2023-3-10
*/
void translate_load_instruction(Instruction* this)
{
    size_t viMem = ins_get_load_and_store_vitual_address(this);
    int offset = get_virtual_Stack_Memory_mapping(cur_memory_mapping_map,viMem);

    AssembleOperand op;
    op = toOperand(this,FIRST_OPERAND);
    sp_indicate_offset.addtion = offset;

    if(ins_operand_is_float(this,FIRST_OPERAND))
        memory_access_instructions("LDR",op,sp_indicate_offset,NONESUFFIX,false,NONELABEL);
    else
        vfp_memory_access_instructions("FLD",op,sp_indicate_offset,FloatTyID);
    
}
/**
 * @brief 翻译store指令
 * @birth: Created by LGD on 2023-3-10
*/
void translate_store_instruction(Instruction* this)
{
    size_t viMem = ins_get_load_and_store_vitual_address(this);
    int offset = get_virtual_Stack_Memory_mapping(cur_memory_mapping_map,viMem);

    AssembleOperand op;
    op = toOperand(this,FIRST_OPERAND);
    sp_indicate_offset.addtion = offset;

    if(ins_operand_is_float(this,FIRST_OPERAND))
        memory_access_instructions("STR",op,sp_indicate_offset,NONESUFFIX,false,NONELABEL);
    else
        vfp_memory_access_instructions("FST",op,sp_indicate_offset,FloatTyID);
}

#endif




#ifdef OPEN_LOAD_AND_STORE_BINARY_OPERATE

/**
 * @brief 翻译所有的双目赋值运算 add mul adc
 * @author Created by LGD on 20221229
 * @update  latest 20230123 添加了对第一二操作数的寄存器回收机制
 *          20230113 考虑整型和浮点型混合运算的隐式转换
*/
void translate_binary_expression_test(Instruction* this)
{
    
    AssembleOperand opList[3];      /* 这个列表记录生成的双目表达式里三个操作数的 寻址方式*/
    
    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        //当第一/二操作数之一为浮点数时统一用vfp浮点寄存器获取数据
        ins_variable_load_in_register(this,FIRST_OPERAND,VFP,&opList[FIRST_OPERAND]);
        ins_variable_load_in_register(this,SECOND_OPERAND,VFP,&opList[SECOND_OPERAND]);
        ins_target_operand_load_in_register(this,&opList[TARGET_OPERAND],FIRST_OPERAND | SECOND_OPERAND);

        fadd_and_fsub_instruction(from_tac_op_2_str(ins_get_opCode(this)),
                    opList[0],opList[1],opList[2],FloatTyID);
    }
    else
    {
        //当第一/二操作数均为整型时用arm寄存器获取数据
        ins_variable_load_in_register(this,FIRST_OPERAND,ARM,&opList[FIRST_OPERAND]);
        ins_variable_load_in_register(this,SECOND_OPERAND,ARM,&opList[SECOND_OPERAND]);
        ins_target_operand_load_in_register(this,&opList[TARGET_OPERAND],FIRST_OPERAND | SECOND_OPERAND);
        
        general_data_processing_instructions(from_tac_op_2_str(ins_get_opCode(this)),
                    opList[0],opList[1],opList[2],NONESUFFIX,false,NONELABEL);
    }
    variable_storage_back_new(this,0,opList[0].oprendVal);

    //若第1/2操作数原本的地址为内存或者 第1/2操作数出现了类型提升，将其寄存器归还
    for(int i=1;i<=ins_get_operand_num(this);i++)
    {
        if(variable_is_in_memory(this,ins_get_operand(this,i))
            || (ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND) && !ins_operand_is_float(this,i)))
            general_recycle_temp_register(this,i,opList[i]);
    }
}

/**
 * @brief 翻译双目逻辑运算表达式
 * @author Created by LGD on 20221225
 * @update 20230113 考虑整型和浮点型混合运算的隐式转换
 * @update 2023-3-28 针对CMP语句不可能出现两个立即数的问题
*/
void translate_logical_binary_instruction(Instruction* this)
{
    AssembleOperand opList[3];
    TAC_OP opCode = ins_get_opCode(this);
    
    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        ins_variable_load_in_register(this,FIRST_OPERAND,VFP,&opList[FIRST_OPERAND]);
        ins_variable_load_in_register(this,SECOND_OPERAND,VFP,&opList[SECOND_OPERAND]);

        //2023-3-28 如果目标操作数为立即数
        if(opernad_is_immediate(opList[FIRST_OPERAND]))
            opList[FIRST_OPERAND] = operand_load_immediate(opList[FIRST_OPERAND],VFP);
        fcmp_instruction(opList[FIRST_OPERAND],opList[SECOND_OPERAND],FloatTyID);
        //回收寄存器
        operand_recycle_temp_register(opList[FIRST_OPERAND]);
    }
    else
    {
        ins_variable_load_in_register(this,FIRST_OPERAND,ARM,&opList[FIRST_OPERAND]);
        ins_variable_load_in_register(this,SECOND_OPERAND,ARM,&opList[SECOND_OPERAND]);
        
        //2023-3-28 如果目标操作数为立即数
        if(opernad_is_immediate(opList[FIRST_OPERAND]))
            opList[FIRST_OPERAND] = operand_load_immediate(opList[FIRST_OPERAND],ARM);
        general_data_processing_instructions("CMP",opList[FIRST_OPERAND],opList[SECOND_OPERAND],nullop,NONESUFFIX,false,NONELABEL);
        //回收寄存器
        operand_recycle_temp_register(opList[FIRST_OPERAND]);
    }

    //暂存寄存器回收
    for(int i=1;i<3;i++)
        general_recycle_temp_register_conditional(this,i,opList[i]);

    AssembleOperand boolOp;
    ins_variable_load_in_register(this,0,ARM,&boolOp);

    AssembleOperand falseOp;
    falseOp.addrMode = IMMEDIATE;
    falseOp.oprendVal = 0;
    general_data_processing_instructions("MOV",boolOp,falseOp,nullop,NONESUFFIX,false,NONELABEL);

    AssembleOperand trueOp;
    trueOp.addrMode = IMMEDIATE;
    trueOp.oprendVal = 1;
    general_data_processing_instructions("MOV",boolOp,trueOp,nullop,from_tac_op_2_str(opCode),false,NONELABEL);

    variable_storage_back(this,0,opList[0].oprendVal);
   //暂存寄存器回收
    if(get_variable_place_by_order(this,0)==IN_MEMORY)
        recycle_temp_arm_register(opList[0].oprendVal);
    
}

#endif



