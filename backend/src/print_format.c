#include "operand.h"
#include "arm_assembly.h"
#include "enum_2_str.h"

//-------------------------------------------------------------------------------
//打印指令

/**
 * @update last 20230124 添加对VFP浮点指令的编号
 *         2023-4-9 添加了基于PUSH POP的新打印方式
 *         2023-4-20 为移位添加了打印方式
 *         2023-5-4 添加了前变址 使用寄存器作为偏移量的打印方式
*/
void print_operand(AssembleOperand op,size_t opernadIdx)
{
    /*
        依据寻址方式打印单个操作数的文本格式
        测试进度：
        立即寻址 支持
        直接寻址 支持
        寄存器直接寻址 支持
    */
    switch(op.addrMode)
    {
        //MOV R0,#15
        case IMMEDIATE:
            printf("%c%d",'#',op.oprendVal);
        break;
        case DIRECT:
            //LDR R0,MEM
            printf("%c%d%c",'[',op.oprendVal,']');
        break;
        case REGISTER_DIRECT:
            //MOV R0,R1
            if(op.oprendVal >= FIRST_ARM_REGISTER && op.oprendVal <= LAST_ARM_REGISTER)
                printf("%c%d",'R',op.oprendVal);
            else if(op.oprendVal >= FIRST_VFP_REGISTER && op.oprendVal <= LAST_VFP_REGISTER)
                printf("%c%d",'S',op.oprendVal - FLOATING_POINT_REG_BASE);
            else
                printf("?%d",op.oprendVal);
            /***********************第二操作数****************************/
            {
                //2023-4-20 第二操作数
                if(opernadIdx == SECOND_OPERAND && op.addrMode == REGISTER_DIRECT)
                    if(op.shiftWay == NONE_SHIFT)
                        break;
                    else
                        printf(",%s #%d",enum_shift_2_str(op.shiftWay),op.shiftNum);
            }
            /*****************************************************************/
        break;
        case REGISTER_INDIRECT:
            //LDR R0,[R1]
            if(op.oprendVal >= FIRST_ARM_REGISTER && op.oprendVal <= LAST_ARM_REGISTER)
                printf("[%c%d]",'R',op.oprendVal);
            else if(op.oprendVal >= FIRST_VFP_REGISTER && op.oprendVal <= LAST_VFP_REGISTER)
                printf("[%c%d]",'S',op.oprendVal - FLOATING_POINT_REG_BASE);
            else
                printf("[?%d]",op.oprendVal);
        break;
        case REGISTER_INDIRECT_POST_INCREMENTING:
            //LDR R0,[R1],#4
            if(op.oprendVal >= FIRST_ARM_REGISTER && op.oprendVal <= LAST_ARM_REGISTER)
                printf("[%c%d], #%d",'R',op.oprendVal,op.addtion);
            else if(op.oprendVal >= FIRST_VFP_REGISTER && op.oprendVal <= LAST_VFP_REGISTER)
                printf("[%c%d], #%d",'S',op.oprendVal,op.addtion - FLOATING_POINT_REG_BASE);
            else
                printf("[?%d], #%d",op.oprendVal,op.addtion);
        break;
        case REGISTER_INDIRECT_PRE_INCREMENTING:
            //LDR R0, [R1, #4]
            if(op.oprendVal >= FIRST_ARM_REGISTER && op.oprendVal <= LAST_ARM_REGISTER)
                printf("[%c%d, #%d]!",'R',op.oprendVal,op.addtion);
            else if(op.oprendVal >= FIRST_VFP_REGISTER && op.oprendVal <= LAST_VFP_REGISTER)
                printf("[%c%d, #%d]!",'S',op.oprendVal,op.addtion - FLOATING_POINT_REG_BASE);
            else
                printf("[?%d, #%d]!",op.oprendVal,op.addtion);
        break;
        case REGISTER_INDIRECT_WITH_OFFSET:
        {
            if(op.offsetType ==  OFFSET_IMMED|| op.offsetType ==NONE_OFFSET)
            {
                if(op.oprendVal >= FIRST_ARM_REGISTER && op.oprendVal <= LAST_ARM_REGISTER)
                    printf("[R%d, #%d]",op.oprendVal,op.addtion);
                else if(op.oprendVal >= FIRST_VFP_REGISTER && op.oprendVal <= LAST_VFP_REGISTER)
                    printf("[S%d, #%d]",op.oprendVal,op.addtion - FLOATING_POINT_REG_BASE);
                else
                    printf("[?%d, #%d]",op.oprendVal,op.addtion);
            }
            else if(op.offsetType == OFFSET_IN_REGISTER)
            {
                 if(op.oprendVal >= FIRST_ARM_REGISTER && op.oprendVal <= LAST_ARM_REGISTER)
                    printf("[R%d, R%d]",op.oprendVal,op.addtion);
                else if(op.oprendVal >= FIRST_VFP_REGISTER && op.oprendVal <= LAST_VFP_REGISTER)
                    printf("[S%d, R%d]",op.oprendVal,op.addtion - FLOATING_POINT_REG_BASE);
                else
                    printf("[?%d, R%d]",op.oprendVal,op.addtion);               
            }
        }

        break;
            //B FOO
        case TARGET_LABEL:
            printf("%s",(char*)op.oprendVal);
        break;
        //PUSH {R0}
        case PP:
            printf("R%d",op.oprendVal);
        break;
        default:
            printf("EOF");
        break;
    }
}

/**
 * @brief 对单个汇编语句节点的格式化输出
 * @birth: Created by LGD on 20230227
 * @update: 2023-4-9 添加了针对PUSH POP 的打印方式
*/
void print_single_assembleNode(assmNode* p)
{
    switch(p->assemType)
    {
        case ASSEM_INSTRUCTION:
            //打印标号                      //20221202
            printf("\t");
            //打印操作码
            printf("%s",p->opCode);
            //打印后缀Cond                      20221204
            printf("%s\t",p->suffix);
            //打印操作数
            for(int i=0;i<p->op_len;i++)
            {
                /***********************MOV指令****************************/
                if(!strcmp(p->opCode,"MOV") && i == Rn)
                    continue;
                /***********************CMP指令****************************/    
                if(!strcmp(p->opCode,"CMP") && i == Rn)
                    continue;
                /**********************************************************/
                print_operand(p->op[i],i);
                if(i!=p->op_len-1)
                    printf(", ");
            }
            printf("\n");
        break;
        case ASSEM_PUSH_POP_INSTRUCTION:
            //打印标号
            printf("\t");
            //打印操作码
            printf("%s",p->opCode);
            //打印后缀Cond
            printf("%s\t",p->suffix);
            //打印前括号
            printf("{");
            //打印操作数
            for(int i=0;i<p->op_len;i++)
            {
                print_operand(p->opList[i],i);
                if(i!=p->op_len-1)
                    printf(", ");
            }
            //打印后括号
            printf("}");
            printf("\n");
        break;
        case LABEL:
            printf("%s:\n",p->label);
        break;
        case LDR_PSEUDO_INSTRUCTION:
            printf("\t");
            printf("%s\t",p->opCode);
            print_operand(p->op[0],0);
            printf(", ");
            if(p->op[1].addrMode == IMMEDIATE)
                printf("=%d\n",(int)p->op[1].oprendVal);
            if(p->op[1].addrMode == LABEL_MARKED_LOCATION)
                printf("=%s\n",(char*)p->op[1].oprendVal);
        break;
    }
}

/**
 *@brief 打印单个数据段节点
 *@brith: Created by LGD on 2023-5-30
*/
void print_single_data(struct _dataNode* node)
{
    if(node->label != NULL)
        printf("%s:\n",node->label);
    else
        printf("\t\t");
    
    printf("%s\t%d\n",enum_as_expression_2_str(node->dExp),node->content);
}

void print_model()
{
    //打印数据段节点
    printf(".section .data\n");
    for(struct _dataNode* node = dataList;node != NULL;node=node->next)
    {
        print_single_data(node);
    }
    //打印zeroInit段节点
    printf(".section .bss\n");
    for(struct _dataNode* node = bssList;node != NULL;node=node->next)
    {
        print_single_data(node);
    }
    //顺序打印一整个链表的汇编指令
    printf(".section .code\n");
    for(assmNode* p = head->next;p!=NULL;p=p->next)
    {
        print_single_assembleNode(p);
    }
}

