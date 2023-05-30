#include "value.h"
#include "instruction.h"

char* enum_shift_2_str(enum SHIFT_WAY shiftWay);

char* vfp_suffix_from_type(TypeID type);

char* from_tac_op_2_str(TAC_OP op);

char* enum_instruction_mnemonic_2_str(enum _ARM_Instruction_Mnemonic aim);

char* enum_section_2_str(enum Section sec);

/**
 * @brief 汇编表达式转译为字符串
 * @birth: Created by LGD on 2023-5-30
*/
char* enum_as_expression_2_str(enum _Data_Expression dExpr);