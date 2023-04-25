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
size_t func_get_param_number(Instruction* this)
{

}

/**
 * @brief 翻译传递参数的指令
 * @author Created by LGD on 2023-3-16
*/
void translate_param_instructions(Instruction* this)
{

    //获取参数 Value
    Value* param_var = get_op_from_param_instruction(this);
    //依据存储位置转换为operand类型
    AssembleOperand param_op = ValuetoOperand(this,param_var);

    int offset;
    if(passed_param_number <= 3)
    //小于等于4个则直接丢内存
        general_data_processing_instructions(MOV,r027[passed_param_number],nullop,param_op,NONESUFFIX,false);
    else
    //申请一个新的参数存放的内存单元
    {
        sp_indicate_offset.addtion = request_new_parameter_stack_memory_unit();
        memory_access_instructions("STR",param_op,sp_indicate_offset,NONESUFFIX,false,NONELABEL);
    }

    //参数计数器自增
    passed_param_number++;
}

/**
 * @brief 无返回值调用
 * @birth: Created by LGD on 2023-3-16
*/
translate_call_instructions(Instruction* this)
{
    //获取跳转标签
    char* label = ins_get_tarLabel(this);
    //执行跳转
    branch_instructions(label,"L",false,NONELABEL);

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
*/
translate_call_with_return_value_instructions(Instruction* this)
{

    //第一步 跳转至对应函数的标号
    char* tarLabel = ins_get_tarLabel(this);

    branch_instructions(tarLabel,"L",false,NONELABEL);
    
    //第二步 回程将R0赋给指定变量
    //根据 The Base Procedure Call Standard
    //32位 (4字节 1字长)的数据 （包括sysy的整型和浮点型数据） 均通过R0 传回
    AssembleOperand op;

    
    ins_variable_load_in_register(this,0,ARM,&op);
    AssembleOperand r0;
    r0.addrMode = REGISTER_DIRECT;
    r0.oprendVal = R0;

    general_data_processing_instructions(MOV,op,nullop,r0,NONESUFFIX,false);

    //暂存寄存器回收
    general_recycle_temp_register_conditional(this,0,op);
    // if(ins_get_op_place(this,0)==IN_MEMORY)
    //     recycle_temp_arm_register(op.oprendVal);

    variable_storage_back(this,0,op.oprendVal);

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
    //乘法左移优化
    if(ins_mul_2_lsl_trigger(this))
    {
        ins_mul_2_lsl(this);
        return;
    }
    if(ins_get_opCode(this) == SubOP)
    {
        translate_sub(this);
        return;
    }
    AssembleOperand op1 = toOperand(this,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(this,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(this,TARGET_OPERAND);

    AssembleOperand middleOp;

    BinaryOperand binaryOp;

    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        //双目中任一不是浮点数
        if(!ins_operand_is_float(this,FIRST_OPERAND) || !ins_operand_is_float(this,SECOND_OPERAND))
        {
            binaryOp = binaryOpfi(op1,op2);
        }
        else
        {
            binaryOp = binaryOpff(op1,op2);
        }

        
        middleOp = operand_pick_temp_register(VFP);

        switch(ins_get_opCode(this))
        {
            case AddOP:
                fadd_and_fsub_instruction("FADD",
                    middleOp,binaryOp.op1,binaryOp.op2,FloatTyID);
            break;
            case SubOP:
                fadd_and_fsub_instruction("FSUB",
                    middleOp,binaryOp.op1,binaryOp.op2,FloatTyID);
            break;
            case MulOP:
                fadd_and_fsub_instruction("FMUL",
                    middleOp,binaryOp.op1,binaryOp.op2,FloatTyID);
            break;
            case DivOP:
                fadd_and_fsub_instruction("FDIV",
                    middleOp,binaryOp.op1,binaryOp.op2,FloatTyID);
            break;
            
        }
        

    }
    else
    {   
        binaryOp = binaryOpii(op1,op2);

    
        switch(ins_get_opCode(this))
        {
            case AddOP:
                middleOp = operand_pick_temp_register(ARM);
                general_data_processing_instructions(ADD,
                    middleOp,binaryOp.op1,binaryOp.op2,NONESUFFIX,false);
            break;
            case SubOP:
                middleOp = operand_pick_temp_register(ARM);
                general_data_processing_instructions(SUB,
                    middleOp,binaryOp.op1,binaryOp.op2,NONESUFFIX,false);
            break;
            case MulOP:
                middleOp = operand_pick_temp_register(ARM);
                general_data_processing_instructions(MUL,
                    middleOp,binaryOp.op1,binaryOp.op2,NONESUFFIX,false);
            break;
#ifdef USE_DIV_ABI
            case DivOP:
                middleOp = r0;
                movii(r0,binaryOp.op1);
                movii(r1,binaryOp.op2);
                branch_instructions_test("__aeabi_idiv","L",false,NONELABEL);
            break;
            case ModOP:
                middleOp = r0;
                movii(r0,binaryOp.op1);
                movii(r1,binaryOp.op2);
                branch_instructions_test("__aeabi_idivmod","L",false,NONELABEL);
#else
            assert("未实现不调用库的除法");
#endif  
        }
    }
    //释放第一、二操作数
    general_recycle_temp_register_conditional(this,FIRST_OPERAND,binaryOp.op1);
    general_recycle_temp_register_conditional(this,SECOND_OPERAND,binaryOp.op2);

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
*/
void translate_sub(Instruction* this)
{   

    AssembleOperand op1 = toOperand(this,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(this,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(this,TARGET_OPERAND);
    AssembleOperand middleOp;

    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        BinaryOperand binaryOp;
        //双目中任一不是浮点数
        if(!ins_operand_is_float(this,FIRST_OPERAND) || !ins_operand_is_float(this,SECOND_OPERAND))
        {
            binaryOp = binaryOpfi(op1,op2);
        }
        else
        {
            binaryOp = binaryOpff(op1,op2);
        }
        middleOp = operand_pick_temp_register(VFP);

        fadd_and_fsub_instruction("FSUB",
            middleOp,binaryOp.op1,binaryOp.op2,FloatTyID);

        //释放第一、二操作数
        general_recycle_temp_register_conditional(this,FIRST_OPERAND,binaryOp.op1);
        general_recycle_temp_register_conditional(this,SECOND_OPERAND,binaryOp.op2);
    }
    else
    {   
        middleOp = operand_pick_temp_register(ARM);
        assert(!(opernad_is_in_instruction(op1) && opernad_is_in_instruction(op2)) && "减法中两个操作数都是立即数是不允许的");
        //如果第2个操作数为立即数，使用SUB指令
        if(opernad_is_in_instruction(op2))
        {
            op1 = operandConvert(op1,ARM,false,IN_MEMORY);
            general_data_processing_instructions(SUB,
                middleOp,op1,op2,NONESUFFIX,false);
        }
        if(opernad_is_in_instruction(op1))
        {
            op2 = operandConvert(op2,ARM,false,IN_MEMORY);
            general_data_processing_instructions(RSB,
                middleOp,op2,op1,NONESUFFIX,false);
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
 * @brief 翻译值传的三地址代码 a = b
 * @author created by LGD on 20221208
 * @update  latest 20230123 添加了对第一二操作数的寄存器回收机制
 *          20230114 添加了对整型和浮点数及其隐式转换的逻辑
 *          20221225 封装后使函数更精简
 *          20221219 更新了获取偏移量的位置(通过哈希表VarMap获取)
*/
void translate_assign_instructions(Instruction* this)
{

    /* 这个列表记录生成的双目表达式里三个操作数的 寻址方式*/
    AssembleOperand opList[2];
    int targetOpMode;
    size_t cnt_tmp_reg = 0;

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
 * @brief 翻译双目逻辑运算表达式重构版
 * @birth: Created by LGD on 2023-4-18
*/
void translate_logical_binary_instruction_new(Instruction* this)
{
    AssembleOperand opList[3];
    TAC_OP opCode = ins_get_opCode(this);
    
    opList[FIRST_OPERAND] = toOperand(this,FIRST_OPERAND);
    opList[SECOND_OPERAND] = toOperand(this,SECOND_OPERAND);
    opList[TARGET_OPERAND] = toOperand(this,TARGET_OPERAND);
    if(ins_operand_is_float(this,FIRST_OPERAND | SECOND_OPERAND))
    {
        assert(NULL && "暂时不支持浮点比较");
    }
    else
    {
        cmpii(opList[FIRST_OPERAND],opList[SECOND_OPERAND]);
    }

    movCondition(opList[TARGET_OPERAND],falseOp,opCode);
    movCondition(opList[TARGET_OPERAND],trueOp,DefaultOP);

}

/**
 * @brief 翻译只包含一个bool变量作为条件的跳转指令
 * @author Created by LGD on 20221225
*/
void translate_goto_instruction_test_bool(Instruction* this)
{

    AssembleOperand op;
    AssembleOperand accessOpInMem[2];
    int tempReg;

    if(goto_is_conditional(ins_get_opCode(this)))
    {
        //有条件时   CMP 不需要cond  不需要S(本身会影响)
        ins_variable_load_in_register(this,1,ARM,&op);
        AssembleOperand trueOp;
        trueOp.addrMode = IMMEDIATE;
        trueOp.oprendVal = 1;
        //比较指令
        general_data_processing_instructions(CMP,op,trueOp,nullop,NONESUFFIX,false);

        branch_instructions_test(ins_get_tarLabel(this),"NE",false,NONELABEL);
    }

    //暂存寄存器回收
    general_recycle_temp_register_conditional(this,1,op);


}


#ifdef OPEN_FUNCTION_WITH_RETURN_VALUE
void translate_return_instructions(Instruction* this)
{
    // @brief:翻译返回语句return
    // @birth:Created by LGD on 20221211
    // @update:latest 20221225 封装后使函数更精简
    //Label(ins_get_label(this));

    AssembleOperand op;
    
    variable_load_in_register(this,get_op_from_return_instruction(this),&op);
    
    AssembleOperand r0;
    r0.addrMode = REGISTER_DIRECT;
    r0.oprendVal = R0;

    general_data_processing_instructions("MOV",r0,op,nullop,NONESUFFIX,false,NONELABEL);
}
#endif




/**
 * @brief 翻译Label指令 zzq专用
 * @date 20220103
*/
void translate_label(Instruction* this)
{
    if(label_is_entry(this))return;
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
        if(opernad_is_in_instruction(opList[FIRST_OPERAND]))
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
        if(opernad_is_in_instruction(opList[FIRST_OPERAND]))
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

/**
 * @brief 翻译前执行的初始化
 * @birth: Created by LGD on 2023-3-28
*/
void TranslateInit()
{
    //初始化用于整型的临时寄存器
    Init_arm_tempReg();
    //初始化浮点临时寄存器
    Free_Vps_Register_Init();
    //初始化链表
    initDlist(); 
}
