#include "operand.h"

char* enum_shift_2_str(enum SHIFT_WAY shiftWay)
{
    switch(shiftWay)
    {
        case NONE_SHIFT:
            return "";
        case LSL:
            return "LSL";
        case LSR:
            return "LSR";
        case ASR:
            return "ASR";
        
    }
}