#include "arm_assembly.h"

/**************************************************************/
/*                          Debug                           */
/***************************************************************/

/**
 *@brief:监测当前指令执行结束后是否有未归还的寄存器
 *@birth:Created by LGD on 2023-5-29
*/
void detect_temp_register_status()
{
    for(int i=0;i<TEMP_REG_NUM;i++)
    {
        assert(TempARMRegList[i].isAviliable && "current Instruction use temp register but didn't recycle\n");
    }
}