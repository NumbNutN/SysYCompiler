#include "instruction.h"

/**
 * @brief 乘法转移码条件触发器
 * @update: Created by LGD on 2023-4-20
*/
bool ins_mul_2_lsl_trigger(Instruction* ins);

/**
 * @brief 乘法移位优化
 * @update: Created by LGD on 2023-4-19
*/
void ins_mul_2_lsl(Instruction* ins);

/**
 * @brief 当判断出强制跳转语句接一个标号时，可以删去该跳转
 * @birth: Created by LGD on 2023-8-9
*/
void remove_unnessary_branch(void);