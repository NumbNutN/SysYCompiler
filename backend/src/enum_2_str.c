#include "operand.h"
#include "arm.h"


char* enum_shift_2_str(enum SHIFT_WAY shiftWay)
{
    switch(shiftWay)
    {
        case LSL:
            return "LSL";
        case LSR:
            return "LSR";
        case ASR:
            return "ASR";
        case ROR:
            return "ROR";
        case RRX:
            return "RRX";
        default:
            return "";
    }
}

char* instructionMnemonic2Str(enum _ARM_Instruction_Mnemonic aim)
{
    switch(aim)
    {
        case ADD:
            return "ADD";
        case SUB:
            return "SUB";
        case RSB:
            return "RSB";
        case MUL:
            return "MUL";
        case MLA:
            return "MLA";
        case MOV:
            return "MOV";
        case MVN:
            return "MVN";
        case CMP:
            return "CMP";
        case FADD:
            return "FADD";
        case FSUB:
            return "FSUB";
        case FMUL:
            return "FMUL";
        case FDIV:
            return "FDIV";
        case LDR:
            return "LDR";
        case STR:
            return "STR";
        case AND:
            return "AND";
        case ORR:
            return "ORR";
        default:
            assert("Uncognize instruction_mnemonic");
    }
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
 * @brief 条件后缀转换为字符串
 * @birth: Created by LGD on 2023-7-16
**/
char* cond2Str(enum _Suffix cond)
{
    switch(cond)
    {
        case NE:
            return "NE";
        case EQ:
            return "EQ";
        case GT:
            return "GT";
        case LT:
            return "LT";
        case GE:
            return "GE";
        case LE:
            return "LE";
    }
}

char* enum_section_2_str(enum Section sec)
{
    switch(sec)
    {
        case NONESECTION:
            assert(sec != NONESECTION && "You should select a specified current section");
        break;
        case CODE:
            return ".code";
        case DATA:
            return ".data";
        case BSS:
            return ".bss";
        default:
            assert(false && "Unknown section");
    }
}

/**
 * @brief 汇编表达式转译为字符串
 * @birth: Created by LGD on 2023-5-30
*/
char* enum_as_expression_2_str(enum _Data_Expression dExpr)
{
    switch(dExpr)
    {
        case DOT_LONG:
            return ".long";
        case DOT_ZERO:
            return ".zero";
    }
}
