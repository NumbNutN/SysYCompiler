#include "value.h"
#include "instruction.h"

char* enum_shift_2_str(enum SHIFT_WAY shiftWay);

char* vfp_suffix_from_type(TypeID type);

/**
 * @brief 将操作码转换为字符串形式
 * @update: 2023-7-16 删去了对条件后缀的转换
**/
char* tacOp2Str(TAC_OP op);

/**
 * @brief 条件后缀转换为字符串
 * @birth: Created by LGD on 2023-7-16
**/
char* cond2Str(enum _Suffix cond);

char* instructionMnemonic2Str(enum _ARM_Instruction_Mnemonic aim);

char* enum_section_2_str(enum Section sec);

/**
 * @brief 汇编表达式转译为字符串
 * @birth: Created by LGD on 2023-5-30
*/
char* enum_as_expression_2_str(enum _Data_Expression dExpr);