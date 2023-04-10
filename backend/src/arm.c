#include "arm.h"
#include <assert.h>
#include "arm_assembly.h"
#include "value.h"
#include <stdarg.h>

int CntAssemble = 0;

assmNode* general_data_processing_instructions(char* opCode,AssembleOperand tar,AssembleOperand op1,AssembleOperand op2,char* suffix,bool symbol,char* label)
{
    /*
        ARM General Data Processing Instructions
        通用数据传输语句
        寻址方式仅限寄存器直接寻址和立即寻址

        加法表达式的节点构建
        目前实现：
            目标操作数寄存器寻址
            源操作数为立即数寻址、直接寻址、寄存器直接寻址
    */
    AddrMode mode;
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);

    mode = tar.addrMode;
    //对于目标数  允许 寄存器寻址
    assert(mode==REGISTER_DIRECT);
    node->op[0] = tar;

    mode = op1.addrMode;
    //对于后面的操作数 允许 立即寻址 寄存器寻址
    assert(mode==IMMEDIATE || mode==REGISTER_DIRECT);
    node->op[1] = op1;

    node->op_len = 2;

    //判断op2 是否不存在   20221202
    if(!op2_is_empty(op2))
    {
        mode = op2.addrMode;
        assert(mode==IMMEDIATE || mode==REGISTER_DIRECT);
        node->op[2] = op2;
        node->op_len += 1;
    }
    
    //Cond后缀
    strcpy(node->suffix,suffix);          //20221202  添加后缀

    //Label标号
    strcpy(node->label,label);

    //指令类型   20221203
    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

assmNode* memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,char* suffix,bool symbol,char* label)
{
    /*
        ARM memory access instructions
    */
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    
    AddrMode mode;
    mode = reg.addrMode;
    assert(mode == REGISTER_DIRECT);
    node->op[0] = reg;


    mode = mem.addrMode;
    assert(mode == REGISTER_INDIRECT || mode == DIRECT || mode == REGISTER_INDIRECT_POST_INCREMENTING || 
    mode == REGISTER_INDIRECT_POST_INCREMENTING || mode == REGISTER_INDIRECT_WITH_OFFSET);
    node->op[1] = mem;

    //Cond后缀
    strcpy(node->suffix,suffix);
    //Label标号
    strcpy(node->label,label);

    //指令类型   20221203
    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

/**
 * @update:2023-4-9 更改了PUSH POP 语句的打印方式
*/
void push_pop_instructions(char* opcode,AssembleOperand reg)
{
    // @brief:生成出入栈的汇编指令
    // @birth:Created by LGD on 20221212
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    node->op[0] = reg;
    node->op_len = 1;
    node->op[0].addrMode = PP;
    strcpy(node->opCode,opcode);
    node->assemType = ASSEM_INSTRUCTION;


    linkNode(node);
}

/**
 * @brief 变长的push尝试
 * @birth: Created by LGD on 2023-4-9
*/
void bash_push_pop_instruction(char* opcode,...)
{
    //参数列表
    va_list ap;
    va_start(ap,opcode);
    
    size_t cnt = 0;
    AssembleOperand* ops;
    while((ops = va_arg(ap,AssembleOperand*)) != END)
        ++cnt;

    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opcode);
    node->op_len = cnt;
    node->opList = (AssembleOperand*)malloc(sizeof(AssembleOperand)*cnt);
    node->assemType = ASSEM_PUSH_POP_INSTRUCTION;

    va_start(ap,opcode);
    cnt = 0;
    while((ops = va_arg(ap,AssembleOperand*)) != END)
    {
        node->opList[cnt] = *ops;
        node->opList[cnt].addrMode = PP;
        ++cnt;
    }
    va_end(ap);

    linkNode(node);
}


void branch_instructions(char* tarLabel,char* suffix,bool symbol,char* label)
{
    /*
        ARM branch instructions
    */
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,"B");
    char* addr = (char*)malloc(sizeof(char)*10);
    node->op[0].oprendVal = addr;
    node->op[0].addrMode = TARGET_LABEL;        //20221203  设定寻址方式
    //目标Label
    strcpy((char*)node->op[0].oprendVal,tarLabel);

    //Cond后缀
    strcpy(node->suffix,suffix);

    //label标号
    strcpy(node->label,label);
    node->symbol = symbol;

    node->op_len = 1;

    //指令类型   20221203
    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

void branch_instructions_test(char* tarLabel,char* suffix,bool symbol,char* label)
{
    /*
        ARM branch instructions
    */
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,"B");
    char* addr = (char*)malloc(sizeof(char)*10);
    node->op[0].oprendVal = addr;
    node->op[0].addrMode = TARGET_LABEL;        //20221203  设定寻址方式
    //目标Label
    strcpy((char*)node->op[0].oprendVal,tarLabel);

    //Cond后缀
    strcpy(node->suffix,suffix);

    //label标号
    strcpy(node->label,label);
    node->symbol = symbol;

    node->op_len = 1;

    //指令类型   20221203
    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

char* vfp_suffix_from_type(TypeID type)
{
    switch(type)
    {
        case FloatTyID:
            return "S";
        case DoubleTyID:
            return "D";
    }
}
/**
 * @brief   浮点数的访存指令
 * @author  Created by LGD on 20230111
*/
void vfp_memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    strcpy(node->suffix,vfp_suffix_from_type(type));

    AddrMode mode;
    mode = reg.addrMode;
    assert(mode == REGISTER_DIRECT);
    node->op[0] = reg;
    mode = mem.addrMode;
    assert(mode == REGISTER_INDIRECT_WITH_OFFSET);
    node->op[1] = mem;

    strcpy(node->label,NONELABEL);

    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);

}

/**
 * @brief 将浮点数转化为有符号或无符号整数  FTOST 转化为有符号整型  FTOUT转化为无符号整型  
 * @param sd 存放整型结果的单精度浮点寄存器
 * @param fm 存放待转换的浮点数的浮点寄存器，其类型必须满足类型后缀
*/
void ftost_and_ftout_instruction(char* opCode,AssembleOperand sd,AssembleOperand fm,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    strcpy(node->suffix,vfp_suffix_from_type(type));
    assert(sd.addrMode == REGISTER_DIRECT);
    assert(fm.addrMode == REGISTER_DIRECT);

    node->op[0] = sd;
    node->op[1] = fm;
    strcpy(node->label,NONELABEL);

    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);

}

/**
 * @brief 将有符号或无符号整数转化为浮点数, FSITO 将有符号整数转为浮点数  FUITO 将无符号整数转为浮点数
 * @param fd 存放浮点数结果的浮点寄存器，其类型必须满足类型后缀
 * @param sm 存放整数的单精度浮点寄存器
 * @author Created by LGD on 20230113
*/
void fsito_and_fuito_instruction(char* opCode,AssembleOperand fd,AssembleOperand sm,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    strcpy(node->suffix,vfp_suffix_from_type(type));
    assert(fd.addrMode == REGISTER_DIRECT);
    assert(sm.addrMode == REGISTER_DIRECT);

    node->op[0] = fd;
    node->op[1] = sm;
    strcpy(node->label,NONELABEL);

    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

/**
 * @brief 浮点数的加减法运算
 * @author Created by LGD on 20230113
*/
void fadd_and_fsub_instruction(char* opCode,AssembleOperand fd,AssembleOperand fn,AssembleOperand fm,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);
    node->op_len = 3;
    strcpy(node->suffix,vfp_suffix_from_type(type));
    assert(fd.addrMode == REGISTER_DIRECT && fn.addrMode == REGISTER_DIRECT && fm.addrMode == REGISTER_DIRECT);

    node->op[0] = fd;
    node->op[1] = fn;
    node->op[2] = fm;
    strcpy(node->label,NONELABEL);

    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

/**
 * @brief arm寄存器与vfp寄存器之间的相互直传
 *          fmrs 将浮点寄存器直传到arm寄存器
 *          fmsr 将arm寄存器直传到浮点寄存器
 * @param rd arm寄存器
 * @param sn 浮点寄存器
 * @author Created by LGD on 20230113
*/
void fmrs_and_fmsr_instruction(char* opCode,AssembleOperand rd,AssembleOperand sn,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    if(!strcmp(opCode,"FMRS"))
    {
        node->op[0] = rd;
        node->op[1] = sn;
    }
    else if(!strcmp(opCode,"FMSR"))
    {
        node->op[0] = sn;
        node->op[1] = rd;
    }
    strcpy(node->label,NONELABEL);
    node->assemType = ASSEM_INSTRUCTION;
    linkNode(node);

}

/**
 * @brief 浮点数的CMP指令
 * @author Created by LGD on 20230113
*/
void fcmp_instruction(AssembleOperand fd,AssembleOperand fm,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,"FCMP");
    node->op_len = 2;
    node->op[0] = fd;
    node->op[1] = fm;
    strcpy(node->label,NONELABEL);
    node->assemType = ASSEM_INSTRUCTION;
    linkNode(node);
}

/**
 * @brief 浮点寄存器VFP之间的传送指令
 *          FABS 取绝对值后传递
 *          FCPY 直传
 *          FNEG 取相反数后传递
 * @param fd 目标操作数
 * @param fm 待直传的操作数
 * @author Created by LGD on 20230114
*/
void fabs_fcpy_and_fneg_instruction(char* opCode,AssembleOperand fd,AssembleOperand fm,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    node->op[0] = fd;
    node->op[1] = fm;
    strcpy(node->label,NONELABEL);
    node->assemType = ASSEM_INSTRUCTION;
    linkNode(node);
}

//伪指令
void Label(char* label)
{
    /*
    创造一个label结点
    */
    if(!label)
        return;
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    node->assemType = LABEL;
    strcpy(node->label,label);
    linkNode(node);
}

/**
 * @brief LDR伪指令
 * @param opCode LDR
 * @birth: Created by LGD on 20230202
*/
void pseudo_ldr(char* opCode,AssembleOperand reg,AssembleOperand immedi)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    node->assemType = LDR_PSEUDO_INSTRUCTION;
    strcpy(node->opCode,opCode);
    node->op[0] = reg;
    node->op[1] = immedi;
    node->op_len = 2;
    linkNode(node);
}

/**
 * @brief FLD伪指令
 * @param opCode LDR
 * @birth: Created by LGD on 20230202
*/
void pseudo_fld(char* opCode,AssembleOperand reg,AssembleOperand immedi,TypeID type)
{
    assmNode* node = (assmNode*)malloc(sizeof(assmNode));
    node->assemType = LDR_PSEUDO_INSTRUCTION;
    strcpy(node->opCode,opCode);
    node->op[0] = reg;
    node->op[0].addrMode = REGISTER_DIRECT;
    node->op[1] = immedi;
    node->op_len = 2;
    linkNode(node);
}

//符号定义 Symbol Definition
//数据定义 Data Definition
//汇编控制 Assembly Control
//其他 Miscellaneous


/**
 * @update: 可以记录链接节点的数量
*/
void linkNode(assmNode* now)
{
    /*
    链接下一个汇编指令
    */
    now->next = NULL;
    prev->next = now;
    prev = now;

    print_single_assembleNode(now);
    ++CntAssemble;
}

//-------------------------------------------------------------------------------
//打印指令
/**
 * @update last 20230124 添加对VFP浮点指令的编号
 *         2023-4-9 添加了基于PUSH POP的新打印方式
*/
void print_operand(AssembleOperand op)
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
            if(op.oprendVal >= FIRST_ARM_REGISTER && op.oprendVal <= LAST_ARM_REGISTER)
                printf("[%c%d, #%d]",'R',op.oprendVal,op.addtion);
            else if(op.oprendVal >= FIRST_VFP_REGISTER && op.oprendVal <= LAST_VFP_REGISTER)
                printf("[%c%d, #%d]",'S',op.oprendVal,op.addtion - FLOATING_POINT_REG_BASE);
            else
                printf("[?%d, #%d]",op.oprendVal,op.addtion);
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
                print_operand(p->op[i]);
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
                print_operand(p->opList[i]);
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
            print_operand(p->op[0]);
            printf(", ");
            printf("=%d\n",p->op[1].oprendVal);
        break;
    }
}

void print_model()
{
    //顺序打印一整个链表的汇编指令
    for(assmNode* p = head->next;p!=NULL;p=p->next)
    {
        print_single_assembleNode(p);
    }
}