#include "arm.h"
#include <assert.h>
#include "arm_assembly.h"
#include "value.h"
#include <stdarg.h>
#include "operand.h"

#include "enum_2_str.h"
#include "print_format.h"

//currentPF
#include "memory_manager.h"

int CntAssemble = 0;

enum _Pick_Arm_Register_Limited global_arm_register_limited = NONE_LIMITED;

/**
 * @brief 翻译通用数据传输指令
 * @update: 2023-3-19 根据乘法寄存器的要求对rm和rs互换
 * @update: 2023-4-20 更改了指令助记符的类型   删去了label选项
*/
void general_data_processing_instructions(enum _ARM_Instruction_Mnemonic opCode,AssembleOperand rd,AssembleOperand rn,AssembleOperand second_operand,char* cond,bool symbol)
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

    //创建新的节点
    assmNode* node = arm_instruction_node_init();


    strcpy(node->opCode,instructionMnemonic2Str(opCode));

    //对于目标数  允许 寄存器寻址
    assert(rd.addrMode==REGISTER_DIRECT);
    node->op[Rd] = rd;

    if(!operand_is_none(rn))
    {
        //对于rn 允许 寄存器寻址
        assert(rn.addrMode==REGISTER_DIRECT);
        node->op[Rn] = rn;
    }
    
    //对于第二操作数 允许 寄存器寻址 直接寻址
    assert(second_operand.addrMode==IMMEDIATE || second_operand.addrMode==REGISTER_DIRECT);
    node->op[2] = second_operand;    

    node->op_len = 3;

    //乘法安全性检查
    if(opCode == MUL && (node->op[1].oprendVal == node->op[2].oprendVal))
    {
        AssembleOperand tmp = node->op[1];
        node->op[1] = node->op[2];
        node->op[2] = tmp;
    }
    
    //Cond后缀
    strcpy(node->suffix,cond);          //20221202  添加后缀

    //指令类型   20221203
    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}
/**
 * @brief 翻译通用数据传输指令
 * @update: 2023-3-19 根据乘法寄存器的要求对rm和rs互换
 * @update: 2023-4-20 更改了指令助记符的类型   删去了label选项
*/
void general_data_processing_instructions_extend(enum _ARM_Instruction_Mnemonic opCode,char* cond,bool symbol,...)
{

    //创建新的节点
    assmNode* node = arm_instruction_node_init();

    strcpy(node->opCode,instructionMnemonic2Str(opCode));

    //处理不定长参数列表
    va_list ops;
    va_start(ops,symbol);
    int cnt=-1;
    AssembleOperand tmp;
    do{
        tmp = va_arg(ops,AssembleOperand);
        ++cnt;
    }while(memcmp(&tmp,&nullop,sizeof(AssembleOperand)));

    struct _operand rd = nullop;
    struct _operand rn= nullop;
    struct _operand rm= nullop;
    struct _operand operand2= nullop;
    struct _operand ra= nullop;

    va_start(ops,symbol);
    if(cnt == 2)
    {
        rd = va_arg(ops,AssembleOperand);
        operand2 = va_arg(ops,AssembleOperand);


        //对于第二操作数 允许 寄存器寻址 直接寻址
        assert(operand2.addrMode==IMMEDIATE || operand2.addrMode==REGISTER_DIRECT);
        node->op[2] = operand2;

        node->op_len = 3;
    }
    else if(cnt == 3)
    {
        rd = va_arg(ops,AssembleOperand);
        rn = va_arg(ops,AssembleOperand);
        operand2 = va_arg(ops,AssembleOperand);

        //对于rn 允许 寄存器寻址
        assert(rn.addrMode==REGISTER_DIRECT);
        node->op[Rn] = rn;

        //对于第二操作数 允许 寄存器寻址 直接寻址
        assert(operand2.addrMode==IMMEDIATE || operand2.addrMode==REGISTER_DIRECT);
        node->op[Rm] = operand2;

        node->op_len = 3;
    }
    else if(cnt == 4)
    {
        rd = va_arg(ops,AssembleOperand);
        rn = va_arg(ops,AssembleOperand);
        rm = va_arg(ops,AssembleOperand);
        ra = va_arg(ops,AssembleOperand);

        //对于rn 允许 寄存器寻址
        assert(rn.addrMode==REGISTER_DIRECT);
        node->op[Rn] = rn;

        //对于rm 允许 寄存器寻址
        assert(rm.addrMode==REGISTER_DIRECT);
        node->op[Rm] = rm;

        //对于ra 允许 寄存器寻址
        assert(ra.addrMode==REGISTER_DIRECT);
        node->op[Ra] = ra;

        node->op_len = 4;
    }
    va_end(ops);

    //对于目标数  允许 寄存器寻址
    assert(rd.addrMode==REGISTER_DIRECT);
    node->op[Rd] = rd;


    //乘法安全性检查
    if(opCode == MUL && (node->op[1].oprendVal == node->op[2].oprendVal))
    {
        AssembleOperand tmp = node->op[1];
        node->op[1] = node->op[2];
        node->op[2] = tmp;
    }
    
    //Cond后缀
    strcpy(node->suffix,cond);          //20221202  添加后缀

    //指令类型   20221203
    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

/**
 * @brief 访存指令
 * @update: 2023-5-3 新增了对偏移寻址的支持
 *          2023-7-17 修复断言缺失REGISTER_INDIRECT_PRE_INCREMENTING
*/
assmNode* memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,char* suffix,bool symbol,char* label)
{

    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    
    AddrMode mode;
    mode = reg.addrMode;
    assert(mode == REGISTER_DIRECT);
    node->op[0] = reg;

    //确定访存方式
    mode = mem.addrMode;
    assert(mode == REGISTER_INDIRECT || mode == DIRECT || mode == REGISTER_INDIRECT_POST_INCREMENTING || 
    mode == REGISTER_INDIRECT_PRE_INCREMENTING || mode == REGISTER_INDIRECT_WITH_OFFSET);
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
    assmNode* node = arm_instruction_node_init();
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
 * @update: 2023-7-13 有序的寄存器列表
 *          2023-7-27 更新当前函数堆栈信息
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


    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,opcode);
    node->op_len = cnt;
    node->opList = (AssembleOperand*)malloc(sizeof(AssembleOperand)*cnt);
    node->assemType = ASSEM_PUSH_POP_INSTRUCTION;

    va_start(ap,opcode);
    cnt = 0;
    while((ops = va_arg(ap,AssembleOperand*)) != END)
    {
        memcpy(&(node->opList[cnt]) ,ops,sizeof(AssembleOperand));
        node->opList[cnt].addrMode = PP;
        ++cnt;
    }
    va_end(ap);

    /*   开始排序  */
    for(int i=0;i<cnt-1;i++)
        for(int j=0;j<cnt-i-1;j++)
        {
            if(node->opList[j].oprendVal > node->opList[j+1].oprendVal)
            {
                int temp = (int)node->opList[j].oprendVal;
                node->opList[j].oprendVal = node->opList[j+1].oprendVal;
                node->opList[j+1].oprendVal = temp;
            }
        }

    currentPF.SPOffset -= cnt*4;
    linkNode(node);
}
/**
 * @brief 变长的push尝试
 * @birth: Created by LGD on 2023-4-9
 * @update: 2023-7-11 当列表为空时不生成指令
 *          2023-7-13 有序的寄存器列表
 *          2023-7-27 更新当前函数堆栈信息
*/
void bash_push_pop_instruction_list(char* opcode,struct _operand* regList)
{
    size_t cnt = 0;
    while (!operand_is_none(regList[cnt])){
        ++cnt;
    }
    /* 为0则不需要生成 */
    if(cnt == 0)return;

    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,opcode);
    node->op_len = cnt;
    node->opList = (AssembleOperand*)malloc(sizeof(AssembleOperand)*cnt);
    memcpy(node->opList,regList,cnt*sizeof(struct _operand));

    /*   开始排序  */
    for(int i=0;i<cnt-1;i++)
        for(int j=0;j<cnt-i-1;j++)
        {
            if(node->opList[j].oprendVal > node->opList[j+1].oprendVal)
            {
                int temp = (int)node->opList[j].oprendVal;
                node->opList[j].oprendVal = node->opList[j+1].oprendVal;
                node->opList[j+1].oprendVal = temp;
            }
        }

    node->assemType = ASSEM_PUSH_POP_INSTRUCTION;
    currentPF.SPOffset -= cnt*4;
    linkNode(node);
}


void branch_instructions(char* tarLabel,char* suffix,bool symbol,char* label)
{
    /*
        ARM branch instructions
    */
    assmNode* node = arm_instruction_node_init();
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

//TODO
void branch_instructions_test(char* tarLabel,char* suffix,bool symbol,char* label)
{
    /*
        ARM branch instructions
    */
    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,"B");
    char* addr = (char*)malloc(sizeof(char)*128);
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


/**
 * @brief   浮点数的访存指令
 * @author  Created by LGD on 20230111
*/
void vfp_memory_access_instructions(char* opCode,AssembleOperand reg,AssembleOperand mem,TypeID type)
{
    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    strcpy(node->suffix,vfp_suffix_from_type(type));

    AddrMode mode;
    mode = reg.addrMode;
    assert(mode == REGISTER_DIRECT);
    assert(operand_get_regType(reg) == VFP);
    node->op[0] = reg;
    mode = mem.addrMode;
    assert(mode == REGISTER_INDIRECT_WITH_OFFSET || mode == REGISTER_INDIRECT || mode == REGISTER_INDIRECT_PRE_INCREMENTING);
    assert(operand_get_regType(mem) == ARM);
    node->op[1] = mem;

    strcpy(node->label,NONELABEL);

    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);

}

/**
 * @brief 将浮点数转化为有符号或无符号整数  FTOSI 转化为有符号整型  FTOUT转化为无符号整型  
 * @param sd 存放整型结果的单精度浮点寄存器
 * @param fm 存放待转换的浮点数的浮点寄存器，其类型必须满足类型后缀
*/
void ftosi_and_ftout_instruction(char* opCode,AssembleOperand sd,AssembleOperand fm,TypeID type)
{
    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    strcpy(node->suffix,"");
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
    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    strcpy(node->suffix,"");
    assert(fd.addrMode == REGISTER_DIRECT);
    assert(sm.addrMode == REGISTER_DIRECT);

    node->op[0] = fd;
    node->op[1] = sm;

    node->assemType = ASSEM_INSTRUCTION;

    linkNode(node);
}

/**
 * @brief 浮点数的加减法运算
 * @author Created by LGD on 20230113
*/
void fadd_and_fsub_instruction(char* opCode,AssembleOperand fd,AssembleOperand fn,AssembleOperand fm,TypeID type)
{
    assmNode* node = arm_instruction_node_init();
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
    assmNode* node = arm_instruction_node_init();
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
    node->assemType = ASSEM_INSTRUCTION;
    linkNode(node);

}

/**
 * @brief 浮点数的CMP指令
 * @author Created by LGD on 20230113
*/
void fcmp_instruction(AssembleOperand fd,AssembleOperand fm,TypeID type)
{
    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,"FCMP");
    node->op_len = 2;
    node->op[0] = fd;
    node->op[1] = fm;
    node->assemType = ASSEM_INSTRUCTION;
    strcpy(node->suffix,vfp_suffix_from_type(type));
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
    assmNode* node = arm_instruction_node_init();
    strcpy(node->opCode,opCode);
    node->op_len = 2;
    node->op[0] = fd;
    node->op[1] = fm;
    node->assemType = ASSEM_INSTRUCTION;
    linkNode(node);
}

//

//伪指令
void Label(char* label)
{
    /*
    创造一个label结点
    */
    if(!label)
        return;
    assmNode* node = arm_instruction_node_init();
    node->assemType = LABEL;
    strcpy(node->label,label);
    linkNode(node);
}

/**
* @brief 生成一个未定义指令
* @birth:Created by LGD on 2023-7-9
*/
void undefined()
{
    assmNode* node = arm_instruction_node_init();
    node->assemType = UNDEF;
    linkNode(node);
}

/**
 * @brief 将FMSCR拷贝到CPSR
 * @birth: Created by LGD on 2023-7-22
*/
void fmstat()
{
    assmNode* node = arm_instruction_node_init();
    node->assemType = FMSTAT;
    linkNode(node);
}

/**
 * @brief 生成一个文字池伪指令
 * @birth: Created by LGD on 2023-7-21
*/
void pool()
{
    assmNode* node = arm_instruction_node_init();
    node->assemType = POOL;
    linkNode(node);
}

/**
 * @brief LDR伪指令
 * @param opCode LDR
 * @birth: Created by LGD on 20230202
 * @update: 2023-5-29 添加判断 ldr伪指令要确保操作数是立即数或者标号
*/
void pseudo_ldr(char* opCode,AssembleOperand reg,AssembleOperand immedi)
{
    assert(immedi.addrMode == IMMEDIATE || immedi.addrMode == LABEL_MARKED_LOCATION &&
        "LDR Pseudo require Immediate or label");
    assmNode* node = arm_instruction_node_init();
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
 * @update: 2023-7-25 增加对节点寻址模式的检查
*/
void pseudo_fld(char* opCode,AssembleOperand reg,AssembleOperand immedi,TypeID type)
{
    assert(immedi.addrMode == IMMEDIATE || immedi.addrMode == LABEL_MARKED_LOCATION &&
        "FLD Pseudo require Immediate or label");
    assmNode* node = arm_instruction_node_init();
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
 *          2023-7-27 prev可以为链表中或链表末
*/
void linkNode(assmNode* now)
{
    /*
    链接下一个汇编指令
    */
    now->next = prev->next;
    prev->next = now;
    prev = now;

    setvbuf(stdout,NULL,_IONBF,0);
    print_single_assembleNode(now);
    ++CntAssemble;
}

