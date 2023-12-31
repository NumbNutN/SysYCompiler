#include "operand.h"
#include "arm_assembly.h"
#include "arm.h"
#include "variable_map.h"
#include "instruction.h"
#include "operand.h"

#include "dependency.h"

#include <assert.h>
#include <string.h>
#include <math.h>

#include "optimize.h"

/**
 * @brief 判断一个数是否是2的指数
*/
bool number_is_power_of_2(int num)
{
    size_t cnt_bit1 = 0;
    do
    {
        cnt_bit1 += num & 0x1;
        num = num >> 1;
    } while (num != 0);
    return cnt_bit1 == 1;
}

/**
 * @brief 获取右移位数
 * @birth: Created by LGD on 2023-8-20
*/
uint8_t number_get_shift_time(int32_t num)
{
    assert(num > 0);
    assert(number_is_power_of_2(num));
    //循环
    for(int i=0;i<31;++i)
    {
        if(num & 0x1)return i;
        num = num >> 1;
    }
}

/**
 * @brief 判断除数是否可以优化
 * @birth: Created by LGD on 2023-8-20
 * @update: 2023-8-20 判断操作数是否是浮点数
*/
bool div_optimize_trigger(Instruction* this)
{
    TAC_OP opCode = ins_get_opCode(this);
    if(opCode == DivOP)
    {
        struct _operand divisor;
        Value* divisor_val;
        divisor = toOperand(this,SECOND_OPERAND);
        divisor_val = ins_get_operand(this, SECOND_OPERAND);
        if(value_is_float(divisor_val))return false;
        if(!operand_is_immediate(divisor))return false;
        if(operand_get_immediate(divisor) < 0)return false;
        return number_is_power_of_2(operand_get_immediate(divisor));
    }
    else{
        return false;
    }

}

/**
 * @brief 判断操作数是否可以移位优化
 * @update: 2023-8-20 判断操作数是否是浮点数
*/
bool number_is_lsl_trigger(Instruction* this,struct _LSL_FEATURE* feat)
{
    struct _operand op1,op2,srcOp1,srcOp2;
    op1 = srcOp1 = toOperand(this,FIRST_OPERAND);
    op2 = srcOp2 = toOperand(this,SECOND_OPERAND);
    TAC_OP opCode = ins_get_opCode(this);
    if(opCode == MulOP)
    {
        Value* op1_var = ins_get_operand(this, FIRST_OPERAND);
        Value* op2_var = ins_get_operand(this, SECOND_OPERAND);
        if( value_is_float(op1_var) || value_is_float(op2_var))return false;
        if(operand_is_immediate(op1) || operand_is_immediate(op2))
        {
            if(operand_is_immediate(op1))
            {
                *feat = number_is_lsl(operand_get_immediate(op1));
                feat->op_idx = 1;
            }
            else if (operand_is_immediate(op2))
            {
                *feat = number_is_lsl(operand_get_immediate(op2));
                feat->op_idx = 2;                
            }
        }
        else return false;

        if ((feat->feature == LSL_NORMAL) || (feat->feature == _2_N_SUB_1))return false;
        else return true;
    }
    else{
        return false;
    }
}


/**
 * @brief 乘法可优化数
 * @birth: Created by LGD on 2023-8-20
*/
struct _LSL_FEATURE number_is_lsl(int32_t num)
{
    //2^n 统计1的个数为1
    //2^n + 1  第0位为1且其余位共1位1
    //2^n - 1  第0位为1且+1后统计1的个数为1
    //2^(m+n)+2^n   统计1的个数为2  2^n(2^m + 1)

    int8_t b1 = -1;
    int8_t b2 = -1;
    uint8_t cnt = 0;
    int32_t temp = num;
    struct _LSL_FEATURE res = {.feature=LSL_NORMAL,.b1=-1,.b2=-1};
    if (num < 0)return res;
    if (num == 0){res.feature = LSL_ZERO;return res;}
    //循环
    for(int i=0;i<31;++i)
    {
        cnt += (temp & 1);
        if((b1 == -1) && (temp & 1)) b1 = i; //b1未赋值 当前temp末位为1
        else if((b1 != -1) && (b2 == -1) && (temp & 1)) b2 = i; //b1赋值 b2未赋值
        temp = temp >> 1;
    }

    //2^n
    if(cnt == 1)
    {
        res.feature = _2_N;
        res.b1 = b1;
        return res;
    }
    //2^n + 1 2^(m+n)+2^n
    else if (cnt == 2)
    {
        if(b1 == 0){
            res.feature = _2_N_ADD_1;
            res.b1 = b2;
        }
        else if(b1 != 0){
            res.feature = _2_M_N_2_N_1;
            res.b1 = b1;
            res.b2 = b2;
        }
        return res;
    }
    else {
        //判断 2^n - 1
        temp = num + 1;
        cnt = 0;
        b1 = -1;
        b2 = -1;
        //循环
        for(int i=0;i<31;++i)
        {
            cnt += (temp & 1);
            if((b1 == 0) && (temp & 1)) b1 = i; //b1未赋值 当前temp末位为1
            else if((b1 != 0) && (b2 == 0) && (temp & 1)) b2 = i; //b1赋值 b2未赋值
            temp = temp >> 1;
        }
        if(cnt == 1)
        {
            res.feature = _2_N_SUB_1;
            res.b1 = b1;
        }
        return res;
    }
}




/**
 * @brief 乘法转移码条件触发器
 * @update: Created by LGD on 2023-4-20
*/
bool ins_mul_2_lsl_trigger(Instruction* ins)
{
    AssembleOperand op1 = toOperand(ins,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(ins,SECOND_OPERAND);

    return ins_get_opCode(ins) == MulOP &&
    (
        (op1.addrMode == IMMEDIATE && number_is_power_of_2(op1.oprendVal))||
        (op2.addrMode == IMMEDIATE && number_is_power_of_2(op2.oprendVal))
    );
}

/**
 * @brief 乘法移位优化
 * @birth: Created by LGD on 2023-4-19
 * @update: 2023-5-29 添加寄存器回收
 *          2023-7-29 更简洁的表达
*/
void ins_mul_2_lsl(Instruction* ins)
{
    AssembleOperand op1 = toOperand(ins,FIRST_OPERAND);
    AssembleOperand op2 = toOperand(ins,SECOND_OPERAND);
    AssembleOperand tarOp = toOperand(ins,TARGET_OPERAND);

    if(op1.addrMode == IMMEDIATE && number_is_power_of_2(op1.oprendVal))
    {
        AssembleOperand cvtOp2;
        
        cvtOp2 = operandConvert(op2, ARM, 0, 0);

        operand_set_shift(&cvtOp2,LSL,log(op1.oprendVal)/log(2));
        
        //第二操作数需要放置在合适的位置
        general_data_processing_instructions(MOV,tarOp,nullop,cvtOp2,NONESUFFIX,false);

        //回收寄存器
        if(!operand_is_same(cvtOp2, op2))
            operand_recycle_temp_register(cvtOp2);
    }
    else
    {
        AssembleOperand cvtOp1;

        cvtOp1 = operandConvert(op1, ARM, 0, 0);
            
        operand_set_shift(&cvtOp1,LSL,log(op2.oprendVal)/log(2));

        general_data_processing_instructions(MOV,tarOp,nullop,cvtOp1,NONESUFFIX,false);

        if(!operand_is_same(cvtOp1, op1))
            operand_recycle_temp_register(cvtOp1);
    }
}


void mul2lsl(assmNode* ins)
{   
    //判断是否是乘法
    assert(!strcmp(ins->opCode,"MUL"));
    //判断是否乘以2的倍数
    //struct _operand secondOperand = AssemblyNode_get_opernad(ins,SECOND_OPERAND);
    //assert(operand_get_immediate(secondOperand) % 2 == 0);

    strcpy(ins->opCode,"MOV");
    ins->op[2].oprendVal = 2;
}

/**
 * @brief 当判断出强制跳转语句接一个标号时，可以删去该跳转
 * @birth: Created by LGD on 2023-8-9
*/
void remove_unnessary_branch(){
    assmNode* p;
    for(p = head;p != NULL;p = p->next){
        //当前语句是branch

        //debug
        //print_single_assembleNode(p);

        if(strcmp("B",p->opCode))continue;
        assmNode* pNext = p->next;
        //下一个语句是label
        //跳过边界
        while(pNext->assemType == INSTRUCTION_BOUNDARY)pNext = pNext->next;
        if(pNext->assemType != LABEL)continue;
        //branch语句指向标号和下一个标号一致
        if(strcmp((char*)p->op[0].oprendVal,pNext->label))continue;
        //移除节点
        codeRemoveNode(p);
    }
}

// /**
//  * @brief 根据指令特点添加寄存器
//  * @birth: Created by LGD on 2023-8-21
// */
// void add_used_reg(assmNode* node)
// {
//     for(int i=1;i<node->op_len;++i)
//     {
//         if(node->op[i].addrMode == REGISTER_DIRECT)
//             regList_Add_Reg(node->op[i]);
//     }
// }

/**
 * @brief 设置寄存器的性质
*/
void set_reg_kill(assmNode* node)
{
    for(int i = 1;i < regList.node->op_len; ++i)
    {
        if((node->op[0].addrMode == regList.node->op[i].addrMode) && \
         (node->op[0].oprendVal == regList.node->op[i].oprendVal) )
        regList.node->op[i].regStat = REG_KILLED;
    }
}

/**
 * @brief 消除单个mov指令
*/
void delete_posible_mov(assmNode* node)
{
    struct _operand dele_reg = node->op[0];
    int idx = reg_in_reglist(dele_reg);
    if(idx != -1)
    {
        //如果mov rd,rn rn不是寄存器 不允消除
        if(!operand_is_in_register(node->op[2]))return;
        //这个寄存器存在于list中  准备消除
        regList.node->op[idx].oprendVal = node->op[2].oprendVal;
        codeRemoveNode(node);
    }
}

/**
 * @brief 删除和消去多余的寄存器
 * @birth: Created by LGD on 2023-8-21
*/
void delete_none_used_reg(){
    assmNode* p;
    for(p = last;p != NULL;p = p->past)
    {

        //DEBUG
        //print_single_assembleNode(p);

        //到达一个新的边界寄存器列表状态清空
        if(p->assemType == INSTRUCTION_BOUNDARY)
        {
            regList_Init();
            switch(p->addtion)
            {
                case AddOP:
                case SubOP:
                case MulOP:
                case GetelementptrOP:
                case GreatEqualOP:
                case GreatThanOP:
                case LessEqualOP:
                case LessThanOP:
                    regList.stat = REGLIST_OK;
                break;
                default:
                    regList.stat = UN_OPTIMIZABLE_INSTRUCTION;
            }
            continue;
        }

        //遇到符合的可优化内容
        if((regList.stat == REGLIST_OK) && \
        (!strcmp(p->opCode,"ADD") || !strcmp(p->opCode,"SUB") || !strcmp(p->opCode,"MUL") || !strcmp(p->opCode,"CMP") || !strcmp(p->opCode,"MLA")))
        {
            regList.node = p;
            regList.stat = ALREADY_USED;
            continue;
        }

        if(regList.stat == ALREADY_USED)
        {
            //如果是mov语句  准备消除
            if(!strcmp(p->opCode,"MOV")){
                delete_posible_mov(p);
            }
            else{
                set_reg_kill(p);
            }
        }

    }
}

/**
 * @brief 判断标签是否可以等效到别的标签
*/
void label_equivalent(assmNode* node)
{
    /* pNext: Branch instruction */
    assert(node->assemType == LABEL);
    //从这里遍历判断下方是否有除b以外的内容
    assmNode* pNext = node->next;
    //跳过边界
    while(pNext->assemType == INSTRUCTION_BOUNDARY)pNext = pNext->next;
    if(!strcmp(node->opCode,"B") && !strcmp(node->suffix,NONESUFFIX))
    {
        //可以等效
        strcpy(node->equalLabel,(char*)pNext->op[0].oprendVal);
    }
    else {
        strcpy(node->equalLabel,"");
    }
}

/**
 * @brief 等效标签更改与消除
*/
void change_and_delete_unused_label(assmNode* node)
{
    if(strcmp(node->equalLabel,""))
    {
        //当前标签具有等效标签
        assmNode* b_node = HashMapGet(label_used_list, node->label);
        while(b_node != NULL)
        {
            //遍历依赖关系链表
            strcpy((char*)b_node->op[0].oprendVal, node->equalLabel);
            b_node = b_node->refereence_next;
        }
        //将当前标签的B消除
        assmNode* b_node_in_unused_label = node->next;
        //跳过边界
        while(b_node_in_unused_label->assemType == INSTRUCTION_BOUNDARY)b_node_in_unused_label = b_node_in_unused_label->next;
        codeRemoveNode(b_node_in_unused_label);
        //将当前标签消除
        codeRemoveNode(node);
    }
}

/**
 * @brief 消除冗余的跳转
 * @birth: Created by LGD on 2023-8-21
*/
void delete_unused_label()
{
    //第一步 判断所有标号的等效关系
    char* label;
    assmNode* label_node;
    HashMap_foreach(label_node_list,label, label_node)
    {
        label_equivalent(label_node);
    }

    //第二步 将具有等效关系的标签的所有reference branch全部调整
    HashMap_foreach(label_node_list, label, label_node)
    {
        change_and_delete_unused_label(label_node);
    }
}