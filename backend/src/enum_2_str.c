#include "operand.h"

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

char* from_tac_op_2_str(TAC_OP op)
{
    switch(op)
    {
        case AddOP:
            return "ADD";
        case SubOP:
            return "SUB";
        case MulOP:
            return "MUL";
        // case Goto:
        //     return "B";
        case LessEqualOP:
            return "LE";
        case GreatEqualOP:
            return "GE";
        case LessThanOP:
            return "LT";
        case GreatThanOP:
            return "GT";
        case EqualOP:
            return "EQ";
        case NotEqualOP:
            return "NE";


        // case Goto_LessEqual:
        //     return "LE";
        // case Goto_GreatEqual:
        //     return "GE";
        // case Goto_LessThan:
        //     return "LT";
        // case Goto_GreatThan:
        //     return "GT";
        // case Goto_Equal:
        //     return "E";
        // case Goto_NotEqual:
        //     return "NE";
        
        case GotoWithConditionOP:
            return "E";
        //add on 20221208
        case AssignOP:
            return "MOV";

        default:
            return "";
    }
}
